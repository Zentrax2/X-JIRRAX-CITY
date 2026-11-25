#include <math.h>
#include <string.h>
#include "npc.h"
#include "render.h"
#include "world.h"

#define NPC_MAX_PATH 96
#define NPC_NONE     0xffffu

typedef enum {
    JOB_VENDOR = 0,
    JOB_WORKER,
    JOB_HOMEMAKER,
    JOB_RETIREE,
    JOB_OJEK
} Job;

typedef struct {
    unsigned short cur;        /* sudut trotoar tempat NPC berada / baru dilewati */
    unsigned short next;       /* sudut tujuan ruas yang sedang dilalui, atau NPC_NONE */
    unsigned short dest;       /* tujuan akhir */
    unsigned short home, warung, masjid, pos, work;
    unsigned short path[NPC_MAX_PATH];
    unsigned char pathLen;
    unsigned char pathPos;
    unsigned char act;
    unsigned char job;
    unsigned char religious;
    unsigned char hijab;
    unsigned char hidden;      /* di dalam rumah */
    Material skin;
    Material shirt;
    unsigned int rng;
    float dist;                /* jarak yang sudah ditempuh pada ruas sekarang */
    float x, z, yaw;
    float jitter;              /* geser jam tidur/kerja per NPC, dalam jam */
    float phase;
} Npc;

static Npc gNpc[NPC_COUNT];
static unsigned short gPoi[ROLE_COUNT][CORNERS_TOTAL];
static int gPoiCount[ROLE_COUNT];
static unsigned int gFrame;
static int gMinute;            /* menit dalam sehari, 0..1439 */

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static unsigned int nextRand(Npc *n)
{
    n->rng ^= n->rng << 13;
    n->rng ^= n->rng >> 17;
    n->rng ^= n->rng << 5;
    return n->rng;
}

static unsigned short randomCorner(Npc *n)
{
    return (unsigned short)(nextRand(n) % CORNERS_TOTAL);
}

static float currentHour(const Npc *n)
{
    float t = (float)gMinute / 60.0f + n->jitter;
    while (t < 0.0f) t += 24.0f;
    while (t >= 24.0f) t -= 24.0f;
    return t;
}

static unsigned short pickPoi(int role, unsigned int i, unsigned int salt)
{
    unsigned int cnt = (unsigned int)gPoiCount[role];
    if (cnt == 0u) return 0;
    return gPoi[role][worldHash(i, salt, 9u) % cnt];
}

/* Rute terpendek di graf trotoar (BFS). Dipanggil saat tujuan berubah, bukan tiap frame. */
static void npcSetDest(Npc *n, unsigned short dest)
{
    static unsigned char seen[CORNERS_TOTAL];
    static unsigned short prev[CORNERS_TOTAL];
    static unsigned short queue[CORNERS_TOTAL];
    static unsigned short rev[CORNERS_TOTAL];
    int head = 0, tail = 0, found = 0, r = 0, i, k, m, nb[4];
    unsigned short start;

    n->dest = dest;
    n->pathLen = 0;
    n->pathPos = 0;
    start = (n->next != NPC_NONE) ? n->next : n->cur;
    if (start == dest) return;

    memset(seen, 0, sizeof seen);
    seen[start] = 1;
    queue[tail++] = start;
    while (head < tail && !found) {
        int c = queue[head++];
        m = worldCornerNeighbors(c, nb);
        for (k = 0; k < m; ++k) {
            if (!seen[nb[k]]) {
                seen[nb[k]] = 1;
                prev[nb[k]] = (unsigned short)c;
                if (nb[k] == dest) { found = 1; break; }
                queue[tail++] = (unsigned short)nb[k];
            }
        }
    }
    if (!found) return;

    /* Rekonstruksi dari tujuan ke awal, lalu dibalik. */
    for (i = dest; i != start && r < CORNERS_TOTAL; i = prev[i]) {
        rev[r++] = (unsigned short)i;
    }
    n->pathLen = (unsigned char)(r < NPC_MAX_PATH ? r : NPC_MAX_PATH);
    for (i = 0; i < n->pathLen; ++i) {
        n->path[i] = rev[r - 1 - i];
    }
}

static void npcStep(Npc *n, float step)
{
    while (step > 0.0f) {
        float ax, az, bx, bz, len, left;
        if (n->next == NPC_NONE) {
            if (n->pathPos >= n->pathLen) return;
            n->next = n->path[n->pathPos++];
            n->dist = 0.0f;
        }
        worldCornerPos(n->cur, &ax, &az);
        worldCornerPos(n->next, &bx, &bz);
        len = fabsf(bx - ax) + fabsf(bz - az);
        left = len - n->dist;
        if (step < left) {
            n->dist += step;
            return;
        }
        step -= left;
        n->cur = n->next;
        n->next = NPC_NONE;
        n->dist = 0.0f;
    }
}

static void npcPosition(Npc *n)
{
    float ax, az, bx, bz, len, t;
    worldCornerPos(n->cur, &ax, &az);
    if (n->next == NPC_NONE) {
        n->x = ax;
        n->z = az;
        return;
    }
    worldCornerPos(n->next, &bx, &bz);
    len = fabsf(bx - ax) + fabsf(bz - az);
    t = (len > 0.0f) ? n->dist / len : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    n->x = ax + (bx - ax) * t;
    n->z = az + (bz - az) * t;
    if (bx != ax || bz != az) {
        n->yaw = atan2f(bx - ax, bz - az);
    }
}

/* Jadwal harian. t = jam dalam sehari (0..24). Keluar: tujuan dan aktivitas. */
static void desiredFor(const Npc *n, float t, unsigned short *dest, int *act)
{
    *dest = n->home;
    *act = ACT_HOME;
    if (t < 4.5f || t >= 21.0f) return;

    if (n->religious &&
        ((t >= 4.5f && t < 5.0f) || (t >= 11.9f && t < 12.3f) ||
         (t >= 15.2f && t < 15.6f) || (t >= 18.0f && t < 18.4f) ||
         (t >= 19.2f && t < 19.6f))) {
        *dest = n->masjid;
        *act = ACT_PRAY;
        return;
    }

    switch (n->job) {
    case JOB_VENDOR:
        if (t >= 5.5f) { *dest = n->warung; *act = ACT_WORK; }
        break;
    case JOB_WORKER:
        if (t >= 7.0f && t < 12.0f)       { *dest = n->work;   *act = ACT_WORK; }
        else if (t >= 12.0f && t < 13.0f) { *dest = n->warung; *act = ACT_EAT; }
        else if (t >= 13.0f && t < 17.0f) { *dest = n->work;   *act = ACT_WORK; }
        else if (t >= 17.0f && t < 19.0f) { *dest = n->dest;   *act = ACT_WALK; }
        else if (t >= 19.0f && t < 21.0f) { *dest = n->warung; *act = ACT_CHAT; }
        break;
    case JOB_HOMEMAKER:
        if (t >= 6.0f && t < 8.5f)        { *dest = n->work;   *act = ACT_WORK; }
        else if (t >= 15.0f && t < 17.0f) { *dest = n->warung; *act = ACT_CHAT; }
        break;
    case JOB_RETIREE:
        if (t >= 6.0f && t < 7.0f)        { *dest = n->warung; *act = ACT_EAT; }
        else if (t >= 7.0f && t < 11.0f)  { *dest = n->pos;    *act = ACT_CHAT; }
        else if (t >= 11.0f && t < 12.5f) { *dest = n->warung; *act = ACT_EAT; }
        else if (t >= 12.5f && t < 15.0f) { *dest = n->pos;    *act = ACT_CHAT; }
        else if (t >= 15.0f && t < 18.0f) { *dest = n->warung; *act = ACT_CHAT; }
        break;
    case JOB_OJEK:
    default:
        if (t >= 6.0f && t < 9.0f)        { *dest = n->pos;    *act = ACT_CHAT; }
        else if (t >= 9.0f && t < 17.0f)  { *dest = n->dest;   *act = ACT_WALK; }
        else if (t >= 17.0f && t < 20.0f) { *dest = n->pos;    *act = ACT_CHAT; }
        break;
    }
}

static void applySchedule(Npc *n, float t)
{
    unsigned short dest;
    int act;
    desiredFor(n, t, &dest, &act);
    if (act != n->act) {
        n->act = (unsigned char)act;
        if (act == ACT_WALK) dest = randomCorner(n);
    }
    if (act != ACT_HOME) n->hidden = 0;
    if (dest != n->dest) npcSetDest(n, dest);
}

static void npcTick(Npc *n)
{
    int arrived;
    if (n->hidden) return;
    npcStep(n, NPC_SPEED);
    npcPosition(n);
    arrived = (n->next == NPC_NONE && n->pathPos >= n->pathLen);
    if (!arrived) return;
    if (n->cur != n->dest) {
        npcSetDest(n, n->dest);            /* rute terpotong: hitung ulang */
    } else if (n->act == ACT_WALK) {
        npcSetDest(n, randomCorner(n));    /* berkeliling */
    } else if (n->act == ACT_HOME) {
        n->hidden = 1;                     /* sudah di dalam rumah */
    }
}

void npcInit(void)
{
    int c, i, r;
    for (r = 0; r < ROLE_COUNT; ++r) gPoiCount[r] = 0;
    for (c = 0; c < CORNERS_TOTAL; ++c) {
        r = (int)worldCornerRole(c);
        if (r != ROLE_NONE) gPoi[r][gPoiCount[r]++] = (unsigned short)c;
    }

    gFrame = 0u;
    gMinute = 6 * 60;

    for (i = 0; i < NPC_COUNT; ++i) {
        Npc *n = &gNpc[i];
        unsigned int h = worldHash((unsigned int)i, 0x51u, 0x77u);
        unsigned int jr = h % 100u;

        n->rng = h | 1u;
        n->job = (unsigned char)(jr < 25u ? JOB_VENDOR : jr < 55u ? JOB_WORKER :
                                 jr < 75u ? JOB_HOMEMAKER : jr < 90u ? JOB_RETIREE : JOB_OJEK);
        n->religious = (unsigned char)(((h >> 8) % 10u) < 8u);
        n->hijab = (unsigned char)((n->job == JOB_HOMEMAKER) || (((h >> 12) % 10u) < 3u));
        n->skin = (Material)(MAT_SKIN_1 + (h >> 16) % 3u);
        n->shirt = (Material)(MAT_SHIRT_1 + (h >> 20) % 6u);

        n->home = pickPoi(ROLE_HOME, (unsigned int)i, 1u);
        n->warung = pickPoi(ROLE_WARUNG, (unsigned int)i, 2u);
        n->masjid = pickPoi(ROLE_MASJID, (unsigned int)i, 3u);
        n->pos = pickPoi(ROLE_POS, (unsigned int)i, 4u);
        switch (n->job) {
        case JOB_VENDOR:    n->work = n->warung; break;
        case JOB_WORKER:    n->work = pickPoi(ROLE_RUKO, (unsigned int)i, 5u); break;
        case JOB_HOMEMAKER: n->work = pickPoi(ROLE_PASAR, (unsigned int)i, 6u); break;
        default:            n->work = n->pos; break;
        }

        n->cur = n->home;
        n->dest = n->home;
        n->next = NPC_NONE;
        n->pathLen = 0;
        n->pathPos = 0;
        n->act = ACT_HOME;
        n->hidden = 0;
        n->dist = 0.0f;
        n->yaw = 0.0f;
        n->jitter = ((float)((h >> 24) % 60u) - 30.0f) / 60.0f;
        n->phase = (float)(h % 628u) * 0.01f;
        worldCornerPos(n->home, &n->x, &n->z);
    }
    for (i = 0; i < NPC_COUNT; ++i) {
        applySchedule(&gNpc[i], currentHour(&gNpc[i]));
    }
}

void npcUpdate(void)
{
    int i;
    gFrame++;
    if (gFrame % NPC_FRAMES_PER_MIN == 0u) {
        gMinute = (gMinute + 1) % 1440;
        for (i = 0; i < NPC_COUNT; ++i) {
            applySchedule(&gNpc[i], currentHour(&gNpc[i]));
        }
    }
    for (i = 0; i < NPC_COUNT; ++i) {
        npcTick(&gNpc[i]);
    }
}

static void drawNpc(const Npc *n)
{
    int moving = (n->next != NPC_NONE);
    int sitting = !moving && (n->act == ACT_EAT || n->act == ACT_CHAT);
    int kneel = !moving && (n->act == ACT_PRAY);
    float legH = sitting ? 0.42f : (kneel ? 0.34f : 0.75f);
    float bob = moving ? fabsf(sinf(n->phase + (float)gFrame * 0.25f)) * 0.06f : 0.0f;
    float torsoY = SIDEWALK_TOP + legH + 0.3f + bob;
    float headY = SIDEWALK_TOP + legH + 0.81f + bob;

    renderCube(MAT_PANTS, n->x, SIDEWALK_TOP + legH * 0.5f, n->z, 0.55f, legH, 0.4f, n->yaw);
    renderCube(n->shirt, n->x, torsoY, n->z, 0.72f, 0.6f, 0.4f, n->yaw);
    renderCube(n->skin, n->x, headY, n->z, 0.42f, 0.42f, 0.42f, n->yaw);
    if (n->hijab) {
        renderCube(MAT_HIJAB, n->x, headY + 0.08f, n->z, 0.5f, 0.3f, 0.5f, n->yaw);
    } else {
        renderCube(MAT_HAIR, n->x, headY + 0.2f, n->z, 0.44f, 0.12f, 0.44f, n->yaw);
    }
}

void npcDraw(float camX, float camZ)
{
    int ccx = clampi((int)floorf(camX / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    int ccz = clampi((int)floorf(camZ / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    int i;
    for (i = 0; i < NPC_COUNT; ++i) {
        const Npc *n = &gNpc[i];
        int cx, cz;
        if (n->hidden) continue;
        cx = (int)floorf(n->x / WORLD_BLOCK);
        cz = (int)floorf(n->z / WORLD_BLOCK);
        if (cx < ccx - WORLD_VIEW_RADIUS || cx > ccx + WORLD_VIEW_RADIUS) continue;
        if (cz < ccz - WORLD_VIEW_RADIUS || cz > ccz + WORLD_VIEW_RADIUS) continue;
        drawNpc(n);
    }
}

int npcGetMinute(void)
{
    return gMinute;
}

void npcGetView(int i, NpcView *v)
{
    const Npc *n = &gNpc[i];
    v->x = n->x;
    v->z = n->z;
    v->act = n->act;
    v->hidden = n->hidden;
}
