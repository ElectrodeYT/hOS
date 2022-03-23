#ifndef KLOG_H
#define KLOG_H

#include <CPP/vector.h>
#include <CPP/mutex.h>


namespace Kernel {
    class KLog {
    public:
        void printf(const char* fmt, ...);
        void userDebug(const char* str);
        void registerCallback(void (*callback)(void*, const char*), void* arg) {
            acquire(&mutex);
            callbacks.push_back(callback);
            arguments.push_back(arg);
            release(&mutex);
        }

        void deregisterCallback(void (*callback)(void*, const char*)) {
            acquire(&mutex);
            for(size_t i = 0; i < callbacks.size(); i++) {
                if(callbacks.at(i) == callback) {
                    callbacks.remove(i);
                    arguments.remove(i);
                }
            }
            release(&mutex);
        }

        static KLog& the() {
            static KLog instance;
            return instance;
        }
    private:
        void puts(const char* str);
        // Mutex for printf
        mutex_t mutex;

        // List of drivers we have to emit to
        Vector<void (*)(void*, const char*)> callbacks;
        Vector<void*> arguments;
    };
}

#endif