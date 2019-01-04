#include <string.h>

static void reverse(char *str) {
    size_t len = strlen(str);

    for (size_t i = 0, j = len - 1; i < j; i++, j--) {
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
}

static const char *alphabet = "0123456789abcdef";

char *uitoa(unsigned int value, char *str, int base) {
    size_t i = 0;
    do {
        str[i++] = alphabet[value % base];
        value /= base;
    } while (value);

    str[i] = 0;
    reverse(str);
    return str;
}
