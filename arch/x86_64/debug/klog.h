#ifndef KLOG_H
#define KLOG_H

namespace Kernel {
    class KLog {
    public:
        void printf(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));
        void puts(const char* str);

        static KLog& the() {
            static KLog instance;
            return instance;
        }
    private:

    };
}

#endif