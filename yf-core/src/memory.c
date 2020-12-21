/*
 * YF
 * memory.c
 *
 * Copyright © 2020 Gustavo C. Viegas.
 */

#include <assert.h>

#include <yf/com/yf-error.h>

#include "memory.h"
#include "context.h"
#include "buffer.h"
#include "image.h"
#include "vk.h"

/* Selects a suitable memory heap. */
static int select_memory(YF_context ctx, unsigned requirement,
    VkFlags properties);

/* Allocates device memory. */
static VkDeviceMemory alloc_memory(YF_context ctx,
    const VkMemoryRequirements *requirements, int host_visible);

/* Deallocates device memory. */
static void free_memory(YF_context ctx, VkDeviceMemory memory);

int yf_buffer_alloc(YF_buffer buf) {
  assert(buf != NULL);
  assert(buf->memory == NULL);

  VkMemoryRequirements mem_req;
  vkGetBufferMemoryRequirements(buf->ctx->device, buf->buffer, &mem_req);
  buf->memory = alloc_memory(buf->ctx, &mem_req, 1);
  if (buf->memory == NULL)
    return -1;

  VkResult res;
  res = vkBindBufferMemory(buf->ctx->device, buf->buffer, buf->memory, 0);
  if (res != VK_SUCCESS) {
    free_memory(buf->ctx, buf->memory);
    return -1;
  }
  res = vkMapMemory(buf->ctx->device, buf->memory, 0, VK_WHOLE_SIZE, 0,
      &buf->data);
  if (res != VK_SUCCESS) {
    free_memory(buf->ctx, buf->memory);
    return -1;
  }
  return 0;
}

int yf_image_alloc(YF_image img) {
  assert(img != NULL);
  assert(img->memory == NULL);

  /* XXX: Since linear tiling is not being used for now, host-visible memory
     is not necessary. As a consequence, the memory will not be mapped. This
     will have to change when linear tiling is used. */

  VkMemoryRequirements mem_req;
  vkGetImageMemoryRequirements(img->ctx->device, img->image, &mem_req);
  img->memory = alloc_memory(img->ctx, &mem_req, 0);
  if (img->memory == NULL)
    return -1;

  VkResult res;
  res = vkBindImageMemory(img->ctx->device, img->image, img->memory, 0);
  if (res != VK_SUCCESS) {
    free_memory(img->ctx, img->memory);
    return -1;
  }
  return 0;
}

void yf_buffer_free(YF_buffer buf) {
  if (buf != NULL) {
    free_memory(buf->ctx, buf->memory);
    buf->memory = NULL;
    buf->data = NULL;
  }
}

void yf_image_free(YF_image img) {
  if (img != NULL) {
    free_memory(img->ctx, img->memory);
    img->memory = NULL;
  }
}

static int select_memory(YF_context ctx, unsigned requirement,
    VkFlags properties)
{
  int mem_type = -1;
  for (unsigned i = 0; i < ctx->mem_prop.memoryTypeCount; ++i) {
    if (requirement & (1 << i)) {
      VkFlags prop_flags = ctx->mem_prop.memoryTypes[i].propertyFlags;
      if ((prop_flags & properties) == properties) {
        mem_type = i;
        break;
      }
    }
  }
  return mem_type;
}

static VkDeviceMemory alloc_memory(YF_context ctx,
    const VkMemoryRequirements *requirements, int host_visible)
{
  VkFlags prop = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  int mem_type = -1;

  if (host_visible)
    prop |=
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  mem_type = select_memory(ctx, requirements->memoryTypeBits, prop);
  if (mem_type == -1) {
    prop &= ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    mem_type = select_memory(ctx, requirements->memoryTypeBits, prop);
    if (mem_type == -1) {
      yf_seterr(YF_ERR_DEVGEN, __func__);
      return NULL;
    }
  }

  VkMemoryAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = NULL,
    .allocationSize = requirements->size,
    .memoryTypeIndex = mem_type
  };

  VkDeviceMemory mem;
  VkResult res = vkAllocateMemory(ctx->device, &info, NULL, &mem);
  if (res != VK_SUCCESS) {
    yf_seterr(YF_ERR_NOMEM, __func__);
    return NULL;
  }
  return mem;
}

static void free_memory(YF_context ctx, VkDeviceMemory memory) {
  vkFreeMemory(ctx->device, memory, NULL);
}
