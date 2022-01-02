#ifndef PS2_H
#define PS2_H
#include <stdint.h>
#include <stddef.h>
#include <interrupts.h>
#include <CPP/mutex.h>
#include <timer.h>
#include <hardware/instructions.h>

namespace Kernel {

class PS2 {
public:
    char getKey();
    void Interrupt(int irq);
    void Init();
    static PS2& the() {
        static PS2 instance;
        return instance;
    }
private:
    // Block until the output buffer has data
    void PollForData() {
        while(~(inb(status_command_port)) & 1);
    }

    // Block until the input buffer is clear
    void PollForDataWrite() {
        while(inb(status_command_port) & (1 << 1));    
    }

    // Poll for write with timeout
    bool TimedPollForDataWrite() {
        uint64_t kernel_timestamp_begin = Hardware::Timer::GetCurrentTimestamp();
        while(Hardware::Timer::GetCurrentTimestamp() <= (kernel_timestamp_begin + 1000)) {
            if(~(inb(status_command_port)) & (1 << 1)) {
                return true;
            }
        }
        return false;
    }

    // Read with a time out
    uint8_t ReadData(bool* timed_out) {
        uint64_t kernel_timestamp_begin = Hardware::Timer::GetCurrentTimestamp();
        while(Hardware::Timer::GetCurrentTimestamp() <= (kernel_timestamp_begin + 1000)) {
            if(inb(status_command_port) & 1) {
                *timed_out = false;
                return inb(data_port);
            }
        }
        *timed_out = true;
        return 0;
    }

    // Read and discard from data output
    void EmptyDataOutput() {
        while(inb(status_command_port) & 1) { inb(data_port); }
    }

    void DetectChannel1();
    void DetectChannel2();

    void EnableChannel1();
    void EnableChannel2();

    void DecodeAndBuffer(uint8_t scancode);

    bool GetACK() {
        bool timed_out;
        uint8_t ret = ReadData(&timed_out);
        if(timed_out || ret != 0xFA) { return false; }
        return true;
    }

    const uint16_t data_port = 0x60;
    const uint16_t status_command_port = 0x64;

    bool init_done = false;
    bool processing_command = false;

    const size_t buffer_size = 20;
    char* buffer;
    size_t read_pos = 0;
    size_t write_pos = 0;

    bool first_channel_works = true;
    bool second_channel_works = true;

    bool first_channel_is_keyboard = false;
    bool second_channel_is_keyboard = false;
    
    bool first_channel_is_mouse = false;
    bool second_channel_is_mouse = false;
    
    // Keyboard states
    // This influences the scancode decoding
    uint8_t shift = 0;
    uint8_t control = 0;
    uint8_t alt = 0;

    mutex_t mutex;

    // TODO: shift + control + alt shit
    uint8_t translation_table_normal[128] = {
        0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0x8, 0x9,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',', '.', '/', 0, '*', 0, 0x20, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0
    };
};

void PS2InterruptWrapper(Interrupts::ISRRegisters* regs);

}

#endif