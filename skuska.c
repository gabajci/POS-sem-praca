#include <ncurses.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>


int main() {
    initscr();
    cbreak();
    noecho();

    WINDOW* win = newwin(10, 40,  2 , 2);
    refresh();

    //okno
    keypad(win,true);
    box(win, 0, 0);
    wrefresh(win);

    char * paddle = "|";

    int xServer = 1;
    int xClient = 38;

    int yServer = 1;
    int yClient = 1;

    int koniecHry=0;
    while(!koniecHry) {
        mvwprintw(win, yServer, xServer, paddle);
        mvwprintw(win, yServer + 1, xServer, paddle);
        mvwprintw(win, yServer + 2, xServer, paddle);

        mvwprintw(win, yClient, xClient, paddle);
        mvwprintw(win, yClient + 1, xClient, paddle);
        mvwprintw(win, yClient + 2, xClient, paddle);

        wrefresh(win);

        int key = wgetch(win);
        if (key == KEY_UP) {
            if (yServer- 1 > 0) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xServer, ' ');
                }
                yServer--;
            }
        } else if (key == KEY_DOWN) {
            if (yServer + 1 < 7) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xServer, ' ');
                }
                yServer++;
            }
        }
        if(yServer==5555){
            koniecHry=1;
        }
        key=0;

    }
    getch();
    endwin();

    return 0;
}


