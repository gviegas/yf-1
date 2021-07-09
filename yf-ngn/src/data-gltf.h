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
#include "yf-mesh.h"
#include "yf-texture.h"
#include "yf-skin.h"
#include "yf-material.h"

/* Data content types. */
/* TODO: scene, node, subgraph, ... */
#define YF_DATAC_COLL 0
#define YF_DATAC_MESH 1
#define YF_DATAC_TEX  2
#define YF_DATAC_SKIN 3
#define YF_DATAC_MATL 4

/* Type defining data contents that can be loaded. */
typedef union {
    YF_collection coll;
    YF_mesh mesh;
    YF_texture tex;
    YF_skin skin;
    YF_material matl;
} YF_datac;

/* Loads contents from a glTF file. */
int yf_loadgltf(const char *pathname, size_t index, int datac, YF_datac *dst);

/* Loads contents from a file containing glTF data.
   Decoding begins at the file's current position, and all data must be
   embedded in the file, since its path is unknown. */
int yf_loadgltf2(FILE *file, size_t index, int datac, YF_datac *dst);

#endif /* YF_DATA_GLTF_H */
