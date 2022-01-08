#ifndef SEMKAPONG_CLIENT_H
#define SEMKAPONG_CLIENT_H

void displayPaddle(WINDOW * win, int y, int x);
void displayBall(WINDOW * win, int y, int x);
void* prenos_func (void* data);
void* plocha_func(void* data);

#endif //SEMKAPONG_CLIENT_H
