/*
 * YF
 * data-gltf.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_GLTF_H
#define YF_DATA_GLTF_H

#include <stdio.h>

#include "yf-collec.h"
#include "yf-scene.h"
#include "yf-node.h"
#include "yf-mesh.h"
#include "yf-skin.h"
#include "yf-material.h"
#include "yf-texture.h"
#include "yf-kfanim.h"

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
typedef struct yf_datac {
    int datac;

    /* contents will be stored in this collection */
    yf_collec_t *coll;

    /* for 'datac' values other than 'COLL', this union will contain
       the created object (already inserted into 'coll') */
    union {
        yf_scene_t *scn;
        yf_node_t *node;
        yf_mesh_t *mesh;
        yf_skin_t *skin;
        yf_material_t *matl;
        yf_texture_t *tex;
        yf_kfanim_t *anim;
    };
} yf_datac_t;

/* Loads contents from a glTF file. */
int yf_loadgltf(const char *pathname, size_t index, yf_datac_t *datac);

/* Loads contents from a file containing glTF data.
   Decoding begins at the file's current position, and all data must be
   embedded in the file, since its path is unknown. */
int yf_loadgltf2(FILE *file, size_t index, yf_datac_t *datac);

#endif /* YF_DATA_GLTF_H */
