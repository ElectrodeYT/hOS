#include <string.h>

int strlen(char* s) {
    int ret = 0;
    for (ret = 0; s[ret]; ret++);
    return ret;
}