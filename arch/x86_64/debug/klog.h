#ifndef KLOG_H
#define KLOG_H

#include <CPP/vector.h>
#include <CPP/mutex.h>


namespace Kernel {
    class KLog {
    public:
        void printf(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

        void registerCallback(void (*callback)(void*, const char*), void* arg) {
            callbacks.push_back(callback);
            arguments.push_back(arg);
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