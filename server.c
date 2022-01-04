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
        printf("server precital %s,%d\n",buffer,atoi(&buffer[0]));
        pthread_mutex_lock(d->mutex);
        if(d->paddleClient!=atoi(&buffer[0])) {
            d->paddleClient=atoi(&buffer[0]);
        }

        pthread_mutex_unlock(d->mutex);
        if (n < 0) {
            perror("Chyba pri načítaní socketu.");
            return 0;
        }

        pthread_mutex_lock(d->mutex);
    //1. paddleClient,2.paddleServer,3ballx,4.bally,5scoreClient,6scoreServer,7koniec
        bzero(buffer,256);
        sprintf(&buffer[0], "%d %d ",d->paddleClient,d->paddleServer);
        pthread_mutex_unlock(d->mutex);

        n = write(newsockfd, buffer, strlen(buffer));
        printf("server zapisal %s\n",buffer);
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

    while(!d->koniecHry) {
        pthread_mutex_unlock(d->mutex);
        displayPaddle(win, yServer, xServer);

        displayPaddle(win, yClient, xClient);


        wrefresh(win);
        pthread_mutex_lock(d->mutex);
        yServer = d->paddleServer;
        yClient = d->paddleClient;

        pthread_mutex_unlock(d->mutex);
        int key=0;
        key = wgetch(win);
        if (key == KEY_UP) {
            if (yServer - 1 > 0) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xServer, ' ');
                }
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
                yServer++;
                pthread_mutex_lock(d->mutex);
                d->paddleServer++;
                pthread_mutex_unlock(d->mutex);
            }
        }
        key=-1;
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


