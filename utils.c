#include <stdio.h>

void swap (char *a, char *b) {
    char *c = a;
    *a = *b;
    *b = *c;
}

void toString (char *s, int n) {
    int rem, i;
    s[4] = '\0';
    for (i = 0; i < 4; i++) {
        rem = n % 10;
        n /= 10;
        s[3 - i] = rem + '0';
    }
}