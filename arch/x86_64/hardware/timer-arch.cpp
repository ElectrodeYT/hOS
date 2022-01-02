#include <mem.h>
#include <timer.h>
// For setting up the timer interrupt
#include <interrupts.h>
#include <hardware/instructions.h>
// For checking if the context needs to be switched
#include <processes/scheduler.h>

namespace Kernel {
    namespace Hardware {
        namespace Timer {
            const uint8_t channel0 = 0x40;
            const uint8_t mode_command = 0x43;
            int divisor = 1193; // 1000Hz
                
            void ArchTimerInterrupt(Interrupts::ISRRegisters* registers) {
                // Call agnostic timer interrupt
                AgnosticTimerInterrupt();
                // Call the scheduler to check if there is the need to schedule a new process
                Processes::Scheduler::the().TimerCallback(registers);
            }

            void ArchSetupTimer() {
                // Disable interrupts
                asm volatile("cli");
                
                // Configure channel 0
                outb(mode_command, 0b00110110); // Channel 0, lo/hi byte, square wave

                outb(channel0, divisor & 0xFF);
                outb(channel0, (divisor >> 8) & 0xFF);

                // Attach interrupt
                Interrupts::the().RegisterIRQHandler(0, ArchTimerInterrupt);
                // Reenable interupts
                asm volatile("sti");
            }

            int ArchTimerValue() {
                // Disable interrupts
                asm volatile("cli");
                // Latch the value
                outb(mode_command, 0b00000000); // Channel 0, latch command
                // Read the value
                int val = inb(channel0) | ((int)inb(channel0) << 8);

                // Reenable interrupts
                asm volatile("sti");
                return val;
            }

            int ArchTimerFrequency() {
                return 1193180 / divisor;
            }

            void ArchResetTimer() {

            }
        }
    }
}