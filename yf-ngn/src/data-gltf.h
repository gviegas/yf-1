/*
 * YF
 * data-gltf.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_GLTF_H
#define YF_DATA_GLTF_H

#include "yf-collection.h"
#include "mesh.h"

/* Loads contents from a glTF file. */
int yf_loadgltf(const char *pathname, YF_collection coll);

/* Loads mesh data from a glTF file. */
int yf_loadgltf_mesh(const char *pathname, size_t index, YF_meshdt *data);

#endif /* YF_DATA_GLTF_H */
