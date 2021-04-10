// A file with a shit ton of stubs atm

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

extern "C" void *__dso_handle = 0;
 
extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso) { return 0; }
extern "C" void __cxa_finalize(void *f) { }

#pragma GCC diagnostic pop