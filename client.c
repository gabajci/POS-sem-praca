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
#include "client.h"
#include "constants.h"


//client
struct dataClient {
    pthread_mutex_t* mutex;
    pthread_cond_t* cond;
    int ballX;
    int ballY;
    int paddleServer;
    int paddleClient;
    int scoreServer;
    int scoreClient;
    int koniecHry;
    int port;
    int hraZacala;
    int odpojenie;
};


void displayPaddle(WINDOW * win, int y, int x) {
    char * paddle = "#";
    mvwprintw(win, y, x, paddle);
    mvwprintw(win, y+1, x, paddle);
    mvwprintw(win, y+2, x, paddle);
}

void displayBall(WINDOW * win, int y, int x) {
    char * ball = "o";
    mvwprintw(win, y, x, ball);
}

void* prenos_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    char buffer[64];

    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Chyba, server sa nenašiel.\n");
        pthread_mutex_lock(d->mutex);
        d->hraZacala=1;
        d->koniecHry=1;
        pthread_cond_signal(d->cond);
        pthread_mutex_unlock(d->mutex);
        return 0;
    }

    bzero((char*)&serv_addr, sizeof(serv_addr)); //vycistime adresu servera
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char*)server->h_addr,
            (char*)&serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(d->port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr,"Chyba pri vytvorení socketu.\n");
        pthread_mutex_lock(d->mutex);
        d->hraZacala=1;
        d->koniecHry=1;
        pthread_cond_signal(d->cond);
        pthread_mutex_unlock(d->mutex);
        close(sockfd);
        return 0;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        //perror("Chyba pri pripojení.");
        fprintf(stderr,"Chyba pri pripojení.\n");
        pthread_mutex_lock(d->mutex);
        d->hraZacala=1;
        d->koniecHry=1;
        pthread_cond_signal(d->cond);
        pthread_mutex_unlock(d->mutex);
        close(sockfd);
        return 0;
    }


    const char* msg = "Pripojil som sa.";
    bzero(buffer,64);
    n = write(sockfd, msg, strlen(msg)+1);

    if (n < 0)
    {
        fprintf(stderr,"Chyba pri zápise do socketu.\n");
        pthread_mutex_lock(d->mutex);
        d->hraZacala=1;
        d->koniecHry=1;
        pthread_cond_signal(d->cond);
        pthread_mutex_unlock(d->mutex);
        close(sockfd);
        return 0;
    }

    printf("Priprav sa!\nHra sa začne o 3 sekundy.\n");
    pthread_mutex_lock(d->mutex);
    while(!d->hraZacala){
        pthread_mutex_unlock(d->mutex);

        bzero(buffer,64);
        n = read(sockfd, buffer, 63);

        if (n < 0) {
            //perror("Chyba pri čítaní socketu.");
            fprintf(stderr,"Chyba pri čítaní socketu.");
            pthread_mutex_lock(d->mutex);
            d->hraZacala=1;
            d->koniecHry=1;
            pthread_cond_signal(d->cond);
            pthread_mutex_unlock(d->mutex);
            close(sockfd);
            return 0;
        }

        if(strcmp(buffer,"Hra začala.")==0) {
            pthread_mutex_lock(d->mutex);
            d->hraZacala=1;
            pthread_cond_signal(d->cond);
            pthread_mutex_unlock(d->mutex);
        }
        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);

    pthread_mutex_lock(d->mutex);

    while(!d->koniecHry) {

        bzero(buffer,64);

        sprintf(&buffer[0], "%d %d ", d->paddleClient,d->odpojenie);

        pthread_mutex_unlock(d->mutex);

        n = write(sockfd, buffer, strlen(buffer));

        if (n < 0) {
            //chyba pri citani
            pthread_mutex_lock(d->mutex);
            d->koniecHry=1;
            pthread_mutex_unlock(d->mutex);
            close(sockfd);
            return 0;
        }

        bzero(buffer,64);
        n = read(sockfd, buffer, 63);

        if (n < 0) {
            //chyba pri yapisovani
            pthread_mutex_lock(d->mutex);
            d->koniecHry=1;
            pthread_mutex_unlock(d->mutex);
            close(sockfd);
            return 0;
        }

        pthread_mutex_lock(d->mutex);

        if(atoi(&buffer[0])!=d->paddleServer){
            d->paddleServer=atoi(&buffer[0]);
        }
        if(atoi(&buffer[2])!=d->paddleServer){
            d->paddleServer=atoi(&buffer[2]);
        }

        if(atoi(&buffer[4])!=d->scoreClient){
            d->scoreClient=atoi(&buffer[4]);
        }
        if(atoi(&buffer[6])!=d->scoreServer){
            d->scoreServer=atoi(&buffer[6]);
        }

        if(!d->koniecHry) {
            if (atoi(&buffer[8]) != d->koniecHry) {
                d->koniecHry = atoi(&buffer[8]);
            }
        }

        if(atoi(&buffer[10])!=d->ballY){
            d->ballY=atoi(&buffer[10]);
        }
        if(atoi(&buffer[12])!=d->ballX){
            d->ballX=atoi(&buffer[12]);
        }
        pthread_mutex_unlock(d->mutex);

        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);
    close(sockfd);

    return 0;

}



void* plocha_func(void* data) {

    struct dataClient *d = (struct dataClient *) data;

    pthread_mutex_lock(d->mutex);
    while(!d->hraZacala){
        pthread_cond_wait(d->cond,d->mutex);
    }
    pthread_mutex_unlock(d->mutex);

    initscr();
    cbreak();
    noecho();

    WINDOW* win = newwin(WHEIGHT, WWIDTH,  2 , 2);
    refresh();

    //okno
    keypad(win,true);
    box(win, 0, 0);
    wrefresh(win);

    int xServer = 1;
    int xClient = WWIDTH-2;

    pthread_mutex_lock(d->mutex);
    int yServer = d->paddleServer;
    int yClient = d->paddleClient;
    int ballY = d->ballY;
    int ballX = d->ballX;
    int scoreClient = d->scoreClient;
    int scoreServer = d->scoreServer;

    while(!d->koniecHry) {
        pthread_mutex_unlock(d->mutex);

        mvwprintw(win, SCORESERVER_Y, SCORESERVER_X, "%d", scoreServer);
        mvwprintw(win, SCORECLIENT_Y, SCORECLIENT_X, "%d", scoreClient);

        mvwaddch(win, yServer, xServer, ' ');
        mvwaddch(win, yServer+1, xServer, ' ');
        mvwaddch(win, yServer+2, xServer, ' ');
        mvwaddch(win, ballY, ballX, ' ');
        wrefresh(win);

        pthread_mutex_lock(d->mutex);
        yServer = d->paddleServer;
        yClient = d->paddleClient;
        ballY = d->ballY;
        ballX = d->ballX;
        scoreClient = d->scoreClient;
        scoreServer = d->scoreServer;

        pthread_mutex_unlock(d->mutex);


        displayBall(win, ballY, ballX);
        displayPaddle(win, yServer, xServer);
        displayPaddle(win, yClient, xClient);

        wrefresh(win);

        wtimeout(win,100);
        switch(wgetch(win)){
            case KEY_UP: {
                if (yClient - 1 > 0) {
                    for (int i = 1; i <= 8; i++) {
                        mvwaddch(win, i, xClient, ' ');
                    }
                    wrefresh(win);
                    pthread_mutex_lock(d->mutex);
                    d->paddleClient--;
                    pthread_mutex_unlock(d->mutex);
                }
            } break;
            case KEY_DOWN: {
                if (yClient + 1 < WHEIGHT - 3) {
                    for(int i = 1 ; i <=8;i++){
                        mvwaddch(win, i, xClient, ' ');
                    }
                    wrefresh(win);
                    pthread_mutex_lock(d->mutex);
                    d->paddleClient++;
                    pthread_mutex_unlock(d->mutex);
                }
            }break;
            case 27:{
                pthread_mutex_lock(d->mutex);
                d->odpojenie=1;
                pthread_mutex_unlock(d->mutex);
            } break;
            default: break;
        }

        pthread_mutex_lock(d->mutex);

    }
    pthread_mutex_unlock(d->mutex);

    endwin();

    return 0;
}



int main(int argc, char *argv[]) {
    pthread_t zobraz;
    pthread_t prenos;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    if(argc < 2) {
        fprintf(stderr, "Program vyžaduje 1 argument: číslo portu.\n");
        return -1;
    }
    int port = atoi(argv[1]);
    int ballY = WHEIGHT / 2;
    int ballX = WWIDTH / 2;

    struct dataClient d ={&mutex,&cond,ballX,ballY,1,1,0,0,0,port,0,0};

    pthread_create(&zobraz,NULL,&plocha_func,&d);
    pthread_create(&prenos,NULL,&prenos_func,&d);

    pthread_join(zobraz,NULL);
    pthread_join(prenos,NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    if(d.scoreServer == MAXSCORE || d.scoreClient==MAXSCORE) {
        if (d.scoreServer > d.scoreClient) {
            printf("  Oops...\n  Prehral si.\n  Konečné skóre:\n    %d : %d\n", d.scoreClient, d.scoreServer);
        } else {
            printf("  Gratulujeme!!!\n  Vyhral si.\n  Konečné skóre:\n    %d : %d\n", d.scoreClient, d.scoreServer);
        }
    } else {
        printf("Hra bola prerušená..\n");
    }
    return 0;

}