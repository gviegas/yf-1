/*
 * YF
 * data-obj.h
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#ifndef YF_DATA_OBJ_H
#define YF_DATA_OBJ_H

#include "mesh.h"

/* Loads mesh data from an OBJ file. */
int yf_loadobj(const char *pathname, YF_meshdt *data);

#endif /* YF_DATA_OBJ_H */
