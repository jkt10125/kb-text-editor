#include <ncurses.h>
#include <vector>
#include <fstream>
#include <functional>
#include <string>

#define CTRL_KEY(k) ((k) & 0x1f)

std::vector<std::string> editorContent;
std::vector<int> trailSpaces;
std::string filename = "";

int cursorX, cursorY;

// these two variables are used to keep track of the topmost visible line and the leftmost visible character in the editor
int extremeX, extremeY;

const int TAB_SIZE = 4;
const int QUIT_TIMES = 2;
const int INDENT_WITH_TABS = 1;

const int LEFT_SPACING = 10;
const int RIGHT_SPACING = 1;
const int TOP_SPACING = 3;
const int BOTTOM_SPACING = 3;

struct Boundary {
    int top, bottom, right, left;
} editorBoundary;

struct Dim {
    int height, width;
} Window;

void setDimensions() {
    getmaxyx(stdscr, Window.height, Window.width);
    editorBoundary.top = TOP_SPACING;
    editorBoundary.bottom = Window.height - BOTTOM_SPACING - 1;
    editorBoundary.left = LEFT_SPACING;
    editorBoundary.right = Window.width - RIGHT_SPACING - 1;
}

void drawEditorBox() {
    box(stdscr, 0, 0);

    mvhline(editorBoundary.top - 1, 1, ACS_HLINE, Window.width - 2);
    mvhline(editorBoundary.bottom + 1, 1, ACS_HLINE, Window.width - 2);
    mvvline(editorBoundary.top, editorBoundary.left - 1, ACS_VLINE, editorBoundary.bottom - editorBoundary.top + 1);
}

void refreshEditor(const std::pair<int, int> &lineOffset) {
    for (int i = lineOffset.first; i <= lineOffset.second; i++) {
        move(i, 1);
        clrtoeol();
        
        int currentLineIdx = i + extremeY - editorBoundary.top;
        if (currentLineIdx < editorContent.size()) {
            std::string num_idx = std::to_string(currentLineIdx + 1);
            move(i, LEFT_SPACING - 2 - num_idx.size());
            for (char ch : num_idx) addch(ch);

            move(i, editorBoundary.left - 1);
            addch(ACS_VLINE);
            if (extremeY + i < (int)editorContent.size() + editorBoundary.top) {
                for (int j = 0; j <= editorBoundary.right - editorBoundary.left; j++) {
                    if (j + extremeX < (int)editorContent[extremeY + i - editorBoundary.top].size()) {                    
                        addch(editorContent[extremeY + i - editorBoundary.top][j + extremeX]);
                    }
                    else break;
                }
            }
        } 

        else {
            
            move(i, LEFT_SPACING - 3);
            addch('~');
            move(i, editorBoundary.left - 1);
            addch(ACS_VLINE);
        }

        mvvline(i, editorBoundary.right + 1, ACS_VLINE, 1);
    }
    refresh();
    move(cursorY, cursorX);
}

void refreshStatus() {
    move(editorBoundary.bottom + 2, 1);
    clrtoeol();
    std::string coordinateStatus = std::string("Spaces: ") + std::to_string(TAB_SIZE) + std::string(" | Ln: ") + std::to_string(cursorY + extremeY - editorBoundary.top + 1) + ", Col: " + std::to_string(cursorX + extremeX - editorBoundary.left + 1);
    mvprintw(editorBoundary.bottom + 2, editorBoundary.right - coordinateStatus.size() - 2, "%s", coordinateStatus.c_str());
    mvvline(editorBoundary.bottom + 2, editorBoundary.right + 1, ACS_VLINE, 1);
    move(cursorY, cursorX);
    refresh();
}

void scrollHandler(int c) {
    if (c == KEY_UP) {
        if (cursorY > editorBoundary.top) {
            cursorY--;
            if (cursorX + extremeX > editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
                if ((int)editorContent[extremeY + cursorY - editorBoundary.top].size() < extremeX) {
                    cursorX = std::min(editorBoundary.right, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left);
                    
                    if (extremeX != std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left))) {
                        extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left));
                        refreshEditor({editorBoundary.top, editorBoundary.bottom});
                    }
                }
                else {
                    cursorX = editorContent[extremeY + cursorY - editorBoundary.top].size() - extremeX + editorBoundary.left;
                }
            }
        }
        else {
            if (extremeY > 0) {
                extremeY--;
                if (cursorX + extremeX > editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
                    if ((int)editorContent[extremeY + cursorY - editorBoundary.top].size() < extremeX) {
                        cursorX = std::min(editorBoundary.right, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left);
                    
                        if (extremeX != std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left))) {
                            extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left));
                        }
                    }
                    else {
                        cursorX = editorContent[extremeY + cursorY - editorBoundary.top].size() - extremeX + editorBoundary.left;
                    }
                }
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
            else {
                cursorX = editorBoundary.left;
                if (extremeX > 0) {
                    extremeX = 0;
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
            }
        }
    }
    else if (c == KEY_DOWN) {
        if (extremeY + cursorY - editorBoundary.top < (int)editorContent.size() - 1) {
            if (cursorY < editorBoundary.bottom) {
                cursorY++;
                if (cursorX + extremeX > editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
                    if (cursorX + extremeX > editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
                        if ((int)editorContent[extremeY + cursorY - editorBoundary.top].size() < extremeX) {
                            cursorX = std::min(editorBoundary.right, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left);
                        
                            if (extremeX != std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left))) {
                                extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left));
                                refreshEditor({editorBoundary.top, editorBoundary.bottom});
                            }
                        }
                        else {
                            cursorX = editorContent[extremeY + cursorY - editorBoundary.top].size() - extremeX + editorBoundary.left;
                        }
                    }
                }
            }
            else {
                extremeY++;
                if (cursorX + extremeX > editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
                    if ((int)editorContent[extremeY + cursorY - editorBoundary.top].size() < extremeX) {
                        cursorX = std::min(editorBoundary.right, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left);
                        extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left));
                    }
                    else {
                        cursorX = editorContent[extremeY + cursorY - editorBoundary.top].size() - extremeX + editorBoundary.left;
                    }
                }
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
        }
        else {
            cursorX = std::min(editorBoundary.right, (int)editorContent.back().size() + editorBoundary.left);

            if (extremeX != std::max(0, (int)editorContent.back().size() - (editorBoundary.right - editorBoundary.left))) {
                extremeX = std::max(0, (int)editorContent.back().size() - (editorBoundary.right - editorBoundary.left));
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
        }
    }
    else if (c == KEY_LEFT) {
        if (cursorX > editorBoundary.left) {
            cursorX--;
        }
        else {
            if (extremeX > 0) {
                extremeX--;
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
            else {
                cursorX = std::min((int)editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left, editorBoundary.right);
                if (extremeX != std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left))) {
                    extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - (editorBoundary.right - editorBoundary.left));
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
            }
        }
    }
    else if (c == KEY_RIGHT) {
        if (cursorX < std::min(editorBoundary.right, (int)editorContent[extremeY + cursorY - editorBoundary.top].size() - extremeX + editorBoundary.left)) {
            cursorX++;
        }
        else {
            if (extremeX + editorBoundary.right - editorBoundary.left < editorContent[extremeY + cursorY - editorBoundary.top].size()) {
                extremeX++;
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
            else {
                cursorX = editorBoundary.left;
                if (extremeX > 0) {
                    extremeX = 0;
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
            }
        }
    }
    refreshStatus();
}

void calcTrailingSpaces(int currLine) {
    while (trailSpaces[currLine] < editorContent[currLine].size()) {
        if (editorContent[currLine][trailSpaces[currLine]] != ' ') {
            break;
        }
        trailSpaces[currLine]++;
    }
}

void deleteHandler(bool isBackspace) {
    if (isBackspace) {
        if (cursorX + extremeX > editorBoundary.left) {
            if (trailSpaces[extremeY + cursorY - editorBoundary.top] >= extremeX + cursorX - editorBoundary.left) {
                int qty = (extremeX + cursorX - editorBoundary.left) % TAB_SIZE;
                if (qty == 0) qty = TAB_SIZE;

                trailSpaces[extremeY + cursorY - editorBoundary.top] -= qty;
                editorContent[extremeY + cursorY - editorBoundary.top].erase(extremeX + cursorX - editorBoundary.left - qty, qty);
                
                cursorX -= qty;
                if (cursorX < editorBoundary.left) {
                    extremeX -= editorBoundary.left - cursorX;
                    cursorX = editorBoundary.left;
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
                else {
                    refreshEditor({cursorY, cursorY});
                }
            }
            else {
                editorContent[extremeY + cursorY - editorBoundary.top].erase(extremeX + cursorX - editorBoundary.left - 1, 1);

                calcTrailingSpaces(extremeY + cursorY - editorBoundary.top);

                if (cursorX > editorBoundary.left) {
                    cursorX--;
                    refreshEditor({cursorY, cursorY});
                }
                else {
                    extremeX--;
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
            }
        }
        else if (cursorY + extremeY > editorBoundary.top) {
                        
            cursorX = std::min((int)editorContent[extremeY + cursorY - editorBoundary.top - 1].size() + editorBoundary.left, editorBoundary.right);
            extremeX = std::max(0, (int)editorContent[extremeY + cursorY - editorBoundary.top - 1].size() - (editorBoundary.right - editorBoundary.left));
            
            editorContent[extremeY + cursorY - editorBoundary.top - 1] += editorContent[extremeY + cursorY - editorBoundary.top];
            if (trailSpaces[extremeY + cursorY - editorBoundary.top - 1] == extremeX + cursorX - editorBoundary.left) {
                trailSpaces[extremeY + cursorY - editorBoundary.top - 1] += trailSpaces[extremeY + cursorY - editorBoundary.top];
            }

            editorContent.erase(editorContent.begin() + extremeY + cursorY - editorBoundary.top);
            trailSpaces.erase(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top);

            if (cursorY > editorBoundary.top) {
                cursorY--;
                refreshEditor({cursorY, editorBoundary.bottom});
            }
            else {
                extremeY--;
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
        }
    }

    else {
        if (extremeX + cursorX - editorBoundary.left < editorContent[extremeY + cursorY - editorBoundary.top].size()) {
            editorContent[extremeY + cursorY - editorBoundary.top].erase(extremeX + cursorX - editorBoundary.left, 1);
            if (trailSpaces[extremeY + cursorY - editorBoundary.top] > extremeX + cursorX - editorBoundary.left) {
                trailSpaces[extremeY + cursorY - editorBoundary.top]--;
            }
            else {
                calcTrailingSpaces(extremeY + cursorY - editorBoundary.top);
            }
            refreshEditor({cursorY, cursorY});
        }
        else if (extremeY + cursorY - editorBoundary.top + 1 < editorContent.size()) {
            editorContent[extremeY + cursorY - editorBoundary.top] += editorContent[extremeY + cursorY - editorBoundary.top + 1];
            editorContent.erase(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1);
            if (trailSpaces[extremeY + cursorY - editorBoundary.top] >= extremeX + cursorX - editorBoundary.left) {
                trailSpaces[extremeY + cursorY - editorBoundary.top] += trailSpaces[extremeY + cursorY - editorBoundary.top + 1];
            }
            refreshEditor({cursorY, editorBoundary.bottom});
        }
    }
    refreshStatus();
}

void insertCharHandler(char c) {
    if (c == ' ') {
        if (trailSpaces[extremeY + cursorY - editorBoundary.top] >= extremeX + cursorX - editorBoundary.left) {
            trailSpaces[extremeY + cursorY - editorBoundary.top]++;
        }
    }
    else {
        if (trailSpaces[extremeY + cursorY - editorBoundary.top] > extremeX + cursorX - editorBoundary.left) {
            trailSpaces[extremeY + cursorY - editorBoundary.top] = extremeX + cursorX - editorBoundary.left;
        }
    }
    editorContent[extremeY + cursorY - editorBoundary.top].insert(extremeX + cursorX - editorBoundary.left, 1, c);
    if (cursorX < editorBoundary.right) {
        cursorX++;
        refreshEditor({cursorY, cursorY});
    }
    else {
        extremeX++;
        refreshEditor({editorBoundary.top, editorBoundary.bottom});
    }
    refreshStatus();
}

void tabspaceHandler() {
    insertCharHandler(' ');
    while ((cursorX + extremeX - editorBoundary.left) % TAB_SIZE != 0) {
        insertCharHandler(' ');
    }
}

void parenthesisHandler(char c, bool AutoParenthesis) {

    std::function<char(char)> MatchingPair = [&] (char c) {
        if (c == '[') return ']';
        if (c == '{') return '}';
        if (c == '(') return ')';
        return '?';
    };

    if (AutoParenthesis) {
        if (MatchingPair(c) == '?') {
            if (editorContent[extremeY + cursorY - editorBoundary.top][extremeX + cursorX - editorBoundary.left] == c) {
                if (cursorX < editorBoundary.right) {
                    cursorX++;
                    refreshEditor({cursorY, cursorY});
                }
                else {
                    extremeX++;
                    refreshEditor({editorBoundary.top, editorBoundary.bottom});
                }
            }
            else {
                insertCharHandler(c);
            }
        }
        else {
            insertCharHandler(c);
            insertCharHandler(MatchingPair(c));
            cursorX--;
        }
    }
    else {
        insertCharHandler(c);
    }
}

void newlineHandler(bool AutoIndent) {
    int newLineTrailingSpaceQty = 0;
    if (cursorX + extremeX == editorContent[extremeY + cursorY - editorBoundary.top].size() + editorBoundary.left) {
        if (AutoIndent && !editorContent[extremeY + cursorY - editorBoundary.top].empty()) {
            newLineTrailingSpaceQty = trailSpaces[extremeY + cursorY - editorBoundary.top];
            char c = editorContent[extremeY + cursorY - editorBoundary.top].back();
            if (c == '{' || c == '[' || c == '(') {
                newLineTrailingSpaceQty += TAB_SIZE;
            }
        }

        editorContent.insert(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, std::string(newLineTrailingSpaceQty, ' '));
        trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, newLineTrailingSpaceQty);
        
        cursorX = editorBoundary.left + newLineTrailingSpaceQty;
        extremeX = 0;

        if (cursorX > editorBoundary.right) {
            extremeX += cursorX - editorBoundary.right;
            cursorX = editorBoundary.right;
        }

        if (cursorY < editorBoundary.bottom) {
            cursorY++;
        }
        else {
            extremeY++;
        }
        refreshEditor({editorBoundary.top, editorBoundary.bottom});
    }

    else {
        std::string &currentLine = editorContent[extremeY + cursorY - editorBoundary.top];
        std::string nextLine = currentLine.substr(cursorX + extremeX - editorBoundary.left);
        currentLine.erase(cursorX + extremeX - editorBoundary.left);
        trailSpaces[extremeY + cursorY - editorBoundary.top] = std::min(trailSpaces[extremeY + cursorY - editorBoundary.top], (int)currentLine.size());
        
        if (AutoIndent) {
            newLineTrailingSpaceQty = trailSpaces[extremeY + cursorY - editorBoundary.top];
            char c = editorContent[extremeY + cursorY - editorBoundary.top].back();
            if (c == '{' || c == '[' || c == '(') {
                newLineTrailingSpaceQty += TAB_SIZE;
                
                std::function<bool(char, char)> isMatchingPair = [&] (char a, char b) {
                    if (a == '{' && b == '}') return true;
                    if (a == '(' && b == ')') return true;
                    if (a == '[' && b == ']') return true;
                    return false;
                };

                if (isMatchingPair(c, nextLine[0])) {
                    editorContent.insert(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, std::string(newLineTrailingSpaceQty - TAB_SIZE, ' ') + nextLine);
                    trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, newLineTrailingSpaceQty - TAB_SIZE);

                    calcTrailingSpaces(extremeY + cursorY - editorBoundary.top + 1);

                    editorContent.insert(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, std::string(newLineTrailingSpaceQty, ' '));
                    trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, newLineTrailingSpaceQty);

                }
                else {
                    editorContent.emplace(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, std::string(newLineTrailingSpaceQty, ' ') + nextLine);
                    trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, newLineTrailingSpaceQty);

                    calcTrailingSpaces(extremeY + cursorY - editorBoundary.top + 1);
                }

                cursorX = editorBoundary.left + newLineTrailingSpaceQty;
                extremeX = 0;

                if (cursorX > editorBoundary.right) {
                    extremeX += cursorX - editorBoundary.right;
                    cursorX = editorBoundary.right;
                }

                if (cursorY < editorBoundary.bottom) {
                    cursorY++;
                }
                else {
                    extremeY++;
                }
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
            else {
                editorContent.emplace(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, std::string(newLineTrailingSpaceQty, ' ') + nextLine);
                trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, newLineTrailingSpaceQty);

                calcTrailingSpaces(extremeY + cursorY - editorBoundary.top + 1);

                cursorX = editorBoundary.left + newLineTrailingSpaceQty;
                extremeX = 0;

                if (cursorX > editorBoundary.right) {
                    extremeX += cursorX - editorBoundary.right;
                    cursorX = editorBoundary.right;
                }

                if (cursorY < editorBoundary.bottom) {
                    cursorY++;
                }
                else {
                    extremeY++;
                }
                refreshEditor({editorBoundary.top, editorBoundary.bottom});
            }
        }
        else {
            editorContent.emplace(editorContent.begin() + extremeY + cursorY - editorBoundary.top + 1, nextLine);
            trailSpaces.insert(trailSpaces.begin() + extremeY + cursorY - editorBoundary.top + 1, 0);

            calcTrailingSpaces(extremeY + cursorY - editorBoundary.top + 1);
            
            cursorX = editorBoundary.left;
            extremeX = 0;

            if (cursorY < editorBoundary.bottom) {
                cursorY++;
            }
            else {
                extremeY++;
            }
            refreshEditor({editorBoundary.top, editorBoundary.bottom});
        }
    }
    refreshStatus();
}

void processKeypress() {

    int c = getch();

    if (c == KEY_RESIZE) {
        setDimensions();
        clear();
        drawEditorBox();
        refreshEditor({editorBoundary.top, editorBoundary.bottom});
        refreshStatus();
        return;
    }
    if (c == 27) {
        c = getch();
        if (c == 91) {
            c = getch();
            switch (c) {
                case 65:
                    scrollHandler(KEY_UP);
                    break;
            
                case 66:
                    scrollHandler(KEY_DOWN);
                    break;
            
                case 67:
                    scrollHandler(KEY_RIGHT);
                    break;
            
                case 68:
                    scrollHandler(KEY_LEFT);
                    break;

                case 51:
                    c = getch();
                    if (c == 126) {
                        deleteHandler(false);
                    }
                    break;
            
                default:
                    break;
            }
        }
        else if (c == 113) {
            exit(0);
        }
    }
    else {
        switch (c) {
            case 127:
                deleteHandler(true);
                break;
            
            case 10:  
            case KEY_ENTER:
                newlineHandler(true);
                break;
            
            case 9:
                tabspaceHandler();
                break;

            case '{':
            case '[':
            case '(':
            case ')':
            case ']':
            case '}':
                parenthesisHandler(c, true);
                break;
            
            default:
                insertCharHandler(c);
                break;
        }
    }

}

// void processKeypress() {
//     int c = getch();
//     // check if c is a special key
//     switch (c) {
//         case KEY_UP:
//         case KEY_DOWN:
//         case KEY_LEFT:
//         case KEY_RIGHT:
//             scrollHandler(c);
//             break;
        
//         case KEY_BACKSPACE:
//             deleteHandler(true);
//             break;
        
//         case KEY_DC:
//             deleteHandler(false);
//             break;
        
//         case '\n':
//         case KEY_ENTER:
//             newlineHandler(true);
//             break;
        
//         case KEY_RESIZE:
//             setDimensions();
//             clear();
//             drawEditorBox();
//             refreshEditor({editorBoundary.top, editorBoundary.bottom});
//             refreshStatus();
//             break;
        
//         case CTRL_KEY('q'):
//             exit(0);
        
//         case '\t':
//             tabspaceHandler();
//             break;
        
//         case '{':
//         case '[':
//         case '(':
//         case ')':
//         case ']':
//         case '}':
//             parenthesisHandler(c, true);
//             break;

//         default:
//             insertCharHandler(c);
//             break;
//     }
// }

void fileReader() {
    refreshStatus();
    if (filename.empty()) {
        editorContent.push_back("");
        trailSpaces.push_back(0);
        return;
    }
    std::fstream file(filename, std::ios::in);
    std::string line;
    while (std::getline(file, line)) {
        editorContent.push_back(line);
        trailSpaces.push_back(0);
        calcTrailingSpaces(editorContent.size() - 1);
    }
    file.close();
}

void fileWriter() {
    if (filename.empty()) return;
    std::fstream file(filename, std::ios::out | std::ios::trunc);
    for (std::string &line : editorContent) {
        file << line << '\n';
    }
    file.close();
}

void Endwin() { endwin(); }

void init() {
    atexit(Endwin);
    initscr();
    noecho();
    cbreak();
    setDimensions();
    cursorX = editorBoundary.left;
    cursorY = editorBoundary.top;
    extremeX = 0;
    extremeY = 0;
    drawEditorBox();
    // keypad(stdscr, FALSE);
}

int main(int argc, char *argv[]) {

    init();

    if (argc > 1) filename = argv[1];

    fileReader();

    refreshEditor({editorBoundary.top, editorBoundary.bottom});

    while (1) {
        move(cursorY, cursorX);
        processKeypress();
        refresh();
        fileWriter();
    }



    // endwin();
}
