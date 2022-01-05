#include <CPP/string.h>

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.

    */
char* itoa(long value, char* result, long base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    long tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

long atoi(const char* s) {
    long res = 0;
    bool neg = false;
    char* ptr = (char*)s;
    if(!ptr) { return 0; }
    if(ptr[0] == '-') { neg = true; ptr++; }
    while(*ptr) {
        // Check if we have a valid ascii number digit
        if(*ptr < 0x30 || *ptr > 0x39) { ptr++; continue; }
        res = (res * 10) + (*ptr - '0');
        ptr++;
    }
    if(neg) { res = -res; }
    return res;
}

long strlen(const char* str) {
    int size = 0;
    char* curr = (char*)str;
    while(*curr++ != '\0') {
        size++;
    }
    return size;
}

int strcmp(const char *str1, const char *str2) {
    int s1;
    int s2;
    do {
        s1 = *str1++;
        s2 = *str2++;
        if (s1 == 0)
            break;
    } while (s1 == s2);
    return (s1 < s2) ? -1 : (s1 > s2);
}