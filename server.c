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
#include "server.h"
#include "constants.h"

#define UPRIGHT 1
#define RIGHT 2
#define DOWNRIGHT 3
#define DOWNLEFT 4
#define LEFT 5
#define UPLEFT 6


//server
struct dataServer {
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
    int pripojilSa;
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

int getZacSmer(int sucetSkore) {
    if(sucetSkore % 2 == 0) {
        return LEFT;
    } else {
        return RIGHT;
    }
}

void *logika_func (void* data) {
    struct dataServer *d = (struct dataServer *) data;

    pthread_mutex_lock(d->mutex);
    while(!d->pripojilSa){
        pthread_cond_wait(d->cond,d->mutex);
    }
    pthread_mutex_unlock(d->mutex);

    int smer, ballY, ballX;

    ballY = WHEIGHT / 2;
    ballX = WWIDTH / 2;

    sleep(2);

    smer = LEFT; //zacina dolava
    pthread_mutex_lock(d->mutex);
    while(!d->koniecHry) {
        d->ballY = ballY;
        d->ballX = ballX;
        pthread_mutex_unlock(d->mutex);


        ////////////////novy if//////////////////////
        if(ballX == WWIDTH - 3) { // ak je lopticka pred palkou vpravo
            pthread_mutex_lock(d->mutex);
            if(ballY == d->paddleClient) { // ak trafi vrch palky odrazi sa hore dolava
                pthread_mutex_unlock(d->mutex);
                smer = UPLEFT;
            } else if(ballY == d->paddleClient + 1) { // ak trafi stred palky odrazi sa dolava
                pthread_mutex_unlock(d->mutex);
                smer = LEFT;
            } else if(ballY == d->paddleClient + 2) { // ak trafi spodok palky odrazi sa dole dolava
                pthread_mutex_unlock(d->mutex);
                smer = DOWNLEFT;
            } else {
                pthread_mutex_unlock(d->mutex);
            }
        }

        if(ballX == WWIDTH - 2) { // ak lopticka trafi pravy okraj
            ballY = WHEIGHT / 2;
            ballX = WWIDTH / 2;
            pthread_mutex_lock(d->mutex);
            d->scoreServer++;
            d->ballY = ballY;
            d->ballX = ballX;
            if(d->scoreClient!=MAXSCORE){
                smer = getZacSmer(d->scoreClient + d->scoreServer);
                pthread_mutex_unlock(d->mutex);
                sleep(3);
            } else {
                pthread_mutex_unlock(d->mutex);
            }
        }

        //////////////////////////////////////////////////////
        if(ballX == 2) { //ak je lopticka pred palkou vlavo
            pthread_mutex_lock(d->mutex);
            if (ballY == d->paddleServer) { // ak trafi vrch palky odrazi sa hore doprava
                pthread_mutex_unlock(d->mutex);
                smer = UPRIGHT;
            } else if( ballY == d->paddleServer + 1) { // ak trafi stred palky odrazi sa doprava
                pthread_mutex_unlock(d->mutex);
                smer = RIGHT;
            } else if( ballY == d->paddleServer + 2) { // ak trafi spodok palky odrazi sa dole doprava
                pthread_mutex_unlock(d->mutex);
                smer = DOWNRIGHT;
            } else {
                pthread_mutex_unlock(d->mutex);
            }
        }


        if (ballX == 1) { //ak je na lavom kraji
            ballY = WHEIGHT / 2;
            ballX = WWIDTH / 2;
            pthread_mutex_lock(d->mutex);
            d->scoreClient++;
            d->ballY = ballY;
            d->ballX = ballX;
            if(d->scoreServer!=MAXSCORE){
                smer = getZacSmer(d->scoreClient + d->scoreServer);
                pthread_mutex_unlock(d->mutex);
                sleep(3);
            } else {
                pthread_mutex_unlock(d->mutex);
            }
        }

        ////////////////////////////////////////////////
        if (ballY < 2) {  //ak lopticka narazi hore
            if(smer == UPLEFT){
                smer = DOWNLEFT;
            } else { //smer == UPRIGHT
                smer = DOWNRIGHT;
            }
        }

        if (ballY > WHEIGHT - 3){ //ak lopticka narazi dole
            if(smer == DOWNLEFT) {
                smer = UPLEFT;
            } else { //smer == DOWNRIGHT
                smer = UPRIGHT;
            }
        }

        if(smer == UPRIGHT) {
            ballY--;
            //ballX+=3;
            ballX++;
        } else if(smer == RIGHT) {
            //ballX+=3;
            ballX++;
        } else if(smer == DOWNRIGHT) {
            ballY++;
            //ballX += 3;
            ballX++;
        } else if (smer == DOWNLEFT) {
            //ballX-=3;
            ballX--;
            ballY++;
        } else if (smer == LEFT) {
            //ballX-=3;
            ballX--;
        } else {
            //ballX-=3;
            ballX--;
            ballY--;
        }

        pthread_mutex_lock(d->mutex);
        if(d->scoreClient!=MAXSCORE && d->scoreServer!=MAXSCORE){
            pthread_mutex_unlock(d->mutex);
            usleep(100000);
        } else {
            pthread_mutex_unlock(d->mutex);
        }

        pthread_mutex_lock(d->mutex);
        if(d->scoreServer == MAXSCORE || d->scoreClient == MAXSCORE) {
            d->koniecHry = 1;
        }
    }

    pthread_mutex_unlock(d->mutex);

    fprintf(stderr,"Server: koniec vlakna logika");
    return 0;
}

void* prenos_func (void* data) {
    struct dataServer *d = (struct dataServer *) data;

    int sockfd, newsockfd; //deskriptory servera a klienta
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr; // adresa klienta a servera
    int n;
    char buffer[64];

    bzero((char*)&serv_addr, sizeof(serv_addr)); //vycistenie strukturu so serverovou adresou
    serv_addr.sin_family = AF_INET; //komunikacia pomocou IPV4
    serv_addr.sin_addr.s_addr = INADDR_ANY; //pocuvame na akukolvek adresu
    //htons - skonvertuje 16-bitove cele cislo na sietovu reprezentaciu
    serv_addr.sin_port = htons(d->port); //port na ktorom budeme pocuvat

    //vytvorenie socketu
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        //perror("Chyba pri vytváraní socketu");
        fprintf(stderr, "Chyba pri vytváraní socketu");

        return 0;
    }

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        //perror("Chyba pri bindovaní socketu.");
        fprintf(stderr, "Chyba pri bindovaní socketu.");
        return 0;
    }

    listen(sockfd, 5);
    cli_len = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
    if (newsockfd < 0)
    {
        //perror("Chyba prijatia.");
        fprintf(stderr, "Chyba prijatia.");
        return 0;
    }

    pthread_mutex_lock(d->mutex);
    while(!d->pripojilSa){
        pthread_mutex_unlock(d->mutex);

        bzero(buffer,64);
        n = read(newsockfd, buffer, 63);

        if (n < 0) {
            //perror("Chyba pri čítaní socketu.");
            fprintf(stderr, "Chyba pri čítaní socketu.");
            return 0;
        }

        if(strcmp(buffer,"Pripojil som sa.")==0) {
            printf("Súper sa pripojil. Priprav sa!\nHra sa začne o 3 sekundy.\n");
            sleep(3);
            pthread_mutex_lock(d->mutex);
            d->pripojilSa=1;
            pthread_cond_broadcast(d->cond);
            pthread_mutex_unlock(d->mutex);
        }
        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);


    const char* msg = "Hra začala.";
    n = write(newsockfd, msg, strlen(msg)+1);

    if (n < 0)
    {
        //perror("Chyba pri zápise do socketu.");
        fprintf(stderr,"Chyba pri zápise do socketu.");
        return 0;
    }



    pthread_mutex_lock(d->mutex);
    while(!d->koniecHry) {
        pthread_mutex_unlock(d->mutex);

        bzero(buffer, 64);
        n = read(newsockfd, buffer, 63);
        pthread_mutex_lock(d->mutex);

        if (d->paddleClient != atoi(&buffer[0])) {
            d->paddleClient = atoi(&buffer[0]);
        }

        pthread_mutex_unlock(d->mutex);

        pthread_mutex_lock(d->mutex);
        if (!d->koniecHry) {
            pthread_mutex_unlock(d->mutex);
            if (n < 0) {
                //perror("Chyba pri načítaní socketu.");
                fprintf(stderr,"Chyba pri načítaní socketu.");
                return 0;
            }
            pthread_mutex_lock(d->mutex);
        }
        pthread_mutex_unlock(d->mutex);


        pthread_mutex_lock(d->mutex);
        bzero(buffer,64);
        sprintf(&buffer[0], "%d %d %d %d %d %d %d ",d->paddleClient, d->paddleServer,
                            d->scoreClient, d->scoreServer, d->koniecHry, d->ballY,d->ballX);
        pthread_mutex_unlock(d->mutex);

        n = write(newsockfd, buffer, strlen(buffer));

        pthread_mutex_lock(d->mutex);
        if(!d->koniecHry) {
            pthread_mutex_unlock(d->mutex);
            if (n < 0) {
                //perror("Chyba pri zapisovaní socketu.");
                fprintf(stderr,"Chyba pri zapisovaní socketu.");
                return 0;
            }
            pthread_mutex_lock(d->mutex);
        }
        pthread_mutex_unlock(d->mutex);


        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);
    close(newsockfd);
    close(sockfd);


    fprintf(stderr,"Server: koniec vlakna prenos");
    return 0;
}


void* plocha_func(void* data) {

    struct dataServer *d = (struct dataServer *) data;

    pthread_mutex_lock(d->mutex);
    while(!d->pripojilSa){
        printf("Čakám na súpera.\n");
        pthread_cond_wait(d->cond,d->mutex);
    }
    pthread_mutex_unlock(d->mutex);

    initscr();
    cbreak();
    noecho();

    WINDOW* win = newwin(WHEIGHT, WWIDTH,  2 , 2);
    int nula = 0;

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

        mvwaddch(win, yClient, xClient, ' ');
        mvwaddch(win, yClient+1, xClient, ' ');
        mvwaddch(win, yClient+2, xClient, ' ');
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
            //pohni sa hore
            case KEY_UP: {
                //dovoli iba ak nevyskoci z plochy hore
                if (yServer - 1 > 0) {
                    //zmaze sa cely riadok kde je paddleServer
                    for(int i = 1; i <= WHEIGHT - 2; i++){
                        mvwaddch(win, i, xServer, ' ');
                    }
                    wrefresh(win);
                    yServer--;
                    pthread_mutex_lock(d->mutex);
                    d->paddleServer--;
                    pthread_mutex_unlock(d->mutex);
                }
            } break;
            case KEY_DOWN: {
                //dovoli iba ak nevyskoci z plochy dole
                if (yServer + 1 < WHEIGHT - 3) {
                    //zmaze sa cely riadok kde je paddleServer
                    for(int i = 1; i <= WHEIGHT - 2; i++){
                        mvwaddch(win, i, xServer, ' ');
                    }
                    wrefresh(win);
                    //yServer++;
                    pthread_mutex_lock(d->mutex);
                    d->paddleServer++;
                    pthread_mutex_unlock(d->mutex);
                }
            } break;
            default: break;
        }

        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);

    //getch();
    endwin();

    fprintf(stderr,"Server: koniec vlakna zobraz");
    return 0;
}



int main(int argc, char *argv[]) {
    pthread_t plocha;
    pthread_t prenos;
    pthread_t logika;

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

    struct dataServer d ={&mutex,&cond,ballX,ballY,1,1,0,0,0,port,0};

    pthread_create(&plocha,NULL,&plocha_func,&d);
    pthread_create(&prenos,NULL,&prenos_func,&d);
    pthread_create(&logika,NULL,&logika_func,&d);

    pthread_join(plocha,NULL);
    pthread_join(prenos,NULL);
    pthread_join(logika,NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    if(d.scoreServer>d.scoreClient){
        printf("  Gratulujeme!!!\n  Vyhral si.\n  Konečné skóre:\n    %d : %d\n",d.scoreServer,d.scoreClient);
    } else {
        printf("  Oops...\n  Prehral si.\n  Konečné skóre:\n    %d : %d \n",d.scoreServer,d.scoreClient);
    }

    fprintf(stderr,"Server: koniec main");
    return 0;
}