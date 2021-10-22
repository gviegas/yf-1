/*
 * YF
 * data-gltf.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_GLTF_H
#define YF_DATA_GLTF_H

#include <stdio.h>

#include "yf-collection.h"
#include "yf-scene.h"
#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-skin.h"
#include "yf-material.h"
#include "yf-texture.h"
#include "yf-animation.h"

/* Data content types. */
#define YF_DATAC_COLL 0
#define YF_DATAC_SCN  1
#define YF_DATAC_NODE 2
#define YF_DATAC_MESH 3
#define YF_DATAC_SKIN 4
#define YF_DATAC_MATL 5
#define YF_DATAC_TEX  6
#define YF_DATAC_ANIM 7

/* Data contents. */
typedef struct {
    int datac;

    /* contents will be stored in this collection */
    YF_collection coll;

    /* for 'datac' values other than 'COLL', this union will contain
       the created object (already inserted into 'coll') */
    union {
        YF_scene scn;
        YF_node node;
        YF_mesh mesh;
        YF_skin skin;
        YF_material matl;
        YF_texture tex;
        YF_animation anim;
    };
} YF_datac;

/* Loads contents from a glTF file. */
int yf_loadgltf(const char *pathname, size_t index, YF_datac *datac);

/* Loads contents from a file containing glTF data.
   Decoding begins at the file's current position, and all data must be
   embedded in the file, since its path is unknown. */
int yf_loadgltf2(FILE *file, size_t index, int datac, YF_datac *dst);

#endif /* YF_DATA_GLTF_H */
