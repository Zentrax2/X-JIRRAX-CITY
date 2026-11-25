/* STUB header for syntax checking only. */
#ifndef PSPSTUB_CTRL_H
#define PSPSTUB_CTRL_H
typedef struct SceCtrlData {
    unsigned int TimeStamp;
    unsigned int Buttons;
    unsigned char Lx;
    unsigned char Ly;
    unsigned char Rsrv[6];
} SceCtrlData;
#define PSP_CTRL_SELECT   0x00000001
#define PSP_CTRL_START    0x00000008
#define PSP_CTRL_UP       0x00000010
#define PSP_CTRL_RIGHT    0x00000020
#define PSP_CTRL_DOWN     0x00000040
#define PSP_CTRL_LEFT     0x00000080
#define PSP_CTRL_TRIANGLE 0x00001000
#define PSP_CTRL_CIRCLE   0x00002000
#define PSP_CTRL_CROSS    0x00004000
#define PSP_CTRL_SQUARE   0x00008000
#define PSP_CTRL_MODE_DIGITAL 0
#define PSP_CTRL_MODE_ANALOG  1
int sceCtrlReadBufferPositive(SceCtrlData *pad_data, int count);
int sceCtrlSetSamplingCycle(int cycle);
int sceCtrlSetSamplingMode(int mode);
#endif
