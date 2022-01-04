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
#define WWIDTH 40


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

void *logika_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;
    int smer, ballY, ballX;
    ballY = WHEIGHT / 2;
    ballX = WWIDTH / 2;
    sleep(2);
    //TODO sucet skore parny -> dolava, sucet skore neparny -> doprava
    smer = 5; //zacina doprava
    pthread_mutex_lock(d->mutex);
    while(!d->koniecHry) {
        d->ballY = ballY;
        d->ballX = ballX;
        pthread_mutex_unlock(d->mutex);


        if(smer == 2) {
            if(ballX == WWIDTH - 2) {
                pthread_mutex_lock(d->mutex);
                if(ballY == d->paddleClient || ballY == d->paddleClient + 1 || ballY == d->paddleClient + 2) {
                    pthread_mutex_unlock(d->mutex);
                    smer = 5;
                } else {
                    ballY = WHEIGHT / 2;
                    ballX = WWIDTH / 2;
                    d->ballY = ballY;
                    d->ballX = ballX;
                    pthread_mutex_unlock(d->mutex);
                    sleep(3);
                }
            } else {
                ballX++;
                usleep(50000);
            }
        }else if(smer == 5) {
            if (ballX == 1) {
                pthread_mutex_lock(d->mutex);
                if (ballY == d->paddleServer || ballY == d->paddleServer + 1 || ballY == d->paddleServer + 2) {
                    pthread_mutex_unlock(d->mutex);
                    smer = 2;
                } else {
                    ballY = WHEIGHT / 2;
                    ballX = WWIDTH / 2;
                    d->ballY = ballY;
                    d->ballX = ballX;
                    pthread_mutex_unlock(d->mutex);
                    sleep(3);
                }
            } else {
                ballX--;
                usleep(50000);
            }
        }
        }
    return 0;
}

void* prenos_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;

    int sockfd, newsockfd; //deskriptory servera a klienta
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr; // adresa klienta a servera
    int n;
    char buffer[256];

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

        bzero(buffer,256);
        n = read(newsockfd, buffer, 255);
        //printf("server precital %s,%d\n",buffer,atoi(&buffer[0]));
        pthread_mutex_lock(d->mutex);
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
    //1. paddleClient,2.paddleServer,3ballx,4.bally,5scoreClient,6scoreServer,7koniec
        bzero(buffer,256);
        sprintf(&buffer[0], "%d %d %d %d ",d->paddleClient,d->paddleServer,d->ballY,d->ballX);
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

void displayPaddle(WINDOW * win, int y, int x) {
    char * paddle = "#";
    mvwprintw(win, y, x, paddle);
    mvwprintw(win, y+1, x, paddle);
    mvwprintw(win, y+2, x, paddle);
}

void* zobraz_func(void* data) {

    struct dataClient *d = (struct dataClient *) data;

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

    pthread_mutex_lock(d->mutex);
    int yServer = d->paddleServer;
    int yClient = d->paddleClient;
    int ballY = d->ballY;
    int ballX = d->ballX;

    while(!d->koniecHry) {
        pthread_mutex_unlock(d->mutex);

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
        pthread_mutex_unlock(d->mutex);

        char * ball = "o";
        mvwprintw(win, ballY, ballX, ball);
        displayPaddle(win, yServer, xServer);
        displayPaddle(win, yClient, xClient);

        wrefresh(win);


        int key=0;
        //key = wgetch(win);
        wtimeout(win,100);
        switch(wgetch(win)){
            case KEY_UP: {
                if (yServer - 1 > 0) {
                    for(int i = 1 ; i <=8;i++){
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
                if (yServer + 1 < 7) {
                    for(int i = 1 ; i <=8;i++){
                        mvwaddch(win, i, xServer, ' ');
                    }
                    wrefresh(win);
                    yServer++;
                    pthread_mutex_lock(d->mutex);
                    d->paddleServer++;
                    pthread_mutex_unlock(d->mutex);
                }
            }break;
            default: break;
        }

        //while(key==0 && yServer==d->paddleServer && yClient==d->paddleClient){
        //    key = wgetch(win);
        //    pthread_cond_wait(d->condZobraz,d->mutex);
        //}


        /*if (key == KEY_UP) {
            if (yServer - 1 > 0) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xServer, ' ');
                }
                wrefresh(win);
                yServer--;
                pthread_mutex_lock(d->mutex);
                d->paddleServer--;
                pthread_mutex_unlock(d->mutex);
            }


        } else if (key == KEY_DOWN) {

            if (yServer + 1 < 7) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xServer, ' ');
                }
                wrefresh(win);
                yServer++;
                pthread_mutex_lock(d->mutex);
                d->paddleServer++;
                pthread_mutex_unlock(d->mutex);
            }
        }
        key=-1;*/
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

    int port = atoi(argv[1]);

    struct dataClient d ={&mutex,&cond,0,0,1,1,0,0,0,port};

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


