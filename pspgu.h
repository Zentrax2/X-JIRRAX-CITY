/* STUB header for syntax checking only. */
#ifndef PSPSTUB_GU_H
#define PSPSTUB_GU_H
#define GU_DIRECT 0
#define GU_TRUE 1
#define GU_FALSE 0
#define GU_PSM_8888 3
#define GU_SCISSOR_TEST 1
#define GU_DEPTH_TEST 2
#define GU_CULL_FACE 3
#define GU_FOG 4
#define GU_GEQUAL 4
#define GU_CW 0
#define GU_SMOOTH 1
#define GU_TRIANGLES 3
#define GU_COLOR_8888 0x0300
#define GU_VERTEX_32BITF 0x0003
#define GU_TRANSFORM_3D 0x0000
#define GU_COLOR_BUFFER_BIT 1
#define GU_DEPTH_BUFFER_BIT 2
#define GU_PROJECTION 0
#define GU_VIEW 1
#define GU_MODEL 2
void sceGuInit(void);
void sceGuTerm(void);
void sceGuStart(int cid, void *list);
void sceGuFinish(void);
void sceGuSync(int mode, int what);
void sceGuDisplay(int state);
void sceGuSwapBuffers(void);
void sceGuDrawBuffer(int psm, void *fbp, int fbw);
void sceGuDispBuffer(int width, int height, void *dispbp, int dispbw);
void sceGuDepthBuffer(void *zbp, int zbw);
void sceGuOffset(unsigned int x, unsigned int y);
void sceGuViewport(int cx, int cy, int width, int height);
void sceGuDepthRange(int zmin, int zmax);
void sceGuScissor(int x, int y, int w, int h);
void sceGuEnable(int state);
void sceGuDisable(int state);
void sceGuDepthFunc(int function);
void sceGuFrontFace(int order);
void sceGuShadeModel(int mode);
void sceGuFog(float zmin, float zmax, unsigned int color);
void sceGuClearColor(unsigned int color);
void sceGuClearDepth(unsigned int depth);
void sceGuClear(int mode);
#endif
