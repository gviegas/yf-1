/*
 * YF
 * filetype-obj.h
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#ifndef YF_FILETYPE_OBJ_H
#define YF_FILETYPE_OBJ_H

#include "mesh.h"

/* Loads mesh data from an OBJ file. */
int yf_filetype_obj_load(const char *pathname, YF_meshdt *data);

#endif /* YF_FILETYPE_OBJ_H */
