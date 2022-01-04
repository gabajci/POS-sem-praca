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


//client
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


void* prenos_func (void* data) {
    struct dataClient *d = (struct dataClient *) data;

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent* server;

    char buffer[256];

    server = gethostbyname("localhost");
    if (server == NULL)
    {
        fprintf(stderr, "Chyba, server sa nenašiel.\n");
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
        perror("Chyba pri vytvorení socketu.");
        return 0;
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Chyba pri pripojení socketu.");
        return 0;
    }

    pthread_mutex_lock(d->mutex);
    while(!d->koniecHry) {

        bzero(buffer,256);
        sprintf(&buffer[0], "%d ", d->paddleClient);
        pthread_mutex_unlock(d->mutex);

        n = write(sockfd, buffer, strlen(buffer));
        //printf("client zapisal %s\n",buffer);
        if (n < 0)
        {
            perror("Chyba pri zápise do socketu.");
            return 0;
        }


        bzero(buffer,256);
        n = read(sockfd, buffer, 255);
        printf("client vycital %s,%d\n",buffer,atoi(&buffer[7]));
        pthread_mutex_lock(d->mutex);

        //1. paddleClient,2.paddleServer,3ballx,4.bally,5scoreClient,6scoreServer,7koniec
        if(atoi(&buffer[0])!=d->paddleServer){
            d->paddleServer=atoi(&buffer[0]);
        }
        //sscanner
        if(atoi(&buffer[2])!=d->paddleServer){
            d->paddleServer=atoi(&buffer[2]);
        }
        //su tam medzeri,posunut o1 dorobit atoi!!!!!!

        if(atoi(&buffer[4])!=d->ballX){

            printf("pridavam 4 %d\n",atoi(&buffer[4]));
            d->ballX=atoi(&buffer[4]);
        }
        if(atoi(&buffer[7])!=d->ballY){
            printf("pridavam 7 %d\n",atoi(&buffer[7]));
            d->ballY=atoi(&buffer[7]);
        }
        /*if(buffer[5]!=d->scoreClient){
            d->scoreClient=buffer[2];
            //signal zobrazeniu.
        }
        if(buffer[6]!=d->scoreServer){
            d->scoreServer=buffer[6];
            //signal zobrazeniu.
        }
        if(buffer[7]!=d->koniecHry){
            d->koniecHry=buffer[7];
        }*/
        pthread_mutex_unlock(d->mutex);

        if (n < 0)
        {
            perror("Chyba pri čítaní zo socketu");
            return 0;
        }
        pthread_mutex_lock(d->mutex);
    }
    pthread_mutex_unlock(d->mutex);
    close(sockfd);

    return 0;

}

void displayPaddle(WINDOW * win, int y, int x) {
    char * paddle = "#";
    mvwprintw(win, y, x, paddle);
    mvwprintw(win, y+1, x, paddle);
    mvwprintw(win, y+2, x, paddle);
}

void* plocha_func(void* data) {

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
                if (yClient - 1 > 0) {
                    for (int i = 1; i <= 8; i++) {
                        mvwaddch(win, i, xClient, ' ');
                    }
                    wrefresh(win);
                    yClient--;
                    pthread_mutex_lock(d->mutex);
                    d->paddleClient--;
                    pthread_mutex_unlock(d->mutex);
                }
            } break;
            case KEY_DOWN: {
                if (yClient + 1 < 7) {
                    for(int i = 1 ; i <=8;i++){
                        mvwaddch(win, i, xClient, ' ');
                    }
                    wrefresh(win);
                    yClient++;
                    pthread_mutex_lock(d->mutex);
                    d->paddleClient++;
                    pthread_mutex_unlock(d->mutex);
                }
            }break;
            default: break;
        }
        //while(key==0 && yServer==d->paddleServer && yClient==d->paddleClient){
        //    key = wgetch(win);
        //    pthread_cond_wait(d->condZobraz,d->mutex);
        //}

        /*
        if (key == KEY_UP) {
            if (yClient - 1 > 0) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xClient, ' ');
                }
                wrefresh(win);
                yClient--;
                pthread_mutex_lock(d->mutex);
                d->paddleClient--;
                pthread_mutex_unlock(d->mutex);
            }


        } else if (key == KEY_DOWN) {

            if (yClient + 1 < 7) {
                for(int i = 1 ; i <=8;i++){
                    mvwaddch(win, i, xClient, ' ');
                }
                wrefresh(win);
                yClient++;
                pthread_mutex_lock(d->mutex);
                d->paddleClient++;
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

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);

    int port = atoi(argv[1]);


    struct dataClient d ={&mutex,&cond,0,0,1,1,0,0,0,port};

    pthread_create(&zobraz,NULL,&plocha_func,&d);
    pthread_create(&prenos,NULL,&prenos_func,&d);

    pthread_join(zobraz,NULL);
    pthread_join(prenos,NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}


