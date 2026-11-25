/* Test logika di host (tanpa PSP SDK). renderCube diganti perekam panggilan. */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "render.h"
#include "world.h"
#include "npc.h"
#include "vehicle.h"

typedef struct { Material mat; float x, y, z, sx, sy, sz, yaw; } Call;
#define MAX_CALLS 16384
static Call gCalls[MAX_CALLS];
static Call gBackup[MAX_CALLS];
static int gCount;
static int gFail;

void renderCube(Material mat, float x, float y, float z,
                float sx, float sy, float sz, float yaw)
{
    Call *c;
    if (gCount >= MAX_CALLS) return;
    c = &gCalls[gCount++];
    c->mat = mat; c->x = x; c->y = y; c->z = z;
    c->sx = sx; c->sy = sy; c->sz = sz; c->yaw = yaw;
}

#define CHECK(cond, msg)                      \
    do {                                      \
        if (cond) printf("ok:   %s\n", msg);  \
        else { printf("FAIL: %s\n", msg); gFail++; } \
    } while (0)

#define NEAR(a, b, eps) (fabsf((float)(a) - (float)(b)) <= (eps))

static int isBuilding(Material m)
{
    switch (m) {
    case MAT_WALL_A: case MAT_WALL_B: case MAT_WALL_C: case MAT_WALL_WHITE:
    case MAT_WALL_ABANDONED: case MAT_ROOF_TILE: case MAT_ROOF_TIN: case MAT_ROOF_CONCRETE:
    case MAT_WARUNG_WALL: case MAT_AWNING_RED: case MAT_MASJID_WALL: case MAT_MASJID_DOME:
    case MAT_PASAR_ROOF: case MAT_RUKO_A: case MAT_RUKO_B: case MAT_RUKO_C: case MAT_TANK:
        return 1;
    default:
        return 0;
    }
}

static int isWallBase(Material m)
{
    switch (m) {
    case MAT_WALL_A: case MAT_WALL_B: case MAT_WALL_C: case MAT_WALL_WHITE:
    case MAT_WALL_ABANDONED: case MAT_WARUNG_WALL: case MAT_MASJID_WALL:
    case MAT_RUKO_A: case MAT_RUKO_B: case MAT_RUKO_C:
        return 1;
    default:
        return 0;
    }
}

static void testCornerGraph(void)
{
    static unsigned char seen[CORNERS_TOTAL];
    static int queue[CORNERS_TOTAL];
    int ok = 1, c, k, m, head = 0, tail = 0, nb[4];

    for (c = 0; c < CORNERS_TOTAL; ++c) {
        m = worldCornerNeighbors(c, nb);
        for (k = 0; k < m; ++k) {
            int back[4], mb, j, found = 0;
            mb = worldCornerNeighbors(nb[k], back);
            for (j = 0; j < mb; ++j) if (back[j] == c) found = 1;
            if (!found || nb[k] < 0 || nb[k] >= CORNERS_TOTAL) ok = 0;
        }
    }
    seen[0] = 1;
    queue[tail++] = 0;
    while (head < tail) {
        int cur = queue[head++];
        m = worldCornerNeighbors(cur, nb);
        for (k = 0; k < m; ++k) {
            if (!seen[nb[k]]) { seen[nb[k]] = 1; queue[tail++] = nb[k]; }
        }
    }
    CHECK(ok, "graf trotoar simetris dan semua tetangga valid");
    CHECK(tail == CORNERS_TOTAL, "semua sudut trotoar terhubung dalam satu jaringan");
}

static void testRoles(void)
{
    int cnt[ROLE_COUNT] = {0}, c;
    for (c = 0; c < CORNERS_TOTAL; ++c) cnt[worldCornerRole(c)]++;
    printf("info: POI rumah=%d warung=%d masjid=%d pos=%d ruko=%d pasar=%d\n",
           cnt[ROLE_HOME], cnt[ROLE_WARUNG], cnt[ROLE_MASJID],
           cnt[ROLE_POS], cnt[ROLE_RUKO], cnt[ROLE_PASAR]);
    CHECK(cnt[ROLE_HOME] > 0 && cnt[ROLE_WARUNG] > 0 && cnt[ROLE_MASJID] > 0 &&
          cnt[ROLE_POS] > 0 && cnt[ROLE_RUKO] > 0 && cnt[ROLE_PASAR] > 0,
          "semua peran POI ada di kota");
}

static void testDrawDeterministic(void)
{
    int n1, i, same = 1;
    gCount = 0;
    worldDrawVisible(WORLD_CENTER, WORLD_CENTER);
    n1 = gCount;
    memcpy(gBackup, gCalls, sizeof(Call) * (size_t)n1);
    gCount = 0;
    worldDrawVisible(WORLD_CENTER, WORLD_CENTER);
    if (gCount != n1) same = 0;
    for (i = 0; same && i < n1; ++i) {
        if (memcmp(&gBackup[i], &gCalls[i], sizeof(Call)) != 0) same = 0;
    }
    printf("info: satu frame kota = %d kubus\n", n1);
    CHECK(same && n1 > 0, "gambar kota deterministik dan tidak kosong");
}

static void testBuildingFootprints(void)
{
    int i, checked = 0, bad = 0;
    gCount = 0;
    worldDrawVisible(WORLD_CENTER, WORLD_CENTER);
    for (i = 0; i < gCount; ++i) {
        Call *c = &gCalls[i];
        float cx, cz, ox, oz, x0, x1, z0, z1;
        if (!isBuilding(c->mat)) continue;
        checked++;
        cx = floorf(c->x / WORLD_BLOCK);
        cz = floorf(c->z / WORLD_BLOCK);
        ox = cx * WORLD_BLOCK;
        oz = cz * WORLD_BLOCK;
        x0 = c->x - c->sx * 0.5f; x1 = c->x + c->sx * 0.5f;
        z0 = c->z - c->sz * 0.5f; z1 = c->z + c->sz * 0.5f;
        if (x0 < ox + 5.9f || x1 > ox + 34.1f || z0 < oz + 5.9f || z1 > oz + 34.1f) bad++;
    }
    CHECK(checked > 0 && bad == 0, "bangunan berada di dalam blok (tidak masuk trotoar/jalan)");
}

static void testWallsOnGround(void)
{
    int i, checked = 0, bad = 0;
    gCount = 0;
    worldDrawVisible(WORLD_CENTER, WORLD_CENTER);
    for (i = 0; i < gCount; ++i) {
        Call *c = &gCalls[i];
        if (!isWallBase(c->mat)) continue;
        checked++;
        if (!NEAR(c->y - c->sy * 0.5f, GROUND_TOP, 1e-3f)) bad++;
    }
    CHECK(checked > 0 && bad == 0, "dinding bangunan berdiri tepat di atas tanah");
}

static void testPotholes(void)
{
    int cx, cz, s, hits = 0, samples = 0;
    for (cz = 0; cz <= WORLD_CHUNKS; ++cz) {
        for (cx = 0; cx < WORLD_CHUNKS; ++cx) {
            for (s = 0; s < 80; ++s) {
                float x = (float)cx * WORLD_BLOCK + 0.5f + 0.5f * (float)s;
                float z = (float)cz * WORLD_BLOCK + 1.2f;
                samples++;
                if (worldPotholeSlow(x, z) < 1.0f) hits++;
            }
        }
    }
    printf("info: sampel lajur jalan=%d, kena lubang=%d\n", samples, hits);
    CHECK(hits > 0 && hits * 100 < samples * 30, "jalan rusak ada tapi tidak mendominasi");
}

static int onRoad(float x, float z)
{
    float rx = floorf(x / WORLD_BLOCK + 0.5f) * WORLD_BLOCK;
    float rz = floorf(z / WORLD_BLOCK + 0.5f) * WORLD_BLOCK;
    return fabsf(x - rx) <= ROAD_CAR_MAX + 1e-3f || fabsf(z - rz) <= ROAD_CAR_MAX + 1e-3f;
}

static void testBumpersFollowHeading(void)
{
    Vehicle car = {10.0f, 20.0f, 1.57079632f, 0.0f};
    gCount = 0;
    vehicleDraw(&car);
    CHECK(gCount == 4, "mobil digambar 4 kubus");
    CHECK(NEAR(gCalls[2].x, 12.0f, 1e-3f) && NEAR(gCalls[2].z, 20.0f, 1e-3f),
          "bumper depan di arah hadap (yaw 90 derajat)");
    CHECK(NEAR(gCalls[3].x, 8.0f, 1e-3f) && NEAR(gCalls[3].z, 20.0f, 1e-3f),
          "bumper belakang di belakang mobil (yaw 90 derajat)");
}

static void testCarOnRoadAndCaps(void)
{
    Vehicle car;
    InputState in = {0, 0, 0, 0.0f};
    int f, bad = 0;
    vehicleReset(&car);
    CHECK(onRoad(car.x, car.z), "mobil mulai di persimpangan pusat");
    for (f = 0; f < 20000; ++f) {
        in.accel = (f % 600) < 400;
        in.brake = (f % 900) > 800;
        in.steer = (f % 300 < 150) ? 0.8f : -0.6f;
        vehicleUpdate(&car, &in);
        if (!isfinite(car.x) || !isfinite(car.z) || !isfinite(car.yaw) || !isfinite(car.speed)) bad++;
        if (!onRoad(car.x, car.z)) bad++;
        if (car.speed > 0.42f + 1e-6f || car.speed < -0.15f - 1e-6f) bad++;
        if (car.yaw > 3.1416f || car.yaw < -3.1416f) bad++;
    }
    CHECK(bad == 0, "20000 frame berkendara: selalu di jalan, finit, kecepatan dibatasi");
}

/* Titik dekat ruas trotoar atau penyeberangan? */
static int onGraph(float px, float pz)
{
    static int checked = 0;
    int c, k, m, nb[4];
    (void)checked;
    for (c = 0; c < CORNERS_TOTAL; ++c) {
        float ax, az;
        worldCornerPos(c, &ax, &az);
        m = worldCornerNeighbors(c, nb);
        for (k = 0; k < m; ++k) {
            float bx, bz, dx, dz, len2, t, qx, qz;
            worldCornerPos(nb[k], &bx, &bz);
            dx = bx - ax; dz = bz - az;
            len2 = dx * dx + dz * dz;
            t = ((px - ax) * dx + (pz - az) * dz) / len2;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;
            qx = ax + dx * t - px;
            qz = az + dz * t - pz;
            if (qx * qx + qz * qz <= 1e-4f) return 1;
        }
    }
    return 0;
}

static void testNpcDay(void)
{
    NpcView v, prev[NPC_COUNT];
    int f, i, badPos = 0, badStep = 0, badEdge = 0, samples = 0;
    int actFrames[6] = {0, 0, 0, 0, 0, 0};
    int everDoing[NPC_COUNT];
    int hiddenAt23 = -1, doingCount = 0;
    int frames = 1440 * NPC_FRAMES_PER_MIN;

    npcInit();
    for (i = 0; i < NPC_COUNT; ++i) {
        npcGetView(i, &prev[i]);
        everDoing[i] = 0;
    }
    for (f = 0; f < frames; ++f) {
        npcUpdate();
        if (hiddenAt23 < 0 && npcGetMinute() == 23 * 60) {
            int h = 0;
            for (i = 0; i < NPC_COUNT; ++i) { npcGetView(i, &v); h += v.hidden; }
            hiddenAt23 = h;
        }
        for (i = 0; i < NPC_COUNT; ++i) {
            npcGetView(i, &v);
            /* Trotoar tepi kota berada 5.5 unit di luar garis jalan terluar. */
            if (!isfinite(v.x) || !isfinite(v.z) || v.x < -5.5f || v.x > WORLD_SIZE + 5.5f ||
                v.z < -5.5f || v.z > WORLD_SIZE + 5.5f) badPos++;
            if (!prev[i].hidden && !v.hidden && !(prev[i].x == v.x && prev[i].z == v.z) &&
                fabsf(v.x - prev[i].x) + fabsf(v.z - prev[i].z) > NPC_SPEED + 1e-4f) badStep++;
            if (!v.hidden) {
                actFrames[v.act]++;
                if (v.act != ACT_WALK && v.act != ACT_HOME) everDoing[i] = 1;
            }
            prev[i] = v;
        }
        if (f % 300 == 0) {
            for (i = 0; i < NPC_COUNT; ++i) {
                npcGetView(i, &v);
                if (v.hidden) continue;
                samples++;
                if (!onGraph(v.x, v.z)) badEdge++;
            }
        }
    }
    for (i = 0; i < NPC_COUNT; ++i) doingCount += everDoing[i];
    printf("info: aktivitas (frame): jalan=%d kerja=%d makan=%d ngobrol=%d ibadah=%d di rumah=%d\n",
           actFrames[0], actFrames[1], actFrames[2], actFrames[3], actFrames[4], actFrames[5]);
    printf("info: NPC yang pernah kerja/makan/ngobrol/ibadah = %d dari %d; tersembunyi jam 23:00 = %d\n",
           doingCount, NPC_COUNT, hiddenAt23);
    CHECK(badPos == 0, "semua NPC tetap di dalam area kota (termasuk trotoar tepi)");
    CHECK(badStep == 0, "NPC bergerak mulus, tidak melompat");
    CHECK(badEdge == 0 && samples > 0, "NPC selalu berada di trotoar atau penyeberangan");
    CHECK(doingCount >= NPC_COUNT * 8 / 10, "minimal 80% NPC punya kegiatan sehari-hari");
    CHECK(hiddenAt23 > 0, "ada NPC yang sudah pulang ke rumah pada jam 23:00");
    CHECK(actFrames[4] > 0, "ada NPC yang beribadah di masjid");

    gCount = 0;
    npcDraw(WORLD_CENTER, WORLD_CENTER);
    printf("info: NPC tergambar di sekitar kamera = %d kubus\n", gCount);
    CHECK(gCount > 0, "NPC tergambar di kamera");
}

int main(void)
{
    testCornerGraph();
    testRoles();
    testDrawDeterministic();
    testBuildingFootprints();
    testWallsOnGround();
    testPotholes();
    testBumpersFollowHeading();
    testCarOnRoadAndCaps();
    testNpcDay();

    if (gFail) {
        printf("RESULT: %d FAILED\n", gFail);
        return 1;
    }
    printf("RESULT: all host logic tests passed\n");
    return 0;
}
