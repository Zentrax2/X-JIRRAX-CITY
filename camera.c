#include <math.h>
#include <pspgu.h>
#include <pspgum.h>
#include "camera.h"

#define CAM_DISTANCE     10.0f
#define CAM_HEIGHT       6.0f
#define CAM_LOOK_HEIGHT  0.8f

void cameraChase(float targetX, float targetZ, float yaw)
{
    ScePspFVector3 target = {targetX, CAM_LOOK_HEIGHT, targetZ};
    ScePspFVector3 eye = {
        targetX - sinf(yaw) * CAM_DISTANCE,
        CAM_HEIGHT,
        targetZ - cosf(yaw) * CAM_DISTANCE
    };
    ScePspFVector3 up = {0.0f, 1.0f, 0.0f};

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&eye, &target, &up);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
}
