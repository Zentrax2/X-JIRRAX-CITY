#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include "render.h"

#define BUF_WIDTH        512
#define SCR_WIDTH        480
#define SCR_HEIGHT       272
#define DISP_BUF_ADDR    0x88000    /* VRAM offset after the 512x272x4 frame buffer */
#define DEPTH_BUF_ADDR   0x110000   /* VRAM offset after the display buffer */
#define LIST_WORDS       131072     /* 512 KB display list */
#define CLEAR_COLOR      0xffb77b4d /* GU ABGR: sky blue */
#define FOG_NEAR         60.0f
#define FOG_FAR          150.0f
#define FAR_PLANE        170.0f

typedef struct {
    unsigned int color;   /* GU_COLOR_8888 (ABGR) */
    float x, y, z;
} Vertex;

static unsigned int __attribute__((aligned(16))) gList[LIST_WORDS];
static Vertex __attribute__((aligned(16))) gCubeCache[MAT_COUNT][36];

/* Unit cube, 6 faces x 2 triangles x 3 vertices. Face order: front, back, left, right, top, bottom. */
static const float kCubePos[36][3] = {
    {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
    {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}
};

/* Material colors written as 0xAARRGGBB, converted to GU's ABGR when baked. */
static const unsigned int kMaterialArgb[MAT_COUNT] = {
    [MAT_GROUND_EARTH]    = 0xff7a6a4a,
    [MAT_GROUND_CONCRETE] = 0xff8c8c88,
    [MAT_GRASS]           = 0xff4d7a3a,
    [MAT_ROAD]            = 0xff3a3d42,
    [MAT_ROAD_PATCH]      = 0xff585b61,
    [MAT_POTHOLE]         = 0xff17181b,
    [MAT_LANE]            = 0xffe8e2b0,
    [MAT_ZEBRA]           = 0xffe9e9e9,
    [MAT_SIDEWALK]        = 0xffa8a39a,
    [MAT_WALL_A]          = 0xffd9b98c,
    [MAT_WALL_B]          = 0xffe8e0d0,
    [MAT_WALL_C]          = 0xffc98f7a,
    [MAT_WALL_WHITE]      = 0xfff2f2f2,
    [MAT_WALL_ABANDONED]  = 0xff6e6a62,
    [MAT_ROOF_TILE]       = 0xffa34b2e,
    [MAT_ROOF_TIN]        = 0xff9aa3a8,
    [MAT_ROOF_CONCRETE]   = 0xff8f8f8f,
    [MAT_WARUNG_WALL]     = 0xffe0c890,
    [MAT_AWNING_RED]      = 0xffc0392b,
    [MAT_TABLE]           = 0xff8b6a40,
    [MAT_CHAIR]           = 0xffd93a3a,
    [MAT_FENCE]           = 0xff6b5a48,
    [MAT_TREE_TRUNK]      = 0xff5a3d28,
    [MAT_TREE_CROWN]      = 0xff2f6b2a,
    [MAT_POLE]            = 0xff7f7f7f,
    [MAT_MASJID_WALL]     = 0xfff7f5ea,
    [MAT_MASJID_DOME]     = 0xff1f8a4c,
    [MAT_PASAR_ROOF]      = 0xff6a8fa0,
    [MAT_RUKO_A]          = 0xffe8a33d,
    [MAT_RUKO_B]          = 0xff4fa3c7,
    [MAT_RUKO_C]          = 0xffd96b8a,
    [MAT_TANK]            = 0xff2e5e8c,
    [MAT_MOTOR]           = 0xff2a2a2a,
    [MAT_CAR_BODY]        = 0xff253bce,
    [MAT_CAR_CABIN]       = 0xff242a35,
    [MAT_BUMPER]          = 0xffbbbbbb,
    [MAT_SKIN_1]          = 0xffe0b08a,
    [MAT_SKIN_2]          = 0xffc68a5c,
    [MAT_SKIN_3]          = 0xff8d5a3b,
    [MAT_SHIRT_1]         = 0xffe84a4a,
    [MAT_SHIRT_2]         = 0xff3a7bd5,
    [MAT_SHIRT_3]         = 0xff2e9e5b,
    [MAT_SHIRT_4]         = 0xffffd23f,
    [MAT_SHIRT_5]         = 0xffffffff,
    [MAT_SHIRT_6]         = 0xff7a4fa8,
    [MAT_PANTS]           = 0xff3b3b48,
    [MAT_HIJAB]           = 0xff2c4a6e,
    [MAT_HAIR]            = 0xff1a1a1a
};

/* Per-face brightness 0..255: front, back, left, right, top, bottom. */
static const unsigned int kFaceShade[6] = {255, 176, 208, 144, 238, 119};

static unsigned int bakeColor(unsigned int argb, unsigned int shade)
{
    unsigned int r = ((argb >> 16) & 0xffu) * shade / 255u;
    unsigned int g = ((argb >> 8) & 0xffu) * shade / 255u;
    unsigned int b = (argb & 0xffu) * shade / 255u;
    return 0xff000000u | (b << 16) | (g << 8) | r;
}

static void bakeCubeCache(void)
{
    for (unsigned int m = 0; m < MAT_COUNT; ++m) {
        for (unsigned int v = 0; v < 36; ++v) {
            gCubeCache[m][v].color = bakeColor(kMaterialArgb[m], kFaceShade[v / 6]);
            gCubeCache[m][v].x = kCubePos[v][0];
            gCubeCache[m][v].y = kCubePos[v][1];
            gCubeCache[m][v].z = kCubePos[v][2];
        }
    }
    /* The GE reads vertex data straight from RAM, so flush the CPU cache first. */
    sceKernelDcacheWritebackInvalidateAll();
}

void renderInit(void)
{
    bakeCubeCache();

    sceGuInit();
    sceGuStart(GU_DIRECT, gList);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)DISP_BUF_ADDR, BUF_WIDTH);
    sceGuDepthBuffer((void*)DEPTH_BUF_ADDR, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);
    sceGuDisable(GU_CULL_FACE);
    /* Distance fog hides chunk pop-in at the edge of the streamed area. */
    sceGuEnable(GU_FOG);
    sceGuFog(FOG_NEAR, FOG_FAR, CLEAR_COLOR);
    sceGuFinish();
    sceGuSync(0, 0);
    sceGuDisplay(GU_TRUE);
}

void renderBeginFrame(void)
{
    sceGuStart(GU_DIRECT, gList);
    sceGuClearColor(CLEAR_COLOR);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(55.0f, 16.0f / 9.0f, 0.5f, FAR_PLANE);
}

void renderEndFrame(void)
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

void renderCube(Material mat, float x, float y, float z,
                float sx, float sy, float sz, float yaw)
{
    ScePspFVector3 p = {x, y, z};
    ScePspFVector3 s = {sx, sy, sz};

    if ((unsigned int)mat >= MAT_COUNT) {
        return;
    }

    sceGumPushMatrix();
    sceGumTranslate(&p);
    sceGumRotateY(yaw);
    sceGumScale(&s);
    sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    36, 0, gCubeCache[mat]);
    sceGumPopMatrix();
}
