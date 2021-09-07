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
 * Gets values of a light source.
 *
 * @param light: The light.
 * @param lightt: The destination for the light type value. Can be 'NULL'.
 * @param color: The destination for the color value. Can be 'NULL'.
 * @param intensity: The destination for the intensity value. Can be 'NULL'.
 * @param range: The destination for the range value. Can be 'NULL'.
 * @param inner_angle: The destination for the inner cone angle value.
 *  Can be 'NULL'.
 * @param outer_angle: The destination for the outer cone angle value.
 *  Can be 'NULL'.
 */
void yf_light_getval(YF_light light, int *lightt, YF_vec3 color,
                     float *intensity, float *range, float *inner_angle,
                     float *outer_angle);

/**
 * Sets a given light source as a point light.
 *
 * @param light: The light.
 * @param color: The point light color.
 * @param intensity: The point light intensity.
 * @param range: The point light range.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_light_setpoint(YF_light light, const YF_vec3 color, float intensity,
                      float range);

/**
 * Sets a given light source as a spot light.
 *
 * @param light: The light.
 * @param color: The spot light color.
 * @param intensity: The spot light intensity.
 * @param range: The spot light range.
 * @param inner_angle: The spot light inner cone angle.
 * @param outer_angle: The spot light outer cone angle.
 * @return: On success, returns zero. Otherwise, a non-zero value is returned
 *  and the global error is set to indicate the cause.
 */
int yf_light_setspot(YF_light light, const YF_vec3 color, float intensity,
                     float range, float inner_angle, float outer_angle);

/**
 * Deinitializes a light source.
 *
 * @param light: The light to deinitialize. Can be 'NULL'.
 */
void yf_light_deinit(YF_light light);

YF_DECLS_END

#endif /* YF_YF_LIGHT_H */
