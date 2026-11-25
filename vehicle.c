#include <math.h>
#include "render.h"
#include "vehicle.h"
#include "world.h"

#define ACCEL_STEP        0.012f
#define COAST_DRAG        0.985f
#define BRAKE_DRAG        0.94f
#define MAX_SPEED         0.42f
#define MAX_REVERSE       (-0.15f)
#define STEER_BASE        0.025f
#define STEER_SPEED_GAIN  0.10f
#define PI_F              3.14159265f
#define TWO_PI_F          6.28318531f

void vehicleReset(Vehicle *v)
{
    v->x = WORLD_CENTER;     /* mulai di persimpangan pusat kota */
    v->z = WORLD_CENTER;
    v->yaw = 0.0f;
    v->speed = 0.0f;
}

void vehicleUpdate(Vehicle *v, const InputState *in)
{
    float slow;

    if (in->accel) {
        v->speed += ACCEL_STEP;
    } else {
        v->speed *= COAST_DRAG;
    }
    if (in->brake) {
        v->speed *= BRAKE_DRAG;
    }
    if (v->speed > MAX_SPEED) v->speed = MAX_SPEED;
    if (v->speed < MAX_REVERSE) v->speed = MAX_REVERSE;

    v->yaw += in->steer * (STEER_BASE + fabsf(v->speed) * STEER_SPEED_GAIN);
    if (v->yaw > PI_F) v->yaw -= TWO_PI_F;
    else if (v->yaw < -PI_F) v->yaw += TWO_PI_F;

    slow = worldPotholeSlow(v->x, v->z);   /* lubang jalan memperlambat mobil */
    v->x += sinf(v->yaw) * v->speed * slow;
    v->z += cosf(v->yaw) * v->speed * slow;

    worldClampToRoad(&v->x, &v->z);        /* mobil hanya bisa di badan jalan */
}

void vehicleDraw(const Vehicle *v)
{
    float fwdX = sinf(v->yaw);
    float fwdZ = cosf(v->yaw);
    float bodyY = ROAD_TOP + 0.325f;   /* dasar badan mobil tepat di permukaan jalan */

    renderCube(MAT_CAR_BODY, v->x, bodyY, v->z, 2.4f, 0.65f, 4.0f, v->yaw);
    renderCube(MAT_CAR_CABIN, v->x + fwdX * 0.1f, bodyY + 0.53f, v->z + fwdZ * 0.1f,
               1.75f, 0.65f, 2.0f, v->yaw);
    renderCube(MAT_BUMPER, v->x + fwdX * 2.0f, bodyY - 0.13f, v->z + fwdZ * 2.0f,
               2.35f, 0.22f, 0.22f, v->yaw);
    renderCube(MAT_BUMPER, v->x - fwdX * 2.0f, bodyY - 0.13f, v->z - fwdZ * 2.0f,
               2.35f, 0.22f, 0.22f, v->yaw);
}
