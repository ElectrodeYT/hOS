#ifndef ERRNO_H
#define ERRNO_H

enum {
    ENOERR = 0,
    EINVAL,     // Invalid argument
    EINVSZ,         // Invalid size
    EEXIST,
    EWOULDBLOCK,
    EPERM,
    ENOENT,
    ENUMERORS       // Number of valid errors
};


#endif