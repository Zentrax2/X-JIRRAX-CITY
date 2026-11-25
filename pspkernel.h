/* STUB header for syntax checking only. Not the real PSPSDK. */
#ifndef PSPSTUB_KERNEL_H
#define PSPSTUB_KERNEL_H
#include <stddef.h>
typedef int SceUID;
typedef unsigned int SceSize;
typedef int (*SceKernelCallbackFunction)(int arg1, int arg2, void *common);
typedef int (*SceKernelThreadEntry)(SceSize args, void *argp);
#define PSP_MODULE_INFO(name, attr, major, minor) static const char gPspModuleInfo[] __attribute__((unused)) = name
#define THREAD_ATTR_USER 0x80000000u
#define THREAD_ATTR_VFPU 0x00004000u
#define PSP_MAIN_THREAD_ATTR(attr) static const int gPspMainAttr __attribute__((unused)) = (attr)
SceUID sceKernelCreateCallback(const char *name, SceKernelCallbackFunction func, void *arg);
int sceKernelRegisterExitCallback(SceUID cbid);
int sceKernelSleepThreadCB(void);
SceUID sceKernelCreateThread(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, unsigned int attr, void *option);
int sceKernelStartThread(SceUID thid, SceSize arglen, void *argp);
void sceKernelExitGame(void);
void sceKernelDcacheWritebackInvalidateAll(void);
#endif
