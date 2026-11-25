#ifndef NPC_H
#define NPC_H

#define NPC_COUNT          64
#define NPC_SPEED          0.02f   /* unit per frame: ~1.2 unit/detik, kecepatan jalan kaki */
#define NPC_FRAMES_PER_MIN 60      /* 1 menit dalam game = 1 detik nyata */

typedef enum {
    ACT_WALK = 0,
    ACT_WORK,
    ACT_EAT,
    ACT_CHAT,
    ACT_PRAY,
    ACT_HOME
} NpcActivity;

typedef struct {
    float x, z;
    int act;
    int hidden;
} NpcView;

void npcInit(void);
void npcUpdate(void);
void npcDraw(float camX, float camZ);
int  npcGetMinute(void);
void npcGetView(int i, NpcView *v);

#endif
