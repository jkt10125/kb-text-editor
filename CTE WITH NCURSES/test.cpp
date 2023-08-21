#include <ncurses.h>

int main() {
    initscr();
    noecho();

    while (1) {
        int c = getch();
        printw("Keycode: %d\n", c);
        refresh();
    }

    endwin();
    return 0;
}