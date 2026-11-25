#ifndef RENDER_H
#define RENDER_H

/* Material palette. Each entry has a pre-baked vertex color table for a unit cube. */
typedef enum {
    MAT_GROUND_EARTH = 0,
    MAT_GROUND_CONCRETE,
    MAT_GRASS,
    MAT_ROAD,
    MAT_ROAD_PATCH,
    MAT_POTHOLE,
    MAT_LANE,
    MAT_ZEBRA,
    MAT_SIDEWALK,
    MAT_WALL_A,
    MAT_WALL_B,
    MAT_WALL_C,
    MAT_WALL_WHITE,
    MAT_WALL_ABANDONED,
    MAT_ROOF_TILE,
    MAT_ROOF_TIN,
    MAT_ROOF_CONCRETE,
    MAT_WARUNG_WALL,
    MAT_AWNING_RED,
    MAT_TABLE,
    MAT_CHAIR,
    MAT_FENCE,
    MAT_TREE_TRUNK,
    MAT_TREE_CROWN,
    MAT_POLE,
    MAT_MASJID_WALL,
    MAT_MASJID_DOME,
    MAT_PASAR_ROOF,
    MAT_RUKO_A,
    MAT_RUKO_B,
    MAT_RUKO_C,
    MAT_TANK,
    MAT_MOTOR,
    MAT_CAR_BODY,
    MAT_CAR_CABIN,
    MAT_BUMPER,
    MAT_SKIN_1,
    MAT_SKIN_2,
    MAT_SKIN_3,
    MAT_SHIRT_1,
    MAT_SHIRT_2,
    MAT_SHIRT_3,
    MAT_SHIRT_4,
    MAT_SHIRT_5,
    MAT_SHIRT_6,
    MAT_PANTS,
    MAT_HIJAB,
    MAT_HAIR,
    MAT_COUNT
} Material;

void renderInit(void);
void renderBeginFrame(void);
void renderEndFrame(void);

/* Draws a unit cube scaled by (sx,sy,sz), rotated around Y by yaw, moved to (x,y,z). */
void renderCube(Material mat, float x, float y, float z,
                float sx, float sy, float sz, float yaw);

#endif
