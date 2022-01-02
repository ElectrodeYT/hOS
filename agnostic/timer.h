#ifndef AGNOSTIC_TIMER_H
#define AGNOSTIC_TIMER_H

#ifndef NKERNEL


namespace Kernel {
    namespace Hardware {
        namespace Timer {
            using timer_callback_t = void(*)(void*);

            // Platform independent kernel functions
            int GetCurrentTimerValue();
            uint64_t GetCurrentTimestamp();

            // Called when a timer interrupt occurs
            void AgnosticTimerInterrupt();

            // Initializes the timer
            void InitTimer();

            // Attaches a timer based callback
            void AttachCallback(timer_callback_t callback, void* arg);

            // Platform dependent kernel functions
            void ArchSetupTimer();
            int ArchTimerValue();
            int ArchTimerFrequency();
            void ArchResetTimer();
        }
    }
}
#else
#error User mode timer not supported
#endif

#endif