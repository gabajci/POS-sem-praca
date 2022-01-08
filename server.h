#ifndef SEMKAPONG_SERVER_H
#define SEMKAPONG_SERVER_H

void displayPaddle(WINDOW * win, int y, int x);
void displayBall(WINDOW * win, int y, int x);
int getZacSmer(int sucetSkore);
void *logika_func (void* data);
void* prenos_func (void* data);
void* plocha_func(void* data);

#endif //SEMKAPONG_SERVER_H
