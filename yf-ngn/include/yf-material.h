/*
 * YF
 * yf-material.h
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#ifndef YF_YF_MATERIAL_H
#define YF_YF_MATERIAL_H

#include "yf-texture.h"
#include "yf-vector.h"

/**
 * Type defining PBR metallic-roughness properties.
 */
typedef struct {
  struct {
    YF_texture tex;
    YF_vec4 factor;
  } color;
  struct {
    YF_texture tex;
    YF_float metalness;
    YF_float roughness;
  } mr;
} YF_pbrmr;

/**
 * Type defining the material properties of an object.
 */
typedef struct {
  YF_pbrmr pbrmr;
  struct {
    YF_texture tex;
    YF_float scale;
  } normal;
  struct {
    YF_texture tex;
    YF_float strength;
  } occlusion;
  struct {
    YF_texture tex;
    YF_vec3 factor;
  } emissive;
} YF_material;

#endif /* YF_YF_MATERIAL_H */
