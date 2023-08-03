/*
 * YF
 * test-mesh.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>
#include <assert.h>

#include "test.h"
#include "print.h"
#include "mesh.h"

/* Tests mesh. */
/* TODO: More tests. */
int yf_test_mesh(void)
{
    unsigned char buf[2048];

    yf_material_t *matl = yf_material_init(NULL);
    assert(matl != NULL);

    yf_primdt_t prim = {
        .topology = YF_TOPOLOGY_TRIANGLE,
        .vert_n = 6,
        .indx_n = 9,
        .data_off = 0,
        .vsemt_mask = YF_VSEMT_POS | YF_VSEMT_CLR,
        .attrs = (yf_attrdt_t[]){
            [0] = {YF_VSEMT_POS, YF_VFMT_FLOAT3, 0},
            [1] = {YF_VSEMT_CLR, YF_VFMT_FLOAT4, 6 * sizeof(float[3])}
        },
        .attr_n = 2,
        .itype = YF_ITYPE_USHORT,
        .indx_data_off = 6 * sizeof(float[3 + 4]),
        .matl = NULL
    };

    yf_meshdt_t data = {
        .prims = &prim,
        .prim_n = 1,
        .data = buf,
        .data_sz = 6 * sizeof(float[3 + 4]) + 9 * sizeof(unsigned short)
    };

    YF_TEST_PRINT("init", "&data", "mesh");
    yf_mesh_t *mesh = yf_mesh_init(&data);
    if (mesh == NULL)
        return -1;

    yf_print_mesh(mesh);
    yf_print_mesh(NULL);

    YF_TEST_PRINT("getprimn", "mesh", "");
    if (yf_mesh_getprimn(mesh) != 1)
        return -1;

    YF_TEST_PRINT("getmatl", "mesh, 0", "");
    if (yf_mesh_getmatl(mesh, 0) != NULL)
        return -1;

    YF_TEST_PRINT("setmatl", "mesh, 0, matl", "");
    if (yf_mesh_setmatl(mesh, 0, matl) != NULL ||
        yf_mesh_getmatl(mesh, 0) != matl)
        return -1;

    YF_TEST_PRINT("init", "&data", "mesh2");
    yf_mesh_t *mesh2 = yf_mesh_init(&data);
    if (mesh2 == NULL)
        return -1;

    yf_print_mesh(mesh2);
    yf_print_mesh(NULL);

    YF_TEST_PRINT("getprimn", "mesh2", "");
    if (yf_mesh_getprimn(mesh2) != 1)
        return -1;

    YF_TEST_PRINT("getmatl", "mesh2, 0", "");
    if (yf_mesh_getmatl(mesh2, 0) != NULL)
        return -1;

    YF_TEST_PRINT("init", "&data", "mesh3");
    yf_mesh_t *mesh3 = yf_mesh_init(&data);
    if (mesh3 == NULL)
        return -1;

    yf_print_mesh(mesh3);
    yf_print_mesh(NULL);

    YF_TEST_PRINT("getprimn", "mesh3", "");
    if (yf_mesh_getprimn(mesh3) != 1)
        return -1;

    YF_TEST_PRINT("getmatl", "mesh3, 0", "");
    if (yf_mesh_getmatl(mesh3, 0) != NULL)
        return -1;

    prim.matl = matl;

    YF_TEST_PRINT("init", "&data", "mesh4");
    yf_mesh_t *mesh4 = yf_mesh_init(&data);
    if (mesh4 == NULL)
        return -1;

    yf_print_mesh(mesh4);
    yf_print_mesh(NULL);

    YF_TEST_PRINT("getprimn", "mesh4", "");
    if (yf_mesh_getprimn(mesh4) != 1)
        return -1;

    YF_TEST_PRINT("getmatl", "mesh4, 0", "");
    if (yf_mesh_getmatl(mesh4, 0) != matl)
        return -1;

    YF_TEST_PRINT("setmatl", "mesh4, 0, NULL", "");
    if (yf_mesh_setmatl(mesh4, 0, NULL) != matl ||
        yf_mesh_getmatl(mesh4, 0) != NULL)
        return -1;

    YF_TEST_PRINT("deinit", "mesh", "");
    yf_mesh_deinit(mesh);

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh3", "");
    yf_mesh_deinit(mesh3);

    yf_print_mesh(NULL);

    prim.vert_n += 5;
    prim.attrs[1].data_off += 5 * sizeof(float[3]);
    prim.indx_data_off += 5 * sizeof(float[3 + 4]);
    data.data_sz += 5 * sizeof(float[3 + 4]);
    puts("\n- data size increased -");

    YF_TEST_PRINT("init", "&data", "mesh");
    mesh = yf_mesh_init(&data);
    if (mesh == NULL)
        return -1;

    yf_print_mesh(NULL);

    prim.vert_n -= 6;
    prim.attrs[1].data_off -= 6 * sizeof(float[3]);
    prim.indx_data_off -= 6 * sizeof(float[3 + 4]);
    data.data_sz -= 6 * sizeof(float[3 + 4]);
    puts("\n- data size decreased -");

    YF_TEST_PRINT("init", "&data", "mesh3");
    mesh3 = yf_mesh_init(&data);
    if (mesh3 == NULL)
        return -1;

    yf_print_mesh(NULL);

    yf_print_mesh(mesh);
    yf_print_mesh(mesh2);
    yf_print_mesh(mesh3);
    yf_print_mesh(mesh4);

    YF_TEST_PRINT("deinit", "mesh3", "");
    yf_mesh_deinit(mesh3);

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh4", "");
    yf_mesh_deinit(mesh4);

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh2", "");
    yf_mesh_deinit(mesh2);

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh", "");
    yf_mesh_deinit(mesh);

    yf_print_mesh(NULL);

    yf_material_deinit(matl);

    return 0;
}
