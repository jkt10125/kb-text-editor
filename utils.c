void toString (char *s, int n) {
    s[4] = '\0';
    for (int i = 0; i < 4; i++) {
        s[3 - i] = n % 10 + '0';
        n /= 10;
    }
}