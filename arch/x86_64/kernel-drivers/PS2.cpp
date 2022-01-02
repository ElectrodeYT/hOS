#include <kernel-drivers/PS2.h>
#include <debug/klog.h>

namespace Kernel {


char PS2::getKey() {
    acquire(&mutex);
    if(buffer && read_pos != write_pos) {
        char ret = buffer[read_pos];
        read_pos = (read_pos + 1) % buffer_size;
        release(&mutex);
        return ret;
    }
    release(&mutex);
    return 0;
}

void PS2::Interrupt(int irq) {
    if(!init_done) { return; }
    if(!buffer) { inb(data_port); return; }
    if(processing_command) { processing_command = false; }
    if(irq == 1) {
        // First channel
        uint8_t data = inb(data_port);
        KLog::the().printf("PS2: got data from first channel: %i\n\r", data);
        DecodeAndBuffer(data);
    } else if(irq == 12) {
        // Second channel
    } else {
        // We shouldnt be here
        KLog::the().printf("PS2: got weird interrupt %i, we only accept 1 or 12\n\r", irq);
    }
}

void PS2::Init() {
    // Send the disable commands
    // We legit dont actually care about any return values or interrupts from these so it
    // doesnt even really matter
    outb(status_command_port, 0xAD);
    outb(status_command_port, 0xA7);
    // Read from the data port once to flush the buffer
    inb(data_port);
    // Read the CCB
    outb(status_command_port, 0x20);
    // Wait until we get some data
    PollForData();
    uint8_t ccb = inb(data_port);
    // Check if bit 5 is set
    // Since we have disabled both ports, if bit 5 is not set, then the
    // 2nd PS/2 port doesnt exist
    if(ccb & (1 << 5)) { second_channel_works = false; }

    // Clear bits 0,1,6
    // This disables both IRQs and translation
    ccb &= ~(1 | (1 << 1) | (1 << 6));
    // Set CCB
    outb(status_command_port, 0x60);
    PollForDataWrite();
    outb(data_port, ccb);

    // Perform self test
    outb(status_command_port, 0xAA);
    PollForData();
    uint8_t self_test = inb(data_port);
    if(self_test != 0x55) {
        KLog::the().printf("PS2: controller self test failed\n\r");
        return;
    }
    // Reset the ccb
    outb(status_command_port, 0x60);
    PollForDataWrite();
    outb(data_port, ccb);

    if(second_channel_works) {
        // We also check if the second port works by enabling the 2nd port
        // the 0xA8 command should get ignored by a single-channel PS2 controller
        outb(status_command_port, 0xA8);
        // Read the CCB
        outb(status_command_port, 0x20);
        // Wait until we get some data
        PollForData();
        uint8_t ccb = inb(data_port);
        if(ccb & (1 << 5)) {
            second_channel_works = false;
        } else {
            // Disable the 2nd port again
            outb(status_command_port, 0xA7);
        }
    }

    bool timed_out = false;

    // Test the first channel
    outb(status_command_port, 0xAB);
    uint8_t first_channel_self_test = ReadData(&timed_out);
    if(timed_out || first_channel_self_test != 0x00) { first_channel_works = false; }

    // Test the second channel
    if(second_channel_works) {
        outb(status_command_port, 0xA9);
        uint8_t second_channel_self_test = ReadData(&timed_out);
        if(timed_out || second_channel_self_test != 0x00) { second_channel_works = false; }
    }

    // Check if we have any working channels lol
    if(!first_channel_works && !second_channel_works) {
        KLog::the().printf("PS2: both channels self test failed\n\r");
        return;
    }

    // Reset and detect both devices
    if(first_channel_works) { PS2::DetectChannel1(); }

    // Register interrupts
    Interrupts::the().RegisterIRQHandler(1, PS2InterruptWrapper);
    Interrupts::the().RegisterIRQHandler(12, PS2InterruptWrapper);

    // TOOD: detect second channel
    // probably need a better way to detect them anyway, and cleaner code here
    // the code here is probably reliant on QEMU, due to really shit timeouts
    // this probably needs scheduler changes

    if(first_channel_works) {
        if(first_channel_is_keyboard) {
            KLog::the().printf("PS2: first channel is a keyboard\n\r");
            EnableChannel1();
        } else if(first_channel_is_mouse) {
            KLog::the().printf("PS2: first channel is a mouse\n\r");
        } else {
            KLog::the().printf("PS2: first channel works but no supported device is connected\n\r");
        }
    } else {
        KLog::the().printf("PS2: first channel detection failed\n\r"); 
    }

    buffer = new char[buffer_size];
    init_done = true;
}

void PS2::DetectChannel1() {
    if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
    bool timed_out;
    // Reset device
    outb(data_port, 0xFF);
    // Read two bytes
    uint8_t ack_byte = ReadData(&timed_out);
    if(timed_out || !((ack_byte == 0xFA) || (ack_byte == 0xAA))) { first_channel_works = false; return; }
    
    // The wiki is a bit weird here but lets just read some bytes here to ensure the buffer is properly empty
    ReadData(&timed_out);
    // These arent timed because if they are sent they would be the PS/2 Device id
    inb(data_port);
    inb(data_port);
    inb(data_port);
    inb(data_port);
    // The device should exist, lets now send the identify command
    // First disable scanning, so as to not conflict with this
    EmptyDataOutput();
    outb(data_port, 0xF5);
    if(!GetACK()) { first_channel_works = false; return; }
    outb(data_port, 0xF2);
    if(!GetACK()) { first_channel_works = false; return; }
    uint8_t first_byte;
    uint8_t second_byte;
    bool second_byte_timed_out;
    first_byte = ReadData(&timed_out);
    second_byte = ReadData(&second_byte_timed_out);
    if(timed_out) { first_channel_works = false; return; }
    // Try and detect any we know of
    if(timed_out) { 
        // Is a AT keyboard, idk if they are compatible but just say its a keyboard
        first_channel_is_keyboard = true;
        return;
    }
    if(second_byte_timed_out) {
        if(first_byte == 0x00 || first_byte == 0x03 || first_byte == 0x04) {
            // Is a mouse
            first_channel_is_mouse = true;
        }
    } else {
        if((first_byte == 0xAB && second_byte == 0x41) || (first_byte == 0xAB && first_byte == 0xC1)) {
            // MF2 keyboard with translation enabled
            first_channel_is_keyboard = true;
        } else if((first_byte == 0xAB && second_byte == 0x83)) {
            // MF2 keyboard
            first_channel_is_keyboard = true;
        }
    }

}

void PS2::DetectChannel2() {

}

void PS2::EnableChannel1() {
    if(first_channel_is_keyboard) {
        // Set scancode set
        if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
        outb(data_port, 0xF0);
        if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
        outb(data_port, 1);
        if(!GetACK()) { first_channel_works = false; return; }
        
        
        // Set typematic
        // basically just put semi-random times in here
        // key repeat delay = 750ms, repeat rate = 15Hz?
        if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
        outb(data_port, 0xF3);
        if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
        outb(data_port, 0b01010000);
        if(!GetACK()) { first_channel_works = false; return; }
    }

    // Read the CCB
    outb(status_command_port, 0x20);
    // Wait until we get some data
    PollForData();
    uint8_t ccb = inb(data_port);
    // Set bit 0
    // This enables the IRQs for the keyboard
    ccb |= 1;
    // Set CCB
    outb(status_command_port, 0x60);
    PollForDataWrite();
    outb(data_port, ccb);

    // Enable scanning
    if(!TimedPollForDataWrite()) { first_channel_works = false; return; }
    outb(data_port, 0xF4);
    if(!GetACK()) { first_channel_works = false; return; }
}

void PS2::EnableChannel2() {

}

// TODO: handle 0xE0 extensions
// also holy fuck the pause button scares me almost
void PS2::DecodeAndBuffer(uint8_t scancode) {
    // Special cases
    if(scancode == 0x2A || scancode == 0x36) { shift++; return; }
    if(scancode == 0xAA || scancode == 0xB6) { shift--; return;  }
    if(scancode == 0x38) { alt++; return;  }
    if(scancode == 0xB8) { alt--; return;  }
    if(scancode == 0x1D) { control++; return;  }
    if(scancode == 0x9D) { control--; return;  }

    // Probably a normal key
    // TODO: do stuff with the released codes
    bool released = (scancode & (1 << 7));
    if(released) { return; }

    uint8_t byte_to_buffer = translation_table_normal[(uint8_t)(scancode & ~(1 << 8))];
    // Check if we actually have the space lol
    if(write_pos == ((read_pos - 1 + buffer_size) % buffer_size)) {
        KLog::the().printf("PS2: dropping char %c\n\r", byte_to_buffer);
        return;
    }
    buffer[write_pos] = byte_to_buffer;
    write_pos = (write_pos + 1) % buffer_size;
}

void PS2InterruptWrapper(Interrupts::ISRRegisters* regs) {
    PS2::the().Interrupt(regs->int_num);
}

}