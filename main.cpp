#include <cstddef>
#include <cstring>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <tinyxml2.h>
#include <ctime>
#include <sys/stat.h>

// This file is such a horrible mish-mash of C and C++, try not to think about it too much
// Also: memory management? I hardly know 'er!

using namespace std;
using namespace tinyxml2;

#define currentMenu currentMenus[currentMenuId]

bool fileExists(string name) {
    struct stat buffer;
    return stat(name.c_str(), &buffer) == 0;
}

struct Menu {
    string name;
    const char** labels;
    int numLabels;
    void (*enterFunctions[])(Menu*);
};

extern Menu mainMenu;
extern Menu createFeedMenu;
extern Menu createItemMenu;
extern Menu editingFeedMenu;
extern Menu editingFeedDataMenu;
extern Menu editingItemsMenu;
void setupScreen(bool drawsTitle = true);
extern void borderScreen();
extern void GoToEditSingleItem(Menu* _cm);

XMLDocument* masterDoc   = NULL;
XMLNode*     mainChannel = NULL;

int currentMenuId = 0;
int chr;
int currentHighlight = 0;

bool shouldQuitApp = false;
bool isSaving      = true;

Menu** currentMenus = (Menu**)malloc(sizeof(Menu) * 16);

char* fileName = new char[80];
char* feedName = new char[200];
char* feedLink = new char[300];
char* feedDesc = new char[1000];

char* itemName = new char[80];
char* itemLink = new char[300];
char* itemDesc = new char[20000];

const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char* months[] = {"Jan", "Feb", "Mar", "May", "Apr", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char* getRFC822Date() {
    time_t now = std::time(nullptr);
    tm *ltm = std::gmtime(&now);

    char* buffer = (char*)malloc(sizeof(char) * 100);
    //strftime(buffer, sizeof(buffer), "%d, %d %d %d %H:%M:%S GMT", days[ltm->tm_wday], ltm->tm_mday);
    //strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", ltm);
    strftime(buffer, 100, "%a, %d %b %Y %H:%M:%S GMT", ltm);
    
    return buffer;
}

void ReadLine(char* chrl, int maxl) {
    echo();
    curs_set(1);

    int index = 0;
    
    // while(index < maxl-1) {
    //     chr = getch();
    //     if(chr == KEY_LEFT) {
    //         index = max(0, --index);
    //     } else if(chr == KEY_RIGHT) {
    //         if(chrl[index+1] != '\0') index++;
    //     } else if(chr == 27 || chr == 10) {
    //         break;
    //     } else if (chr == KEY_BACKSPACE || chr == 127) {
    //         if(index > 0) {
    //             index--;
    //             chrl[index] = '\0';
    //             printw("\b \b");
    //         }
    //     } else if(chr == KEY_ENTER) {
    //         break;
    //     } else if(chr != ERR) {
    //         chrl[index++] = chr;
    //         chrl[index] = '\0';
    //         printw("%c", chr);
    //     }
    // }

    getnstr(chrl, maxl);

    noecho();
    curs_set(0);
}

void ReadLine(char* chrl) {
    ReadLine(chrl, 79);
}

void drawOptions() {
    for(int i = 0; i < currentMenu->numLabels; i++) {
        printw("   - ");
        if(i == currentHighlight) {
            attron(A_REVERSE);
            printw("%s", currentMenu->labels[i]);
            attroff(A_REVERSE);
        } else {
            printw("%s", currentMenu->labels[i]);
        }
        printw("\n");
    }
}

void ReturnToPreviousMenu(Menu* _cm) {
    currentHighlight = 0;
    if(currentMenuId == 0) {
        shouldQuitApp = true;
    } else {
        currentMenuId--;
    }
}

void QuitApp(Menu* _cm) {
    shouldQuitApp = true;
}

void QuitSansSaving(Menu* _cm) {
    isSaving = false;
    shouldQuitApp = true;
}

void goToEditFeed(Menu* _cm) {
    currentMenus[currentMenuId] = &editingFeedMenu;

    while(true) {
        setupScreen();
        printw("    Editing \"%s\" (%s)\n\n", feedName, fileName);
        drawOptions();
        borderScreen();

        chr = getch();
        if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = currentMenu->numLabels - 1;
        } else if(chr == KEY_DOWN) {
            if(++currentHighlight >= currentMenu->numLabels) currentHighlight = 0;
        } else if(chr == 10) { // Subs for KEY_ENTER
            if(currentMenu->enterFunctions[currentHighlight] != nullptr) {
                currentMenu->enterFunctions[currentHighlight](currentMenu);
            }
        }
        if(shouldQuitApp) break;
    }
}

void GoToCreateItem(Menu* _cm) {
    currentMenuId++;
    currentMenus[currentMenuId] = &createItemMenu;

    if(!mainChannel) {
        setupScreen();
        printw("    Just prevented segfault! ");
        borderScreen();
        getch();
        ReturnToPreviousMenu(nullptr);
        return;
    }

    XMLNode* item = masterDoc->NewElement("item");
    mainChannel->InsertEndChild(item);

    setupScreen();
    printw("    Enter the item name (\\cancel to cancel): ");
    borderScreen();
    
    do {ReadLine(itemName, 299);} while(strlen(itemName) == 0);

    if(strcmp(itemName, "\\cancel") == 0) {
        ReturnToPreviousMenu(nullptr);
        mainChannel->DeleteChild(item);
        return;
    }

    XMLNode* nameNode = masterDoc->NewElement("title");
    nameNode->InsertFirstChild(masterDoc->NewText(itemName));
    item->InsertEndChild(nameNode);

    setupScreen();
    printw("    Enter the item name: %s\n", itemName);
    printw("    Enter the item's link (\\cancel to cancel): ");
    borderScreen();
    
    do {ReadLine(itemLink, 299);} while(strlen(itemLink) == 0);

    if(strcmp(itemLink, "\\cancel") == 0) {
        ReturnToPreviousMenu(nullptr);
        mainChannel->DeleteChild(item);
        return;
    }

    XMLNode* linkNode = masterDoc->NewElement("link");
    linkNode->InsertFirstChild(masterDoc->NewText(itemLink));
    item->InsertEndChild(linkNode);

    setupScreen();
    printw("    Enter the item name: %s\n", itemName);
    printw("    Enter the item's link: %s\n", itemLink);
    printw("    Enter the item's description (\\cancel to cancel): ");
    borderScreen();
    
    do {ReadLine(itemDesc, 299);} while(strlen(itemDesc) == 0 && strlen(itemName) == 0); // Either the name or description must be non-empty

    if(strcmp(itemDesc, "\\cancel") == 0) {
        ReturnToPreviousMenu(nullptr);
        mainChannel->DeleteChild(item);
        return;
    }

    XMLNode* descNode = masterDoc->NewElement("description");
    descNode->InsertFirstChild(masterDoc->NewText(itemDesc));
    item->InsertEndChild(descNode);

    char* date = getRFC822Date();

    XMLNode* pubNode = masterDoc->NewElement("pubDate");
    pubNode->InsertFirstChild(masterDoc->NewText(date));
    item->InsertEndChild(pubNode);

    free(date);

    currentHighlight = 0;

    bool isDoingMoreOptions = false;

    while(true) {
        setupScreen();
        printw("    Enter the item name: %s\n", itemName);
        printw("    Enter items's link: %s\n", itemLink);
        printw("    Enter the item's description: %s\n\n    Specify additional item options?\n\n", itemDesc);
        drawOptions();
        borderScreen();

        chr = getch();

        if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = currentMenu->numLabels - 1;
        } else if(chr == KEY_DOWN) {
            if(++currentHighlight >= currentMenu->numLabels) currentHighlight = 0;
        } else if(chr == 10) { // Subs for KEY_ENTER
            if(currentHighlight == 0) isDoingMoreOptions = true;
            break;
        }
    }

    const int numCustomOptions = 3;
    const char* customTagNames[] = {"author", "category", "comments"};
    const char* customTagDescs[] = {"Author of the item", "Sort the item into a category", "URL to a comment page"};

    if(isDoingMoreOptions) {
        for(int i = 0; i < numCustomOptions; i++) {
            setupScreen();
            printw("    Enter the item name: %s\n", itemName);
            printw("    Enter items's link: %s\n", itemLink);
            printw("    Enter the item's description: %s\n\n    (Leave empty to exclude)\n", feedLink);

            printw("    %s: ", customTagDescs[i]);

            borderScreen();

            char* tstr = (char*)malloc(sizeof(char) * 1000);
            ReadLine(tstr, 999);

            if(strlen(tstr)) {
                XMLNode* tNode = masterDoc->NewElement(customTagNames[i]);
                tNode->InsertFirstChild(masterDoc->NewText(tstr));
                item->InsertEndChild(tNode);
            }
        }
    }

    ReturnToPreviousMenu(nullptr);
}

void GoToEditFeedData(Menu* _cm) {
    currentMenuId++;
    currentMenus[currentMenuId] = &editingFeedDataMenu;

    currentHighlight = 0;

    XMLNode* tempLoopNode = mainChannel->FirstChild();
    
    if(!mainChannel) return;

    int childCount = mainChannel->ChildElementCount();
    int numOptions = 0;

    XMLNode* currentSelectedChild = nullptr;

    while(true) {
        setupScreen();
        printw("    Edit feed attributes\n\n");

        numOptions = 0;
        tempLoopNode = mainChannel->FirstChild();

        int currentOption = 0;

        for(int i = 0; i < childCount; i++) {
            if(tempLoopNode == NULL) {
                break;
            }
            if(strcmp((char*)tempLoopNode->Value(), "item") == 0 || strcmp((char*)tempLoopNode->Value(), "generator") == 0) continue;
            printw("   - ");
            if(currentOption == currentHighlight) {
                attron(A_REVERSE);
                currentSelectedChild = tempLoopNode;
            }

            printw("Edit %s (currently %.75s)", tempLoopNode->Value(), (char*)tempLoopNode->FirstChild()->Value());
            numOptions++;

            if(currentOption == currentHighlight) {
                attroff(A_REVERSE);
            }
            currentOption++;
            printw("\n");
            tempLoopNode = tempLoopNode->NextSibling();
        }

        printw("\n   - ");
        if(numOptions == currentHighlight) {
            attron(A_REVERSE);
        }
        printw("Exit");
        if(numOptions == currentHighlight) {
            attroff(A_REVERSE);
        }

        borderScreen();

        chr = getch();

        if(chr == KEY_DOWN) {
            if(++currentHighlight > numOptions) currentHighlight = 0;
        } else if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = numOptions;
        } else if(chr == 10) {
            if(currentHighlight < numOptions) {
                if(currentSelectedChild == nullptr) continue;

                setupScreen();

                char tchl[10000];

                printw("\n    Edit %s:\n    (Currently %s)\n    (Enter to cancel)\n    (\\clear to delete)\n\n    ", currentSelectedChild->Value(), currentSelectedChild->FirstChild()->Value());

                borderScreen();

                ReadLine(tchl, 9999);

                if(strcmp(tchl, "\\clear") == 0) {
                    currentSelectedChild->Parent()->DeleteChild(currentSelectedChild);
                } else if(strlen(tchl)) currentSelectedChild->FirstChild()->SetValue(tchl);
            } else {
                break;
            }
        }
    }

    ReturnToPreviousMenu(nullptr);
}

XMLNode* currentEditingItemNode = nullptr;

void GoToEditItems(Menu* _cm) {
    currentMenuId++;
    currentMenus[currentMenuId] = &editingFeedDataMenu;

    currentHighlight = 0;

    XMLNode* tempLoopNode = mainChannel->FirstChild();
    
    if(!mainChannel) return;

    int childCount = mainChannel->ChildElementCount();
    int numOptions = 0;

    XMLNode* currentSelectedChild = nullptr;

    while(true) {
        setupScreen(false);
        printw("    Edit items\n\n");

        numOptions = 0;
        tempLoopNode = mainChannel->FirstChild();

        int currentOption = 0;

        for(int i = 0; i < childCount; i++) {
            if(tempLoopNode == NULL) {
                break;
            }
            if(strcmp((char*)tempLoopNode->Value(), "item") != 0) {
                tempLoopNode = tempLoopNode->NextSibling();
                continue;
            }
            printw("   - ");
            if(currentOption == currentHighlight) {
                attron(A_REVERSE);
                currentSelectedChild = tempLoopNode;
            }

            printw("Edit \"%.75s\"", tempLoopNode->FirstChildElement("title")->FirstChild()->Value());
            numOptions++;

            if(currentOption == currentHighlight) {
                attroff(A_REVERSE);
            }
            currentOption++;
            printw("\n");
            tempLoopNode = tempLoopNode->NextSibling();
        }

        printw("\n   - ");
        if(numOptions == currentHighlight) {
            attron(A_REVERSE);
        }
        printw("Exit");
        if(numOptions == currentHighlight) {
            attroff(A_REVERSE);
        }

        borderScreen();

        chr = getch();

        if(chr == KEY_DOWN) {
            if(++currentHighlight > numOptions) currentHighlight = 0;
        } else if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = numOptions;
        } else if(chr == 10) {
            if(currentHighlight == numOptions) {
                break;
            }
            currentEditingItemNode = currentSelectedChild;
            GoToEditSingleItem(nullptr);
        }
    }

    ReturnToPreviousMenu(nullptr);
}

void GoToEditSingleItem(Menu* _cm) {
    if(currentEditingItemNode == nullptr || currentEditingItemNode->ChildElementCount() == 0) return;

    currentMenus[++currentMenuId] = &editingItemsMenu;

    currentHighlight = 0;

    XMLNode* tempLoopNode = currentEditingItemNode->FirstChild();

    int childCount = currentEditingItemNode->ChildElementCount();
    int numOptions = 0;

    XMLNode* currentSelectedChild = nullptr;

    while(true) {
        setupScreen();
        printw("    Editing \"%s\"\n\n", currentEditingItemNode->FirstChildElement("title")->FirstChild()->Value());

        numOptions = 0;
        tempLoopNode = currentEditingItemNode->FirstChild();
        childCount = currentEditingItemNode->ChildElementCount();
        currentSelectedChild = nullptr;

        int currentOption = 0;

        for(int i = 0; i < childCount; i++) {
            if(tempLoopNode == NULL) {
                break;
            }
            if(strcmp((char*)tempLoopNode->Value(), "pubDate") == 0) {
                tempLoopNode = tempLoopNode->NextSibling();
                continue;
            }
            printw("   - ");
            if(currentOption == currentHighlight) {
                attron(A_REVERSE);
                currentSelectedChild = tempLoopNode;
            }

            printw("Edit \"%.75s\" (currently \"%.75s\")", tempLoopNode->Value(), tempLoopNode->FirstChild()->Value());
            numOptions++;

            if(currentOption == currentHighlight) {
                attroff(A_REVERSE);
            }
            currentOption++;
            printw("\n");
            tempLoopNode = tempLoopNode->NextSibling();
        }

        printw("\n   - ");
        if(numOptions == currentHighlight) {
            attron(A_REVERSE);
        }
        printw("Exit");
        if(numOptions == currentHighlight) {
            attroff(A_REVERSE);
        }

        borderScreen();

        chr = getch();

        if(chr == KEY_DOWN) {
            if(++currentHighlight > numOptions) currentHighlight = 0;
        } else if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = numOptions;
        } else if(chr == 10) {
            if(currentHighlight < numOptions) {
                if(currentSelectedChild == nullptr) break;

                setupScreen();

                char tchl[10000];

                printw("\n    Edit %s:\n    (Currently %s)\n    (Enter to cancel)\n    (\\clear to delete)\n\n    ", currentSelectedChild->Value(), currentSelectedChild->FirstChild()->Value());

                borderScreen();

                ReadLine(tchl, 9999);

                if(strcmp(tchl, "\\clear") == 0) {
                    currentSelectedChild->Parent()->DeleteChild(currentSelectedChild);
                } else if(strlen(tchl)) currentSelectedChild->FirstChild()->SetValue(tchl);
            } else {
                break;
            }
        }
    }

    ReturnToPreviousMenu(nullptr);
}

void goToCreateFeed(Menu* _cm) {
    currentMenuId++;
    currentMenus[currentMenuId] = &createFeedMenu;

    if(masterDoc) delete masterDoc;

    masterDoc = new XMLDocument();

    XMLNode* root = masterDoc->NewElement("rss");
    root->ToElement()->SetAttribute("version", "2.0");
    masterDoc->InsertFirstChild(root);

    mainChannel = masterDoc->NewElement("channel");
    root->InsertFirstChild(mainChannel);

    STARTPOINT:

    setupScreen();
    printw("    Enter your file name (feed.xml): ");
    borderScreen();
    
    ReadLine(fileName, 299);
    if(strlen(fileName) == 0) strcpy(fileName, "feed.xml");

    if(fileExists(fileName)) {
        printw("    That file already exists!\n");
        borderScreen();
        getch();
        goto STARTPOINT;
    }

    BEFOREFILE:

    setupScreen();
    printw("    Enter your file name: %s\n", fileName);
    printw("    Enter your feed's display name: ");
    borderScreen();
    
    ReadLine(feedName, 299);
    if(strlen(feedName) == 0) goto BEFOREFILE;

    XMLNode* nameNode = masterDoc->NewElement("title");
    nameNode->InsertFirstChild(masterDoc->NewText(feedName));
    mainChannel->InsertEndChild(nameNode);

    BEFORETITLE:

    setupScreen();
    printw("    Enter your file name: %s\n", fileName);
    printw("    Enter your feed's display name: %s\n", feedName);
    printw("    Enter your feed website's link: ");
    borderScreen();
    
    ReadLine(feedLink, 299);
    if(strlen(feedLink) == 0) goto BEFORETITLE;

    XMLNode* linkNode = masterDoc->NewElement("link");
    linkNode->InsertFirstChild(masterDoc->NewText(feedLink));
    mainChannel->InsertEndChild(linkNode);

    BEFOREDESC:

    setupScreen();
    printw("    Enter your file name: %s\n", fileName);
    printw("    Enter your feed's display name: %s\n", feedName);
    printw("    Enter your feed website's link: %s\n", feedLink);
    printw("    Enter your feed's description: ");
    borderScreen();
    
    ReadLine(feedDesc, 299);
    if(strlen(feedDesc) == 0) goto BEFOREDESC;

    XMLNode* descNode = masterDoc->NewElement("description");
    descNode->InsertFirstChild(masterDoc->NewText(feedDesc));
    mainChannel->InsertEndChild(descNode);

    XMLNode* genNode = masterDoc->NewElement("generator");
    genNode->InsertFirstChild(masterDoc->NewText("Alex's RSSgen"));
    mainChannel->InsertEndChild(genNode);

    goToEditFeed(nullptr);
}

void loadFeed(Menu* _cm) {

}

const char* mainMenuLabels[]     = {"Create Feed", "Edit Feed", "Exit"};
const char* createFeedLabels[]   = {"Something", fileName};
const char* editingFeedLabels[]  = {"Add item", "Edit feed data", "Edit existing items", "Exit and save", "Exit and do NOT save"};
const char* creatingItemLabels[] = {"Yes", "No"};

const char* noLabels[]           = {};

Menu mainMenu            = {"RSSGEN", mainMenuLabels, 3, {goToCreateFeed, loadFeed, ReturnToPreviousMenu}};
Menu createFeedMenu      = {"Create Feed", createFeedLabels, 2, {nullptr, ReturnToPreviousMenu}};
Menu editingFeedMenu     = {"Edit Feed", editingFeedLabels, 5, {GoToCreateItem, GoToEditFeedData, GoToEditItems, QuitApp, QuitSansSaving}};
Menu createItemMenu      = {"Create Item", creatingItemLabels, 2, {nullptr, nullptr}};
Menu editingFeedDataMenu = {"", noLabels, 0, {}};
Menu editingItemsMenu    = {"", noLabels, 0, {}};

void setupScreen(bool drawsTitle) {
    clear();
    printw("\n\n");
    if(drawsTitle) {
        printw("    %s", currentMenu->name.c_str());
        printw("\n\n");
    }
}

void borderScreen() {
    attron(A_REVERSE);
    box(stdscr, 0, 0);
    attroff(A_REVERSE);
}

int main(int argc, char **argv) {
    bool shouldDoNormalMenu = true;

    initscr();

    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    wnoutrefresh(stdscr);

    if(argc > 1) {
        if(fileExists(string(argv[1]))) {
            masterDoc = new XMLDocument();
            masterDoc->LoadFile(argv[1]);

            bool loadedFine = true;

            strcpy(fileName, argv[1]);
            if(masterDoc->ChildElementCount("rss") && masterDoc->FirstChildElement("rss")->ChildElementCount("channel")) {
                mainChannel = masterDoc->FirstChildElement("rss")->FirstChildElement("channel");

                strcpy(feedName, masterDoc->FirstChildElement("rss")->FirstChildElement("channel")->FirstChildElement("title")->GetText());
                strcpy(feedDesc, masterDoc->FirstChildElement("rss")->FirstChildElement("channel")->FirstChildElement("description")->GetText());
                strcpy(feedLink, masterDoc->FirstChildElement("rss")->FirstChildElement("channel")->FirstChildElement("link")->GetText());
            } else {
                loadedFine = false;
            }

            if(loadedFine) {
                shouldDoNormalMenu = false;
                goToEditFeed(nullptr);
            }
        } else {
            printw("\n\n    No such file exists!\n\n    (Press any key to continue)");
            borderScreen();

            getch();

            currentMenus[0] = &mainMenu;
        }
    } else {
        currentMenus[0] = &mainMenu;
    }

    if(shouldDoNormalMenu) while(true) {
        setupScreen();
        drawOptions();
        borderScreen();

        chr = getch();
        if(chr == KEY_UP) {
            if(--currentHighlight < 0) currentHighlight = currentMenu->numLabels - 1;
        } else if(chr == KEY_DOWN) {
            if(++currentHighlight >= currentMenu->numLabels) currentHighlight = 0;
        } else if(chr == 10) { // Subs for KEY_ENTER
            if(currentMenu->enterFunctions[currentHighlight] != nullptr) {
                currentMenu->enterFunctions[currentHighlight](currentMenu);
            }
        }
        if(shouldQuitApp) break;
    }

    curs_set(1);

    endwin();

    if(masterDoc && isSaving) {
        FILE* f = fopen(fileName, "w");
        if(f) {
            masterDoc->SaveFile(f);
            fclose(f);
        } else {
            cout << "Could not save to " << fileName << "!" << endl;
        }
    }

    return 0;
}