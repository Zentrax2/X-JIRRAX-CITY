#include <pspkernel.h>
#include "callbacks.h"
#include "input.h"
#include "render.h"
#include "camera.h"
#include "world.h"
#include "npc.h"
#include "vehicle.h"

PSP_MODULE_INFO("X-JIRRAX CITY", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

int main(void)
{
    Vehicle car;
    InputState input;

    SetupCallbacks();
    inputInit();
    renderInit();
    npcInit();
    vehicleReset(&car);

    while (1) {
        inputPoll(&input);
        if (input.reset) {
            vehicleReset(&car);
        } else {
            vehicleUpdate(&car, &input);
        }
        npcUpdate();

        renderBeginFrame();
        cameraChase(car.x, car.z, car.yaw);
        worldDrawVisible(car.x, car.z);
        npcDraw(car.x, car.z);
        vehicleDraw(&car);
        renderEndFrame();
    }

    return 0;
}
