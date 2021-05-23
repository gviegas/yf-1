/*
 * YF
 * stage.c
 *
 * Copyright © 2020-2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "yf/com/yf-dict.h"
#include "yf/com/yf-error.h"

#include "stage.h"
#include "context.h"

#define YF_MODWRD 4

/* Type defining stage variables stored in a context. */
typedef struct {
    YF_dict mods;
    YF_modid modid;
} T_priv;

/* Destroys the 'T_priv' data stored in a given context. */
static void destroy_priv(YF_context ctx)
{
    assert(ctx != NULL);

    if (ctx->stg.priv == NULL)
        return;

    T_priv *priv = ctx->stg.priv;
    yf_dict_deinit(priv->mods);
    free(priv);
    ctx->stg.priv = NULL;
}

int yf_loadmod(YF_context ctx, const char *pathname, YF_modid *mod)
{
    assert(ctx != NULL);
    assert(pathname != NULL);

    T_priv *priv = ctx->stg.priv;

    if (priv == NULL) {
        if ((ctx->stg.priv = malloc(sizeof *priv)) == NULL) {
            yf_seterr(YF_ERR_NOMEM, __func__);
            return -1;
        }

        priv = ctx->stg.priv;

        if ((priv->mods = yf_dict_init(NULL, NULL)) == NULL) {
            free(ctx->stg.priv);
            ctx->stg.priv = NULL;
            return -1;
        }

        priv->modid = 0;
        ctx->stg.deinit_callb = destroy_priv;
    }

    FILE *file = fopen(pathname, "r");

    if (file == NULL) {
        yf_seterr(YF_ERR_NOFILE, __func__);
        return -1;
    }

    long n = 0;

    if (fseek(file, 0, SEEK_END) != 0 || (n = ftell(file)) <= 0 ||
        n % YF_MODWRD != 0) {
        yf_seterr(YF_ERR_INVFILE, __func__);
        fclose(file);
        return -1;
    }

    rewind(file);
    unsigned char *buf = malloc(n);

    if (buf == NULL) {
        yf_seterr(YF_ERR_NOMEM, __func__);
        fclose(file);
        return -1;
    }

    if (fread(buf, 1, n, file) != (size_t)n) {
        yf_seterr(YF_ERR_OTHER, __func__);
        free(buf);
        fclose(file);
        return -1;
    }

    fclose(file);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = n,
        .pCode = (const uint32_t *)buf
    };

    VkShaderModule val;
    VkResult res = vkCreateShaderModule(ctx->device, &info, NULL, &val);
    free(buf);

    if (res != VK_SUCCESS) {
        yf_seterr(YF_ERR_DEVGEN, __func__);
        return -1;
    }

    const YF_modid key = ++priv->modid;

    if (yf_dict_insert(priv->mods, (void *)key, (void *)val) != 0) {
        *mod = 0;
        return -1;
    }

    *mod = key;
    return 0;
}

void yf_unldmod(YF_context ctx, YF_modid mod)
{
    assert(ctx != NULL);

    if (ctx->stg.priv == NULL)
        return;

    T_priv *priv = ctx->stg.priv;

    if (yf_dict_contains(priv->mods, (void *)mod)) {
        VkShaderModule val = yf_dict_remove(priv->mods, (void *)mod);
        vkDestroyShaderModule(ctx->device, val, NULL);
    }
}

VkShaderModule yf_getmod(YF_context ctx, YF_modid mod)
{
    assert(ctx != NULL);

    if (ctx->stg.priv == NULL)
        return VK_NULL_HANDLE;

    T_priv *priv = ctx->stg.priv;
    void *val = yf_dict_search(priv->mods, (void *)mod);

    if (val != NULL)
        return (VkShaderModule)val;

    return VK_NULL_HANDLE;
}
