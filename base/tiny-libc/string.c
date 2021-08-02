#include <string.h>

size_t strlen(const char* s) {
    size_t ret = 0;
    for (ret = 0; s[ret]; ret++);
    return ret;
}