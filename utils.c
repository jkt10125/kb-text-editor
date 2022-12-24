void toString (char *s, int n) {
    s[4] = '\0';
    for (int i = 0; i < 4; i++) {
        s[3 - i] = n % 10 + '0';
        n /= 10;
    }
}

char pairOf(char c) {
    if (c == '{') return '}';
    if (c == '(') return ')';
    return ']';
}

int validOpeningBracket(char c) {
    return (c == '[' || c == '{' || c == '(');
}

int validClosingBracket(char c) {
    return (c == ']' || c == '}' || c == ')');
}

