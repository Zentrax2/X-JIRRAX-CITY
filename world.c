#include <math.h>
#include "render.h"
#include "world.h"

#define CORNER_OFS   4.5f    /* titik berjalan kaki: 4.5 dari pusat jalan */
#define ROAD_HALF_W  3.0f
#define SW_LEN       (WORLD_BLOCK - 2.0f * ROAD_HALF_W)   /* 34: trotoar per sisi */
#define SW_OFS       4.25f   /* pusat trotoar dari pusat jalan (3.0 .. 5.5) */
#define POT_ALONG    1.1f    /* setengah panjang lubang */
#define POT_ACROSS   0.8f    /* setengah lebar lubang */
#define POT_SLOW     0.55f

typedef enum {
    BLK_KAMPUNG = 0,
    BLK_RUKO,
    BLK_PASAR,
    BLK_PERUMAHAN,
    BLK_MASJID,
    BLK_LAPANGAN
} BlockType;

/* Peran tiap sudut blok, indeks: 0 = pojok barat-selatan, 1 = timur-selatan, 2 = barat-utara, 3 = timur-utara. */
static const int kRoleTable[6][4] = {
    {ROLE_WARUNG, ROLE_HOME,  ROLE_HOME,  ROLE_POS},   /* kampung */
    {ROLE_RUKO,   ROLE_RUKO,  ROLE_RUKO,  ROLE_RUKO},  /* ruko */
    {ROLE_PASAR,  ROLE_PASAR, ROLE_PASAR, ROLE_PASAR}, /* pasar */
    {ROLE_HOME,   ROLE_HOME,  ROLE_HOME,  ROLE_HOME},  /* perumahan */
    {ROLE_MASJID, ROLE_NONE,  ROLE_NONE,  ROLE_POS},   /* masjid */
    {ROLE_NONE,   ROLE_NONE,  ROLE_NONE,  ROLE_POS}    /* lapangan */
};

static const Material kWallMats[4] = {MAT_WALL_A, MAT_WALL_B, MAT_WALL_C, MAT_WALL_WHITE};
static const Material kRoofMats[3] = {MAT_ROOF_TILE, MAT_ROOF_TIN, MAT_ROOF_CONCRETE};
static const Material kRukoMats[3] = {MAT_RUKO_A, MAT_RUKO_B, MAT_RUKO_C};

unsigned int worldHash(unsigned int a, unsigned int b, unsigned int c)
{
    unsigned int h = 2166136261u;
    h = (h ^ a) * 16777619u;
    h = (h ^ b) * 16777619u;
    h = (h ^ c) * 16777619u;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    return h;
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static void box(Material m, float x, float y, float z, float sx, float sy, float sz)
{
    renderCube(m, x, y, z, sx, sy, sz, 0.0f);
}

/* ---- Graf trotoar ---- */

static int encodeCorner(int nx, int nz, int qx, int qz)
{
    return (((nx * WORLD_NODES) + nz) << 2) | (qx > 0 ? 1 : 0) | (qz > 0 ? 2 : 0);
}

static void decodeCorner(int c, int *nx, int *nz, int *qx, int *qz)
{
    int node = c >> 2;
    int q = c & 3;
    *nx = node / WORLD_NODES;
    *nz = node % WORLD_NODES;
    *qx = (q & 1) ? 1 : -1;
    *qz = (q & 2) ? 1 : -1;
}

int worldCornerNeighbors(int c, int out[4])
{
    int nx, nz, qx, qz, n = 0;
    decodeCorner(c, &nx, &nz, &qx, &qz);
    /* Trotoar sepanjang jalan X (ke persimpangan di sebelah) */
    if (nx + qx >= 0 && nx + qx < WORLD_NODES) {
        out[n++] = encodeCorner(nx + qx, nz, -qx, qz);
    }
    /* Trotoar sepanjang jalan Z */
    if (nz + qz >= 0 && nz + qz < WORLD_NODES) {
        out[n++] = encodeCorner(nx, nz + qz, qx, -qz);
    }
    /* Menyeberang jalan Z (zebra cross) */
    out[n++] = encodeCorner(nx, nz, -qx, qz);
    /* Menyeberang jalan X (zebra cross) */
    out[n++] = encodeCorner(nx, nz, qx, -qz);
    return n;
}

void worldCornerPos(int c, float *x, float *z)
{
    int nx, nz, qx, qz;
    decodeCorner(c, &nx, &nz, &qx, &qz);
    *x = (float)nx * WORLD_BLOCK + (float)qx * CORNER_OFS;
    *z = (float)nz * WORLD_BLOCK + (float)qz * CORNER_OFS;
}

/* ---- Tipe blok dan peran POI ---- */

static int blockType(int bx, int bz)
{
    unsigned int r;
    if (bx == 3 && bz == 3) return BLK_MASJID;   /* masjid utama dekat pusat kota */
    r = worldHash((unsigned int)bx, (unsigned int)bz, 7u) % 100u;
    if (r < 40u) return BLK_KAMPUNG;
    if (r < 55u) return BLK_RUKO;
    if (r < 65u) return BLK_PASAR;
    if (r < 82u) return BLK_PERUMAHAN;
    if (r < 92u) return BLK_MASJID;
    return BLK_LAPANGAN;
}

PoiRole worldCornerRole(int c)
{
    int nx, nz, qx, qz, bx, bz, idx;
    decodeCorner(c, &nx, &nz, &qx, &qz);
    bx = (qx > 0) ? nx : nx - 1;
    bz = (qz > 0) ? nz : nz - 1;
    if (bx < 0 || bz < 0 || bx >= WORLD_CHUNKS || bz >= WORLD_CHUNKS) return ROLE_NONE;
    idx = (qx > 0 ? 0 : 1) + (qz > 0 ? 0 : 2);
    return (PoiRole)kRoleTable[blockType(bx, bz)][idx];
}

/* ---- Lubang jalan (deterministik, dipakai untuk gambar dan fisika) ---- */

static unsigned int potCount(unsigned int kind, int cx, int cz)
{
    return worldHash(kind + 17u, (unsigned int)cx, (unsigned int)cz) % 3u;
}

static void potCentre(unsigned int kind, int cx, int cz, unsigned int k, float *ux, float *uz)
{
    unsigned int h = worldHash(kind * 16u + k, (unsigned int)cx, (unsigned int)cz * 8u + 5u);
    float along = 8.0f + (float)(h % 24u);
    float across = ((h >> 8) & 1u) ? 1.2f : -1.2f;
    if (kind == 0u) {
        *ux = (float)cx * WORLD_BLOCK + along;
        *uz = (float)cz * WORLD_BLOCK + across;
    } else {
        *ux = (float)cx * WORLD_BLOCK + across;
        *uz = (float)cz * WORLD_BLOCK + along;
    }
}

static int inPothole(float x, float z, float ux, float uz, int alongX)
{
    float ha = alongX ? POT_ALONG : POT_ACROSS;
    float hb = alongX ? POT_ACROSS : POT_ALONG;
    return fabsf(x - ux) <= ha && fabsf(z - uz) <= hb;
}

float worldPotholeSlow(float x, float z)
{
    int rz = clampi((int)floorf(z / WORLD_BLOCK + 0.5f), 0, WORLD_NODES - 1);
    int rx = clampi((int)floorf(x / WORLD_BLOCK + 0.5f), 0, WORLD_NODES - 1);
    int cx = clampi((int)floorf(x / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    int cz = clampi((int)floorf(z / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    unsigned int k, n;
    float ux, uz;

    if (fabsf(z - (float)rz * WORLD_BLOCK) <= ROAD_HALF_W) {
        n = potCount(0u, cx, rz);
        for (k = 0u; k < n; ++k) {
            potCentre(0u, cx, rz, k, &ux, &uz);
            if (inPothole(x, z, ux, uz, 1)) return POT_SLOW;
        }
    }
    if (fabsf(x - (float)rx * WORLD_BLOCK) <= ROAD_HALF_W) {
        n = potCount(1u, rx, cz);
        for (k = 0u; k < n; ++k) {
            potCentre(1u, rx, cz, k, &ux, &uz);
            if (inPothole(x, z, ux, uz, 0)) return POT_SLOW;
        }
    }
    return 1.0f;
}

void worldClampToRoad(float *x, float *z)
{
    float rx, rz, dx, dz;
    *x = (*x < ROAD_CAR_MAX) ? ROAD_CAR_MAX : *x;
    *x = (*x > WORLD_SIZE - ROAD_CAR_MAX) ? WORLD_SIZE - ROAD_CAR_MAX : *x;
    *z = (*z < ROAD_CAR_MAX) ? ROAD_CAR_MAX : *z;
    *z = (*z > WORLD_SIZE - ROAD_CAR_MAX) ? WORLD_SIZE - ROAD_CAR_MAX : *z;

    rx = floorf(*x / WORLD_BLOCK + 0.5f) * WORLD_BLOCK;
    rz = floorf(*z / WORLD_BLOCK + 0.5f) * WORLD_BLOCK;
    dx = fabsf(*x - rx);
    dz = fabsf(*z - rz);
    if (dx <= ROAD_CAR_MAX || dz <= ROAD_CAR_MAX) return;

    /* Di dalam blok: dorong ke tepi jalan terdekat. */
    if (dx < dz) {
        *x = rx + ((*x < rx) ? -ROAD_CAR_MAX : ROAD_CAR_MAX);
    } else {
        *z = rz + ((*z < rz) ? -ROAD_CAR_MAX : ROAD_CAR_MAX);
    }
}

/* ---- Jalan, trotoar, lajur, zebra ---- */

static void drawEdgeX(int nx, int nz)
{
    float x0 = (float)nx * WORLD_BLOCK;
    float z0 = (float)nz * WORLD_BLOCK;
    unsigned int n = potCount(0u, nx, nz);
    unsigned int k;
    float ux, uz;

    box(MAT_ROAD, x0 + WORLD_BLOCK * 0.5f, ROAD_TOP - 0.04f, z0, WORLD_BLOCK, 0.08f, 2.0f * ROAD_HALF_W);
    box(MAT_SIDEWALK, x0 + WORLD_BLOCK * 0.5f, SIDEWALK_TOP - 0.08f, z0 + SW_OFS, SW_LEN, 0.16f, 2.5f);
    box(MAT_SIDEWALK, x0 + WORLD_BLOCK * 0.5f, SIDEWALK_TOP - 0.08f, z0 - SW_OFS, SW_LEN, 0.16f, 2.5f);
    for (k = 0u; k < 4u; ++k) {
        box(MAT_LANE, x0 + 4.0f + 8.0f * (float)k, ROAD_TOP + 0.01f, z0, 3.5f, 0.025f, 0.22f);
    }
    for (k = 0u; k < n; ++k) {
        potCentre(0u, nx, nz, k, &ux, &uz);
        if ((k + (unsigned int)nx + (unsigned int)nz) & 1u) {
            box(MAT_ROAD_PATCH, ux, ROAD_TOP + 0.02f, uz, 3.2f, 0.03f, 2.4f);
        }
        box(MAT_POTHOLE, ux, ROAD_TOP + 0.03f, uz, 2.2f, 0.03f, 1.6f);
    }
}

static void drawEdgeZ(int nx, int nz)
{
    float x0 = (float)nx * WORLD_BLOCK;
    float z0 = (float)nz * WORLD_BLOCK;
    unsigned int n = potCount(1u, nx, nz);
    unsigned int k;
    float ux, uz;

    box(MAT_ROAD, x0, ROAD_TOP - 0.03f, z0 + WORLD_BLOCK * 0.5f, 2.0f * ROAD_HALF_W, 0.08f, WORLD_BLOCK);
    box(MAT_SIDEWALK, x0 + SW_OFS, SIDEWALK_TOP - 0.08f, z0 + WORLD_BLOCK * 0.5f, 2.5f, 0.16f, SW_LEN);
    box(MAT_SIDEWALK, x0 - SW_OFS, SIDEWALK_TOP - 0.08f, z0 + WORLD_BLOCK * 0.5f, 2.5f, 0.16f, SW_LEN);
    for (k = 0u; k < 4u; ++k) {
        box(MAT_LANE, x0, ROAD_TOP + 0.02f, z0 + 4.0f + 8.0f * (float)k, 0.22f, 0.025f, 3.5f);
    }
    for (k = 0u; k < n; ++k) {
        potCentre(1u, nx, nz, k, &ux, &uz);
        if ((k + (unsigned int)nx + (unsigned int)nz) & 1u) {
            box(MAT_ROAD_PATCH, ux, ROAD_TOP + 0.02f, uz, 2.4f, 0.03f, 3.2f);
        }
        box(MAT_POTHOLE, ux, ROAD_TOP + 0.03f, uz, 1.6f, 0.03f, 2.2f);
    }
}

/* Zebra cross di satu persimpangan: 4 penyeberangan, masing-masing 2 garis. */
static void drawZebra(int nx, int nz)
{
    float x0 = (float)nx * WORLD_BLOCK;
    float z0 = (float)nz * WORLD_BLOCK;
    int s, k;
    for (s = -1; s <= 1; s += 2) {
        float zc = z0 + (float)s * CORNER_OFS;
        for (k = -1; k <= 1; k += 2) {
            box(MAT_ZEBRA, x0, ROAD_TOP + 0.04f, zc + (float)k * 0.5f, 6.0f, 0.02f, 0.6f);
        }
    }
    for (s = -1; s <= 1; s += 2) {
        float xc = x0 + (float)s * CORNER_OFS;
        for (k = -1; k <= 1; k += 2) {
            box(MAT_ZEBRA, xc + (float)k * 0.5f, ROAD_TOP + 0.04f, z0, 0.6f, 0.02f, 6.0f);
        }
    }
}

/* ---- Isi blok ---- */

static void drawPoles(float ox, float oz)
{
    /* Tiang listrik di dua pojok diagonal trotoar, lengkap dengan palang. */
    box(MAT_POLE, ox + 3.5f, SIDEWALK_TOP + 2.25f, oz + 3.5f, 0.3f, 4.5f, 0.3f);
    box(MAT_POLE, ox + 4.8f, SIDEWALK_TOP + 4.3f, oz + 3.5f, 2.6f, 0.12f, 0.12f);
    box(MAT_POLE, ox + 36.5f, SIDEWALK_TOP + 2.25f, oz + 36.5f, 0.3f, 4.5f, 0.3f);
    box(MAT_POLE, ox + 35.2f, SIDEWALK_TOP + 4.3f, oz + 36.5f, 2.6f, 0.12f, 0.12f);
}

static void drawWarung(float cx, float cz)
{
    box(MAT_WARUNG_WALL, cx, GROUND_TOP + 1.4f, cz, 5.0f, 2.8f, 5.0f);
    box(MAT_AWNING_RED, cx, GROUND_TOP + 2.9f, cz, 5.8f, 0.25f, 5.8f);
    box(MAT_TABLE, cx, GROUND_TOP + 0.4f, cz + 3.2f, 2.4f, 0.8f, 1.0f);
    box(MAT_CHAIR, cx - 1.6f, GROUND_TOP + 0.3f, cz + 3.2f, 0.6f, 0.6f, 0.6f);
    box(MAT_CHAIR, cx + 1.6f, GROUND_TOP + 0.3f, cz + 3.2f, 0.6f, 0.6f, 0.6f);
}

static void drawPosRonda(float cx, float cz)
{
    box(MAT_POLE, cx - 1.5f, GROUND_TOP + 1.3f, cz - 1.5f, 0.25f, 2.6f, 0.25f);
    box(MAT_POLE, cx + 1.5f, GROUND_TOP + 1.3f, cz - 1.5f, 0.25f, 2.6f, 0.25f);
    box(MAT_POLE, cx - 1.5f, GROUND_TOP + 1.3f, cz + 1.5f, 0.25f, 2.6f, 0.25f);
    box(MAT_POLE, cx + 1.5f, GROUND_TOP + 1.3f, cz + 1.5f, 0.25f, 2.6f, 0.25f);
    box(MAT_ROOF_TILE, cx, GROUND_TOP + 2.75f, cz, 3.6f, 0.3f, 3.6f);
}

static void drawTree(float x, float z)
{
    box(MAT_TREE_TRUNK, x, GROUND_TOP + 1.75f, z, 0.6f, 3.5f, 0.6f);
    box(MAT_TREE_CROWN, x, GROUND_TOP + 4.2f, z, 3.2f, 2.8f, 3.2f);
}

/* Kampung: rumah rapat dengan gang sempit, warung di sel (0,0), pos ronda di sel (3,3). */
static void drawKampung(float ox, float oz, int bx, int bz)
{
    int i, j;
    drawWarung(ox + 9.5f, oz + 9.5f);
    drawPosRonda(ox + 30.5f, oz + 30.5f);
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 4; ++j) {
            unsigned int h;
            float w, hgt, cxh, czh;
            Material wall, roof;
            if ((i == 0 && j == 0) || (i == 3 && j == 3)) continue;
            h = worldHash((unsigned int)bx, (unsigned int)bz, (unsigned int)(i * 4 + j + 32));
            if (h % 100u < 22u) continue;   /* lahan kosong: halaman atau jemuran */
            w = 4.0f + (float)((h >> 3) % 3u) * 0.6f;
            hgt = 3.0f + (float)((h >> 6) % 5u);
            cxh = ox + 6.0f + 7.0f * (float)i + 3.5f + ((float)((h >> 9) % 3u) - 1.0f) * 0.4f;
            czh = oz + 6.0f + 7.0f * (float)j + 3.5f + ((float)((h >> 11) % 3u) - 1.0f) * 0.4f;
            wall = ((h >> 13) % 10u == 0u) ? MAT_WALL_ABANDONED : kWallMats[(h >> 17) % 4u];
            roof = kRoofMats[(h >> 21) % 3u];
            box(wall, cxh, GROUND_TOP + hgt * 0.5f, czh, w, hgt, w);
            box(roof, cxh, GROUND_TOP + hgt + 0.2f, czh, w + 0.5f, 0.4f, w + 0.5f);
        }
    }
}

/* Ruko: dua deret ruko 3-4 lantai, motor parkir di tengah. */
static void drawRuko(float ox, float oz, int bx, int bz)
{
    int i;
    for (i = 0; i < 4; ++i) {
        unsigned int h1 = worldHash((unsigned int)bx, (unsigned int)bz, 200u + (unsigned int)i);
        unsigned int h2 = worldHash((unsigned int)bx, (unsigned int)bz, 300u + (unsigned int)i);
        float x = ox + 9.25f + 6.6f * (float)i;
        float hg1 = 6.0f + (float)(h1 % 3u) * 3.0f;
        float hg2 = 6.0f + (float)(h2 % 3u) * 3.0f;
        box(kRukoMats[(h1 >> 4) % 3u], x, GROUND_TOP + hg1 * 0.5f, oz + 9.0f, 6.2f, hg1, 6.0f);
        box(kRukoMats[(h2 >> 4) % 3u], x, GROUND_TOP + hg2 * 0.5f, oz + 31.0f, 6.2f, hg2, 6.0f);
        if (h1 & 8u) {
            box(MAT_TANK, x + 1.2f, GROUND_TOP + hg1 + 0.75f, oz + 8.2f, 1.5f, 1.5f, 1.5f);
        }
    }
    for (i = 0; i < 4; ++i) {
        box(MAT_MOTOR, ox + 12.0f + 5.0f * (float)i, GROUND_TOP + 0.5f, oz + 20.0f, 1.6f, 1.0f, 0.5f);
    }
}

/* Pasar: atap besar di atas tiang, deretan meja dagangan. */
static void drawPasar(float ox, float oz)
{
    int i, j;
    box(MAT_PASAR_ROOF, ox + 20.0f, GROUND_TOP + 5.2f, oz + 20.0f, 24.0f, 0.6f, 24.0f);
    box(MAT_POLE, ox + 8.0f, GROUND_TOP + 2.6f, oz + 8.0f, 0.5f, 5.2f, 0.5f);
    box(MAT_POLE, ox + 32.0f, GROUND_TOP + 2.6f, oz + 8.0f, 0.5f, 5.2f, 0.5f);
    box(MAT_POLE, ox + 8.0f, GROUND_TOP + 2.6f, oz + 32.0f, 0.5f, 5.2f, 0.5f);
    box(MAT_POLE, ox + 32.0f, GROUND_TOP + 2.6f, oz + 32.0f, 0.5f, 5.2f, 0.5f);
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 2; ++j) {
            box(MAT_TABLE, ox + 11.0f + 6.0f * (float)i, GROUND_TOP + 0.4f,
                oz + 14.0f + 12.0f * (float)j, 3.5f, 0.8f, 1.2f);
        }
    }
}

/* Perumahan: 3 x 3 rumah seragam dengan pagar depan. */
static void drawPerumahan(float ox, float oz)
{
    int i, j;
    for (i = 0; i < 3; ++i) {
        for (j = 0; j < 3; ++j) {
            float cx = ox + 6.0f + 9.33f * ((float)i + 0.5f);
            float cz = oz + 6.0f + 9.33f * ((float)j + 0.5f);
            box(MAT_WALL_WHITE, cx, GROUND_TOP + 2.5f, cz, 6.2f, 5.0f, 6.2f);
            box(MAT_ROOF_CONCRETE, cx, GROUND_TOP + 5.2f, cz, 6.6f, 0.4f, 6.6f);
            box(MAT_FENCE, cx, GROUND_TOP + 0.5f, cz - 3.6f, 6.2f, 1.0f, 0.25f);
        }
    }
}

/* Masjid: badan utama, kubah, dua menara. */
static void drawMasjid(float ox, float oz)
{
    box(MAT_MASJID_WALL, ox + 20.0f, GROUND_TOP + 4.0f, oz + 20.0f, 14.0f, 8.0f, 14.0f);
    box(MAT_MASJID_DOME, ox + 20.0f, GROUND_TOP + 9.5f, oz + 20.0f, 7.0f, 3.0f, 7.0f);
    box(MAT_MASJID_WALL, ox + 8.0f, GROUND_TOP + 8.0f, oz + 32.0f, 2.5f, 16.0f, 2.5f);
    box(MAT_MASJID_DOME, ox + 8.0f, GROUND_TOP + 16.5f, oz + 32.0f, 2.8f, 1.0f, 2.8f);
    box(MAT_MASJID_WALL, ox + 32.0f, GROUND_TOP + 8.0f, oz + 8.0f, 2.5f, 16.0f, 2.5f);
    box(MAT_MASJID_DOME, ox + 32.0f, GROUND_TOP + 16.5f, oz + 8.0f, 2.8f, 1.0f, 2.8f);
    drawPosRonda(ox + 30.5f, oz + 30.5f);
}

/* Lapangan: rumput dan pohon, tanpa bangunan. */
static void drawLapangan(float ox, float oz)
{
    drawTree(ox + 10.0f, oz + 10.0f);
    drawTree(ox + 26.0f, oz + 10.0f);
    drawTree(ox + 10.0f, oz + 26.0f);
    drawPosRonda(ox + 30.5f, oz + 30.5f);
}

static void drawBlock(int bx, int bz)
{
    int type = blockType(bx, bz);
    float ox = (float)bx * WORLD_BLOCK;
    float oz = (float)bz * WORLD_BLOCK;
    Material ground;

    switch (type) {
    case BLK_RUKO:
    case BLK_PASAR:
        ground = MAT_GROUND_CONCRETE;
        break;
    case BLK_MASJID:
    case BLK_LAPANGAN:
        ground = MAT_GRASS;
        break;
    default:
        ground = MAT_GROUND_EARTH;
        break;
    }
    box(ground, ox + 20.0f, GROUND_TOP - 0.125f, oz + 20.0f, WORLD_BLOCK, 0.25f, WORLD_BLOCK);
    drawPoles(ox, oz);

    switch (type) {
    case BLK_KAMPUNG:    drawKampung(ox, oz, bx, bz); break;
    case BLK_RUKO:       drawRuko(ox, oz, bx, bz);    break;
    case BLK_PASAR:      drawPasar(ox, oz);           break;
    case BLK_PERUMAHAN:  drawPerumahan(ox, oz);       break;
    case BLK_MASJID:     drawMasjid(ox, oz);          break;
    default:             drawLapangan(ox, oz);        break;
    }
}

/* Menggambar area kamera. Blok digambar per kotak, jalan per simpul agar tiap ruas hanya sekali. */
void worldDrawVisible(float camX, float camZ)
{
    int ccx = clampi((int)floorf(camX / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    int ccz = clampi((int)floorf(camZ / WORLD_BLOCK), 0, WORLD_CHUNKS - 1);
    int x0 = clampi(ccx - WORLD_VIEW_RADIUS, 0, WORLD_CHUNKS - 1);
    int x1 = clampi(ccx + WORLD_VIEW_RADIUS, 0, WORLD_CHUNKS - 1);
    int z0 = clampi(ccz - WORLD_VIEW_RADIUS, 0, WORLD_CHUNKS - 1);
    int z1 = clampi(ccz + WORLD_VIEW_RADIUS, 0, WORLD_CHUNKS - 1);
    int cx, cz, nx, nz;

    for (cz = z0; cz <= z1; ++cz) {
        for (cx = x0; cx <= x1; ++cx) {
            drawBlock(cx, cz);
        }
    }
    for (nz = z0; nz <= z1 + 1; ++nz) {
        for (nx = x0; nx <= x1 + 1; ++nx) {
            drawZebra(nx, nz);
            if (nx < WORLD_CHUNKS) drawEdgeX(nx, nz);
            if (nz < WORLD_CHUNKS) drawEdgeZ(nx, nz);
        }
    }
}
