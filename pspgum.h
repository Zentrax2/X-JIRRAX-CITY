/* STUB header for syntax checking only. */
#ifndef PSPSTUB_GUM_H
#define PSPSTUB_GUM_H
#include "pspgu.h"
typedef struct ScePspFVector3 { float x; float y; float z; } ScePspFVector3;
void sceGumMatrixMode(int mode);
void sceGumLoadIdentity(void);
void sceGumPerspective(float fovy, float aspect, float zNear, float zFar);
void sceGumLookAt(const ScePspFVector3 *eye, const ScePspFVector3 *center, const ScePspFVector3 *up);
void sceGumPushMatrix(void);
void sceGumPopMatrix(void);
void sceGumTranslate(const ScePspFVector3 *v);
void sceGumRotateY(float angle);
void sceGumScale(const ScePspFVector3 *v);
void sceGumDrawArray(int prim, int vtype, int count, const void *indices, const void *vertices);
#endif
