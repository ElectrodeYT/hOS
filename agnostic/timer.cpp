#include <mem.h>
#include <timer.h>


#ifndef NKERNEL

namespace Kernel {
    namespace Hardware {
        namespace Timer {

            struct TimercallbackLL {
                timer_callback_t callback;
                void* arg;
                TimercallbackLL* next;
            };

            static TimercallbackLL* ll = NULL;

            // Gets the current timestamp of the setup timer.
            int GetCurrentTimeStamp() {
                return ArchTimerValue() / ArchTimerFrequency();
            }
            // Resets the timer.
            void ResetTimeStamp() {
                ArchResetTimer();
            }
            // Initialize the timer.
            void InitTimer() {
                ArchSetupTimer();
                ArchResetTimer();
            }

            void AgnosticTimerInterrupt() {
                // Traverse the linked list and call all of the callbacks
                TimercallbackLL* curr = ll;
                while(curr != NULL) {
                    curr->callback(curr->arg);
                    curr = curr->next;
                }
            }

            void AttachCallback(timer_callback_t callback, void* arg) {
                // Check if this is the first callback
                if(ll == NULL) {
                    // Allocate first callback
                    ll = new TimercallbackLL;
                    ll->callback = callback;
                    ll->arg = arg;
                    ll->next = NULL;
                    return;
                }
                TimercallbackLL* curr = ll;
                while(curr->next != NULL) { curr = curr->next; }
                // We have arrived at the last element of the LL
                curr->next = new TimercallbackLL;
                curr->next->callback = callback;
                curr->next->arg = arg;
                curr->next->next = NULL;
            }
        }
    }
}

#endif