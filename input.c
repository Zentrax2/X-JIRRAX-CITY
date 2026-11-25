#include <math.h>
#include <pspctrl.h>
#include "input.h"

#define STICK_DEADZONE 0.15f

static float applyDeadzone(float v)
{
    float a = fabsf(v);
    float s;
    if (a < STICK_DEADZONE) {
        return 0.0f;
    }
    s = (a - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
    return (v < 0.0f) ? -s : s;
}

void inputInit(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void inputPoll(InputState *in)
{
    SceCtrlData pad;
    float stick;

    sceCtrlReadBufferPositive(&pad, 1);

    in->accel = (pad.Buttons & PSP_CTRL_CROSS) != 0;
    in->brake = (pad.Buttons & PSP_CTRL_SQUARE) != 0;
    in->reset = (pad.Buttons & PSP_CTRL_START) != 0;

    /* D-pad steering plus analog stick (Lx, centre = 128). */
    in->steer = 0.0f;
    if (pad.Buttons & PSP_CTRL_LEFT) in->steer -= 1.0f;
    if (pad.Buttons & PSP_CTRL_RIGHT) in->steer += 1.0f;

    stick = ((float)pad.Lx - 128.0f) / 127.0f;
    if (stick > 1.0f) stick = 1.0f;
    if (stick < -1.0f) stick = -1.0f;
    in->steer += applyDeadzone(stick);

    if (in->steer > 1.0f) in->steer = 1.0f;
    if (in->steer < -1.0f) in->steer = -1.0f;
}
