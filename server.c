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

#define WHEIGHT 10
#define WWIDTH 50

#define MAXSCORE 4

#define UPRIGHT 1
#define RIGHT 2
#define DOWNRIGHT 3
#define DOWNLEFT 4
#define LEFT 5
#define UPLEFT 6


//server
typedef struct dataClient {
    pthread_mutex_t* mutex;
    pthread_cond_t* condZobraz;
    int ballX;
    int ballY;
    int paddleServer;
    int paddleClient;
    int scoreServer;
    int scoreClient;
    int koniecHry;
    int port;
} Data_client;

void displayPaddle(WINDOW * win, int y, int x) {
    char * paddle = "#"; //TODO: memory leak?
    mvwprintw(win, y, x, paddle);
    mvwprintw(win, y+1, x, paddle);
    mvwprintw(win, y+2, x, paddle);
}

void *logika_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;

    int smer, ballY, ballX;

    //zaciatok hry?
    ballY = WHEIGHT / 2;
    ballX = WWIDTH / 2;

    sleep(2);

    //TODO sucet skore parny -> dolava, sucet skore neparny -> doprava
    smer = RIGHT; //zacina doprava
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
            pthread_mutex_unlock(d->mutex);
            smer=LEFT;
            sleep(3);
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
            pthread_mutex_unlock(d->mutex);
            smer=RIGHT;
            sleep(3);
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
        usleep(100000);

        pthread_mutex_lock(d->mutex);
        if(d->scoreServer == MAXSCORE || d->scoreClient == MAXSCORE) {
            d->koniecHry = 1;
        }
    }

    pthread_mutex_unlock(d->mutex);
    return 0;
}

void* prenos_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;

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
        perror("Chyba pri vytváraní socketu");
        return 0;
    }

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Chyba pri bindovaní socketu.");
        return 0;
    }

    listen(sockfd, 5);
    cli_len = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &cli_len);
    if (newsockfd < 0)
    {
        perror("Chyba prijatia.");
        return 0;
    }

    pthread_mutex_lock(d->mutex);
    while(!d->koniecHry) {
        pthread_mutex_unlock(d->mutex);

        bzero(buffer,64);
        n = read(newsockfd, buffer, 63);
        //printf("server precital %s,%d\n",buffer,atoi(&buffer[0]));
        pthread_mutex_lock(d->mutex);

        //ak sa pozicia klienta, ktoru pozna server nezhoduje s tou, co mu poslal klient v bufferi, synchronizuje si poziciu
        if(d->paddleClient!=atoi(&buffer[0])) {
            d->paddleClient=atoi(&buffer[0]);
            pthread_cond_signal(d->condZobraz);
        }

        pthread_mutex_unlock(d->mutex);
        if (n < 0) {
            perror("Chyba pri načítaní socketu.");
            return 0;
        }

        pthread_mutex_lock(d->mutex);
        //pozicie v bufferi: 1. paddleClient, 2. paddleServer, 3. ballY, 4.ballX, 5. scoreClient, 6. scoreServer, 7. koniec
        bzero(buffer,64);
        sprintf(&buffer[0], "%d %d %d %d %d %d %d ",d->paddleClient, d->paddleServer, d->scoreClient, d->scoreServer, d->koniecHry, d->ballY,d->ballX);
        pthread_mutex_unlock(d->mutex);

        n = write(newsockfd, buffer, strlen(buffer));
        //printf("server zapisal %s,%d\n",buffer,atoi(&buffer[4]));
        if (n < 0) {
            perror("Chyba pri zapisovaní socketu.");
            return 0;
        }
        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);
    close(newsockfd);
    close(sockfd);

    return 0;
}


void* zobraz_func(void* data) {

    struct dataClient *d = (struct dataClient *) data;

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

        mvwprintw(win, 0, 22, "%d", scoreServer);
        mvwprintw(win, 0, 28, "%d", scoreClient);

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

        //TODO: funkcia na zobrazenie lopty
        char * ball = "o";
        mvwprintw(win, ballY, ballX, ball);
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

    getch();
    endwin();

    return 0;
}



int main(int argc, char *argv[]) {
    pthread_t zobraz;
    pthread_t prenos;
    pthread_t logika;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    if(argc < 2) {
        fprintf(stderr, "Program vyzaduje 1 argument: cislo portu\n");
        return -1;
    }
    int port = atoi(argv[1]);
    int ballY = WHEIGHT / 2;
    int ballX = WWIDTH / 2;

    struct dataClient d ={&mutex,&cond,ballX,ballY,1,1,0,0,0,port};

    pthread_create(&zobraz,NULL,&zobraz_func,&d);
    pthread_create(&prenos,NULL,&prenos_func,&d);
    pthread_create(&logika,NULL,&logika_func,&d);

    pthread_join(zobraz,NULL);
    pthread_join(prenos,NULL);
    pthread_join(logika,NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}


