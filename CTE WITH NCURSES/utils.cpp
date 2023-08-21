#include "utils.hpp"
#include <array>
#include <cctype>
#include <iostream>
#include <unistd.h>  // Required for read() on Unix-like systems
#include <termios.h>  // Required for manipulating terminal settings
#include <errno.h>  // Required for errno and EAGAIN
#include <cstdlib>  // Required for exit() and atexit()
#include <sys/ioctl.h>  // Required for ioctl() and TIOCGWINSZ

void enableRawMode() {
    atexit(disableRawMode);
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// below is a function that reads a single readKey from the user and returns the character
// it also recognizes arrow keys and maps them to our own enum values

int readKey() {
    int c = 0;
    while (read(STDIN_FILENO, &c, 1) != 1) {
        if (errno != EAGAIN) {
            exit(1);
        }
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return '\x1b';
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return '\x1b';
        }
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    }
    return c;
}

int getWindowSize(int &row, int &col) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }

        return getCursorPosition(row, col);
    }
    else {
        col = ws.ws_col;
        row = ws.ws_row;
    }
    return 0;
}

// this function is used to get the cursor position
// it is used to position the cursor in the correct place when the user presses the arrow keys
// logic : we move the cursor to the bottom right corner of the screen and then we get the cursor position
// reason of using 32 sized char array : the cursor position is returned in the form of a string like this : "\x1b[24;80R"
int getCursorPosition(int &row, int &col) {
    std::string buf(32, 0);
    int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < 32) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1 || buf[i] == 'R') break;
        i++;
    }

    if (buf.substr(0, 2) != "\x1b[") return -1;

    if (sscanf(buf.c_str(), "\x1b[%d;%dR", &row, &col) != 2) return -1;

    return 0;
}


editorConfig::editorConfig() {
    cx = 0;
    cy = 0;
    rowoff = 0;
    coloff = 0;
    screenrows = 0;
    screencols = 0;
    numrows = 0;
    filename = "";
    statusmsg = "";
    dirty = false;
    
    getWindowSize(screenrows, screencols);
    screenrows -= 2;
}

int editorConfig::cx_to_rx(int at, int cx) {
    int curr_rx = 0, curr_len = 0;
    for (int i = 0; i < (int)row[at].size(); i++) {
        if (curr_len < cx && row[at][i] == "\t") {
            curr_rx += TAB_SIZE - (curr_rx % TAB_SIZE);
            curr_len++;
        }
        else if (curr_len + row[at][i].size() < cx) {
            curr_rx += row[at][i].size();
            curr_len += row[at][i].size();
        }
        else {
            return curr_rx + cx - curr_len;
        }
    }
    return curr_rx;
}

int editorConfig::rx_to_cx(int at, int rx) {
    int curr_rx = 0, curr_len = 0;
    for (int i = 0; i < (int)row[at].size(); i++) {
        if (row[at][i] == "\t") {
            if (curr_rx + TAB_SIZE - (curr_rx % TAB_SIZE) > rx) {
                return curr_len;
            }
            curr_rx += TAB_SIZE - (curr_rx % TAB_SIZE);
            curr_len++;
        }
        else if (curr_rx + row[at][i].size() < rx) {
            curr_rx += row[at][i].size();
            curr_len += row[at][i].size();
        }
        else {
            return curr_len + rx - curr_rx;
        }
    }
    return curr_len;
}

std::array<int, 2> editorConfig::get_idx(int at, int cx) {
    int curr_len = 0;
    for (int i = 0; i < (int)row[at].size(); i++) {
        if (row[at][i].size() + curr_len <= cx) {
            curr_len += row[at][i].size();
        }
        else {
            return {i, cx - curr_len};
        }
    }
    return {(int)row[at].size(), 0};
}

std::vector<std::string> editorConfig::process(const std::string &s) {
    std::vector<std::string> ret;
    std::string curr = "";
    for (int i = 0; i < (int)s.size(); i++) {
        if (s[i] == '\t' || s[i] == ' ') {
            if (!curr.empty()) {
                ret.push_back(curr);
                curr.clear();
            }
            ret.push_back(std::string(1, s[i]));
        }
        else {
            curr += s[i];
        }
    }
    if (!curr.empty()) {
        ret.push_back(curr);
    }
    return ret;
}

void editorConfig::insertRow(int at, const std::string &s) {
    if (at < 0 || at > numrows) return;
    row.insert(row.begin() + at, process(s));
    numrows++;
    dirty++;
}

void editorConfig::appendRow(int at1, int at2, int cx) {
    if (at1 < 0 || at1 > numrows) return;
    
    for (std::string &s : getRow(at2, cx)) {
        row[at1].push_back(s);
    }
}

std::vector<std::string> editorConfig::getRow(int at, int Idx) {
    std::vector<std::string> ret;
    if (at < 0 || at >= numrows) return ret;
    auto idx = get_idx(at, Idx);

    if (idx[0] == (int)row[at].size()) return ret;

    ret.push_back(row[at][idx[0]].substr(idx[1]));
    idx[0]++;
    while (idx[0] < (int)row[at].size()) {
        ret.push_back(row[at][idx[0]]);
        idx[0]++;
    }

    return ret;
}

void editorConfig::deleteRow(int at) {
    if (at < 0 || at >= numrows) return;
    row.erase(row.begin() + at);
    numrows--;
    dirty++;
}

void editorConfig::insertChar(int at, int cx, int c) {
    if (at < 0 || at >= numrows) return;
    auto idx = get_idx(at, cx);

    if (c == ' ' || c == '\t') {
        if (idx[1] == 0) {
            row[at].insert(row[at].begin() + idx[0], std::string(1, c));
        }
        else {
            std::string tmp = row[at][idx[0]].substr(idx[1]);
            row[at][idx[0]].erase(idx[1]);
            row[at].insert(row[at].begin() + idx[0] + 1, tmp);
            row[at].insert(row[at].begin() + idx[0] + 1, std::string(1, c));
        }
    }

    else {
        if (idx[0] == (int)row[at].size()) {
            row[at].push_back(std::string(1, c));
        }

        else {
            if (row[at][idx[0]] == " " || row[at][idx[0]] == "\t") {
                row[at].insert(row[at].begin() + idx[0], std::string(1, c));
            }
            else {
                row[at][idx[0]].insert(row[at][idx[0]].begin() + idx[1], c);
            }
        }
    }
}

void editorConfig::deleteChar(int at, int cx) {
    if (at < 0 || at >= numrows) return;
    auto idx = get_idx(at, cx);

    if (idx[0] == (int)row[at].size()) return; // this means we are merging two rows

    if (idx[1] == (int)row[at][idx[0]].size()) { // case not possible 
        std::cerr << "logic of get_idx() is wrong\n";
        return;
    }
    else {
        if (row[at][idx[0]] == " " || row[at][idx[0]] == "\t") {
            row[at].erase(row[at].begin() + idx[0]);
            if (idx[0] == 0 || idx[0] == (int)row[at].size()) {
                
            }
            else {
                if (row[at][idx[0] - 1] != " " && row[at][idx[0] - 1] != "\t") {
                    if (row[at][idx[0]] != " " && row[at][idx[0]] != "\t") {
                        row[at][idx[0] - 1].append(row[at][idx[0]]);
                        row[at].erase(row[at].begin() + idx[0]);
                    }
                }
                else {
                    row[at][idx[0] - 1].append(row[at][idx[0]]);
                    row[at].erase(row[at].begin() + idx[0]);
                }
            }
        }
        else {
            row[at][idx[0]].erase(row[at][idx[0]].begin() + idx[1]);
            if (row[at][idx[0]].empty()) {
                row[at].erase(row[at].begin() + idx[0]);
            }
        }
    }
}

void editorConfig::scroll() {
    rx = 0;
    if (cy < numrows) {
        rx = cx_to_rx(cy, cx);
    }

    if (cy < rowoff) {
        rowoff = cy;
    }

    if (cy >= rowoff + screenrows) {
        rowoff = cy - screenrows + 1;
    }

    if (rx < coloff) {
        coloff = rx;
    }

    if (rx >= coloff + screencols) {
        coloff = rx - screencols + 1;
    }
}

void editorConfig::drawRows(std::string &s) {
    // draw welcome message if no file is opened
    for (int y = 0; y < screenrows; y++) {
        int filerow = y + rowoff;
        if (filerow >= numrows) {

        }

        else {
            std::string render;
            for (std::string &s : getRow(filerow, coloff)) {
                if (s == "\t") {
                    render += " ";
                    while (render.size() % TAB_SIZE != 0) {
                        render += " ";
                    }
                }
                render += s;
            }
            int len = std::min(std::max(0, (int)render.size() - coloff), screencols);
            for (int i = 0; i < len; i++) {
                if (iscntrl(render[i])) {
                    char c = (render[i] <= 26) ? '@' + render[i] : '?';
                    s += "\x1b[7m" + std::string(1, c) + "\x1b[m";
                }
            }
            s += render.substr(0, len);
        }
        s += "\x1b[K\r\n";
    }
}

std::string editorConfig::formatStatus() {
    std::string status = filename.empty() ? "[No Name]" : filename;
    status = status.substr(0, 20) + " - " + std::to_string(numrows) + " lines";
    if (dirty) {
        status += " (modified)";
    }
    return status;
}

std::string editorConfig::formatrStatus() {
    std::string rstatus = std::to_string(cy + 1) + " | " + std::to_string(cx + 1);
    return rstatus;
}

void editorConfig::drawStatusBar(std::string &s) {
    s += "\x1b[7m";
    std::string status = formatStatus();
    std::string rstatus = formatrStatus();
    int len = std::min((int)status.size(), screencols);

    s += status.substr(0, len);
    
    while (len < screencols) {
        if (screencols - len == (int)rstatus.size()) {
            s += rstatus;
            break;
        }
        else {
            s += " ";
            len++;
        }
    }
    s += "\x1b[m\r\n";
}

void editorConfig::drawMessageBar(std::string &s) {
    s += "\x1b[K";
    int msglen = std::min((int)statusmsg.size(), screencols);
    if (msglen && time(NULL) - statusmsg_time < 5) {
        s += statusmsg.substr(0, msglen);
    }
}

void editorConfig::refreshScreen() {
    std::cout << "hehe";
    scroll();
    std::string s;
    s += "\x1b[?25l";
    s += "\x1b[H";

    drawRows(s);
    drawStatusBar(s);
    drawMessageBar(s);

    s += "\x1b[" + std::to_string(cy - rowoff + 1) + ";" + std::to_string(rx - coloff + 1) + "H";
    s += "\x1b[?25h";
    write(STDOUT_FILENO, s.c_str(), s.size());
}

void editorConfig::setStatusMessage(const std::string &s) {
    statusmsg = s;
    statusmsg_time = time(NULL);
}

void editorConfig::moveCursor(int key) {
    int filerow = cy + rowoff;
    int filecol = cx + coloff;
    int rowlen = row[filerow].size();

    switch (key) {
    case ARROW_LEFT:
        if (cx != 0) {
            cx--;
        }
        else if (cy > 0) {
            cy--;
            cx = row[cy + rowoff].size();
        }
        break;
    case ARROW_RIGHT:
        if (cx < rowlen) {
            cx++;
        }
        else if (cy < numrows) {
            cy++;
            cx = 0;
        }
        break;
    case ARROW_UP:
        if (cy != 0) {
            cy--;
        }
        break;
    case ARROW_DOWN:
        if (cy < numrows) {
            cy++;
        }
        break;
    }

    filerow = cy + rowoff;
    filecol = cx + coloff;
    rowlen = row[filerow].size();
    if (filecol > rowlen) {
        cx = rowlen;
    }
}

void editorConfig::processKeypress() {

    int quit_times = QUIT_TIMES;

    int c = readKey();
    switch (c) {
    case CTRL_KEY('q'):
        if (dirty && quit_times > 0) {
            setStatusMessage("WARNING!!! File has unsaved changes. Press Ctrl-Q " + std::to_string(quit_times) + " more times to quit.");
            quit_times--;
            return;
        }
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    case CTRL_KEY('s'):
        // save();
        break;
    case HOME_KEY:
        cx = 0;
        break;
    case END_KEY:
        if (cy < numrows) {
            cx = row[cy + rowoff].size();
        }
        break;
    case CTRL_KEY('f'):
        // find();
        break;
    // case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (c == DEL_KEY) {
            moveCursor(ARROW_RIGHT);
        }
        // delChar();
        break;
    case PAGE_UP:
    case PAGE_DOWN:
        {
            if (c == PAGE_UP) {
                cy = rowoff;
            }
            else if (c == PAGE_DOWN) {
                cy = rowoff + screenrows - 1;
                if (cy > numrows) {
                    cy = numrows;
                }
            }
            int times = screenrows;
            while (times--) {
                moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }
        }
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        moveCursor(c);
        break;
    case CTRL_KEY('l'):
    case '\x1b':
        break;
    default:
        // inserts at the current position of the cursor
        insertChar(cy, cx, c);
        break;
    }
}