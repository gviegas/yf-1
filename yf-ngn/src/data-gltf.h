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
#include "texture.h"
#include "yf-skin.h"
#include "yf-material.h"

/* Loads contents from a glTF file. */
int yf_loadgltf(const char *pathname, YF_collection coll);

/* Loads mesh data from a glTF file. */
int yf_loadgltf_mesh(const char *pathname, size_t index, YF_meshdt *data);

/* Loads texture data from a glTF file. */
int yf_loadgltf_tex(const char *pathname, size_t index, YF_texdt *data);

/* Loads skin object from a glTF file. */
int yf_loadgltf_skin(const char *pathname, size_t index, YF_skin *skin);

/* Loads material object from a glTF file. */
int yf_loadgltf_matl(const char *pathname, size_t index, YF_material *matl);

#endif /* YF_DATA_GLTF_H */
