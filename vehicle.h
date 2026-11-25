#ifndef VEHICLE_H
#define VEHICLE_H

#include "input.h"

typedef struct {
    float x, z;     /* world position on the ground plane */
    float yaw;      /* heading in radians; forward = (sin yaw, 0, cos yaw) */
    float speed;    /* signed speed in world units per frame */
} Vehicle;

void vehicleReset(Vehicle *v);
void vehicleUpdate(Vehicle *v, const InputState *in);
void vehicleDraw(const Vehicle *v);

#endif
