/*
 * YF
 * yf-light.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_LIGHT_H
#define YF_YF_LIGHT_H

#include "yf/com/yf-defs.h"

#include "yf-node.h"
#include "yf-vector.h"

YF_DECLS_BEGIN

/**
 * Opaque type defining a light source.
 */
typedef struct YF_light_o *YF_light;

/**
 * Types of light source.
 */
#define YF_LIGHTT_POINT  0
#define YF_LIGHTT_SPOT   1
#define YF_LIGHTT_DIRECT 2

/**
 * Initializes a new light source.
 *
 * @param lightt: The 'YF_LIGHTT' value indicating the type of the light.
 * @param color: The color of the light.
 * @param intensity: The intensity of the light.
 * @param range: The range of the light.
 * @param inner_angle: The inner cone angle of the light.
 * @param outer_angle: The outer cone angle of the light.
 * @return: On success, returns a new light source. Otherwise, 'NULL' is
 *  returned and the global error is set to indicate the cause.
 */
YF_light yf_light_init(int lightt, const YF_vec3 color, float intensity,
                       float range, float inner_angle, float outer_angle);

/**
 * Gets the node of a light source.
 *
 * @param light: The light.
 * @return: The light's node.
 */
YF_node yf_light_getnode(YF_light light);

/**
 * Deinitializes a light source.
 *
 * @param light: The light to deinitialize. Can be 'NULL'.
 */
void yf_light_deinit(YF_light light);

YF_DECLS_END

#endif /* YF_YF_LIGHT_H */
