#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <termios.h>
#include <vector>
#include <array>
#include <ncurses.h>

#define CTRL_KEY(k) ((k) & 0x1f)

const int TAB_SIZE = 4;
const int QUIT_TIMES = 3;
const int EXPAND_TAB = 1;

class editorConfig {

    int cx_to_rx(int at, int cx);

    int rx_to_cx(int at, int rx);

    std::array<int, 2> get_idx(int at, int cx);
    
    std::string formatStatus();

    std::string formatrStatus();

public:
    int cx, cy; // for cursor position
    int screenrows; // for screen size
    int screencols; // for screen size
    int numrows; // for number of rows
    std::vector<std::vector<std::string>> row; // for rows
    int rowoff; // for row offset
    int coloff; // for column offset
    int rx; // for render index
    int dirty; // for unsaved changes
    std::string filename, statusmsg; // for file name and status message
    time_t statusmsg_time; // for status message time
    std::string append_buffer; // for append buffer
    struct termios orig_termios; // for terminal settings

    editorConfig();

    void insertRow(int at, const std::string &s);

    void appendRow(int at_src, int at_dest, int cx);

    void deleteRow(int at);

    void insertChar(int at, int idx, int c);

    void deleteChar(int at, int idx);

    std::vector<std::string> getRow(int at, int idx);

    std::vector<std::string> process(const std::string &s);

    void scroll();

    void drawRows(std::string &s);

    void drawStatusBar(std::string &s);

    void drawMessageBar(std::string &s);

    void refreshScreen();

    void moveCursor(int key);

    void processKeypress();

    void setStatusMessage(const std::string &s);
};

int readKey();

int getWindowSize(int &row, int &col); // for window size
int getCursorPosition(int &row, int &col); // for cursor position

// int cx_to_rx(const erow &row, int cx);
// int rx_to_cx(const erow &row, int rx);

void enableRawMode();
void disableRawMode();

#endif