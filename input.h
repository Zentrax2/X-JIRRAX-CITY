#ifndef INPUT_H
#define INPUT_H

typedef struct {
    int accel;      /* Cross held */
    int brake;      /* Square held */
    int reset;      /* Start held */
    float steer;    /* -1.0 (full left) .. +1.0 (full right) */
} InputState;

void inputInit(void);
void inputPoll(InputState *in);

#endif
