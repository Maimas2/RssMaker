g++ -g -o rss-maker.o -c main.cpp -g
g++ -g -std=c++20 -o rss-maker rss-maker.o -lncursesw -lncurses -ltinfo -ltinyxml2 `pkg-config --libs ncurses ncursesw` -g