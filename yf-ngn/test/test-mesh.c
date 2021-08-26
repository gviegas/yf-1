/*
 * YF
 * test-mesh.c
 *
 * Copyright © 2021 Gustavo C. Viegas.
 */

#include <stdio.h>

#include "test.h"
#include "print.h"
#include "mesh.h"

/* Tests mesh. */
/* TODO: More tests. */
int yf_test_mesh(void)
{
    YF_vmdl verts[128];
    unsigned short inds[256];

    printf("- sizeof(vmdl): %zu -\n", sizeof *verts);
    printf("- sizeof(index): %zu -\n", sizeof *inds);

    YF_meshdt data = {
        .v = {
            .vtype = YF_VTYPE_MDL,
            .data = &verts,
            .n = 6
        },
        .i = {
            .itype = YF_ITYPE_USHORT,
            .data = inds,
            .n = 9
        }
    };

    YF_TEST_PRINT("initdt", "&data", "mesh");
    YF_mesh mesh = yf_mesh_initdt(&data);
    if (mesh == NULL)
        return -1;

    yf_print_mesh(NULL);

    YF_TEST_PRINT("initdt", "&data", "mesh2");
    YF_mesh mesh2 = yf_mesh_initdt(&data);
    if (mesh2 == NULL)
        return -1;

    yf_print_mesh(NULL);

    YF_TEST_PRINT("initdt", "&data", "mesh3");
    YF_mesh mesh3 = yf_mesh_initdt(&data);
    if (mesh3 == NULL)
        return -1;

    yf_print_mesh(NULL);

    YF_TEST_PRINT("initdt", "&data", "mesh4");
    YF_mesh mesh4 = yf_mesh_initdt(&data);
    if (mesh4 == NULL)
        return -1;

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh", "");
    yf_mesh_deinit(mesh);

    yf_print_mesh(NULL);

    YF_TEST_PRINT("deinit", "mesh3", "");
    yf_mesh_deinit(mesh3);

    yf_print_mesh(NULL);

    data.v.n += 5;
    puts("\n- data size increased -");

    YF_TEST_PRINT("initdt", "&data", "mesh");
    mesh = yf_mesh_initdt(&data);
    if (mesh == NULL)
        return -1;

    yf_print_mesh(NULL);

    data.v.n -= 6;
    puts("\n- data size decreased -");

    YF_TEST_PRINT("initdt", "&data", "mesh3");
    mesh3 = yf_mesh_initdt(&data);
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

    return 0;
}
