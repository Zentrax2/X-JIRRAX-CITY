#ifndef WORLD_H
#define WORLD_H

#define WORLD_CHUNKS       8                          /* kota 8 x 8 blok */
#define WORLD_NODES        (WORLD_CHUNKS + 1)         /* 9 x 9 persimpangan */
#define WORLD_BLOCK        40.0f                      /* jarak pusat jalan ke pusat jalan */
#define WORLD_SIZE         (WORLD_CHUNKS * WORLD_BLOCK)
#define WORLD_CENTER       (WORLD_SIZE * 0.5f)
#define WORLD_VIEW_RADIUS  1                          /* blok digambar di sekitar kamera */
#define CORNERS_TOTAL      (WORLD_NODES * WORLD_NODES * 4)
#define ROAD_CAR_MAX       1.8f                       /* pusat mobil maks. dari pusat jalan */

#define GROUND_TOP         (-0.525f)
#define ROAD_TOP           (-0.440f)
#define SIDEWALK_TOP       (-0.360f)

typedef enum {
    ROLE_NONE = 0,
    ROLE_HOME,
    ROLE_WARUNG,
    ROLE_MASJID,
    ROLE_PASAR,
    ROLE_RUKO,
    ROLE_POS,
    ROLE_COUNT
} PoiRole;

unsigned int worldHash(unsigned int a, unsigned int b, unsigned int c);

/* Graf trotoar: setiap sudut persimpangan punya 4 sudut (quadrant). Mengisi out[] dan mengembalikan jumlahnya. */
int  worldCornerNeighbors(int corner, int out[4]);
void worldCornerPos(int corner, float *x, float *z);
PoiRole worldCornerRole(int corner);

void worldDrawVisible(float camX, float camZ);

/* Pengali kecepatan: 1.0 di jalan normal, lebih kecil di lubang jalan. */
float worldPotholeSlow(float x, float z);

/* Menjaga mobil tetap di badan jalan. */
void worldClampToRoad(float *x, float *z);

#endif
