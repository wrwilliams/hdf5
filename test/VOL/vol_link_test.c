/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "vol_link_test.h"

static int test_create_hard_link(void);
static int test_create_hard_link_same_loc(void);
static int test_create_soft_link_existing_relative(void);
static int test_create_soft_link_existing_absolute(void);
static int test_create_soft_link_dangling_relative(void);
static int test_create_soft_link_dangling_absolute(void);
static int test_create_external_link(void);
static int test_create_dangling_external_link(void);
static int test_create_user_defined_link(void);
static int test_delete_link(void);
static int test_copy_link(void);
static int test_move_link(void);
static int test_get_link_info(void);
static int test_get_link_name(void);
static int test_get_link_val(void);
static int test_link_iterate(void);
static int test_link_iterate_0_links(void);
static int test_link_visit(void);
static int test_link_visit_cycles(void);
static int test_link_visit_0_links(void);
static int test_unused_link_API_calls(void);

static herr_t link_iter_callback1(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);
static herr_t link_iter_callback2(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);
static herr_t link_iter_callback3(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);

static herr_t link_visit_callback1(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);
static herr_t link_visit_callback2(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);
static herr_t link_visit_callback3(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data);

/*
 * The array of link tests to be performed.
 */
static int (*link_tests[])(void) = {
        test_create_hard_link,
        test_create_hard_link_same_loc,
        test_create_soft_link_existing_relative,
        test_create_soft_link_existing_absolute,
        test_create_soft_link_dangling_relative,
        test_create_soft_link_dangling_absolute,
        test_create_external_link,
        test_create_dangling_external_link,
        test_create_user_defined_link,
        test_delete_link,
        test_copy_link,
        test_move_link,
        test_get_link_info,
        test_get_link_name,
        test_get_link_val,
        test_link_iterate,
        test_link_iterate_0_links,
        test_link_visit,
        test_link_visit_cycles,
        test_link_visit_0_links,
        test_unused_link_API_calls,
};

/*
 * A test to check that a hard link can be created.
 */
static int
test_create_hard_link(void)
{
    htri_t link_exists;
    hid_t  file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t  container_group = H5I_INVALID_HID;

    TESTING("create hard link")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if (H5Lcreate_hard(file_id, "/", container_group, HARD_LINK_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create hard link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(container_group, HARD_LINK_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that behavior is correct when using
 * the H5L_SAME_LOC macro for H5Lcreate_hard().
 */
static int
test_create_hard_link_same_loc(void)
{
    hsize_t dims[H5L_SAME_LOC_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   space_id = H5I_INVALID_HID;

    TESTING("create hard link with H5L_SAME_LOC")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, H5L_SAME_LOC_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    memset(dims, 0, sizeof(dims));
    for (i = 0; i < H5L_SAME_LOC_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(H5L_SAME_LOC_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, H5L_SAME_LOC_TEST_DSET_NAME, dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#if 0 /* Library functionality for this part of the test is broken */
    if (H5Lcreate_hard(H5L_SAME_LOC, H5L_SAME_LOC_TEST_DSET_NAME, group_id, H5L_SAME_LOC_TEST_LINK_NAME1, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create first link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, H5L_SAME_LOC_TEST_LINK_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }
#endif

    if (H5Lcreate_hard(group_id, H5L_SAME_LOC_TEST_DSET_NAME, H5L_SAME_LOC, H5L_SAME_LOC_TEST_LINK_NAME2, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create second link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, H5L_SAME_LOC_TEST_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(space_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a soft link, which points to an
 * existing object with a relative path, can be created.
 */
static int
test_create_soft_link_existing_relative(void)
{
    hsize_t dims[SOFT_LINK_EXISTING_RELATIVE_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;

    TESTING("create soft link to existing object by relative path")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, SOFT_LINK_EXISTING_RELATIVE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < SOFT_LINK_EXISTING_RELATIVE_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(SOFT_LINK_EXISTING_RELATIVE_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, SOFT_LINK_EXISTING_RELATIVE_TEST_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if (H5Lcreate_soft(SOFT_LINK_EXISTING_RELATIVE_TEST_DSET_NAME, group_id,
            SOFT_LINK_EXISTING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, SOFT_LINK_EXISTING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if ((dset_id = H5Dopen2(group_id, SOFT_LINK_EXISTING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset through the soft link\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a soft link, which points to an
 * existing object using an absolute path, can be created.
 */
static int
test_create_soft_link_existing_absolute(void)
{
    htri_t link_exists;
    hid_t  file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t  container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID, root_id = H5I_INVALID_HID;

    TESTING("create soft link to existing object by absolute path")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, SOFT_LINK_EXISTING_ABSOLUTE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_soft("/", group_id, SOFT_LINK_EXISTING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(file_id,
            "/" LINK_TEST_GROUP_NAME "/" SOFT_LINK_EXISTING_ABSOLUTE_TEST_SUBGROUP_NAME "/" SOFT_LINK_EXISTING_ABSOLUTE_TEST_LINK_NAME,
            H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if ((root_id = H5Gopen2(group_id, SOFT_LINK_EXISTING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open object pointed to by soft link\n");
        goto error;
    }

    if (H5Gclose(root_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(root_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a soft link, which points to
 * an object that doesn't exist by using a relative
 * path, can be created.
 */
static int
test_create_soft_link_dangling_relative(void)
{
    hsize_t dims[SOFT_LINK_DANGLING_RELATIVE_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;

    TESTING("create dangling soft link to object by relative path")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, SOFT_LINK_DANGLING_RELATIVE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_soft(SOFT_LINK_DANGLING_RELATIVE_TEST_DSET_NAME, group_id,
            SOFT_LINK_DANGLING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, SOFT_LINK_DANGLING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Dopen2(group_id, SOFT_LINK_DANGLING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    opened target of dangling link!\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < SOFT_LINK_DANGLING_RELATIVE_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(SOFT_LINK_DANGLING_RELATIVE_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, SOFT_LINK_DANGLING_RELATIVE_TEST_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(group_id, SOFT_LINK_DANGLING_RELATIVE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset pointed to by soft link\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a soft link, which points to an
 * object that doesn't exist by using an absolute path,
 * can be created.
 */
static int
test_create_soft_link_dangling_absolute(void)
{
    hsize_t dims[SOFT_LINK_DANGLING_ABSOLUTE_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;

    TESTING("create dangling soft link to object by absolute path")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, SOFT_LINK_DANGLING_ABSOLUTE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" SOFT_LINK_DANGLING_ABSOLUTE_TEST_SUBGROUP_NAME "/" SOFT_LINK_DANGLING_ABSOLUTE_TEST_DSET_NAME,
            group_id, SOFT_LINK_DANGLING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, SOFT_LINK_DANGLING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Dopen2(group_id, SOFT_LINK_DANGLING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    opened target of dangling link!\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < SOFT_LINK_DANGLING_ABSOLUTE_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(SOFT_LINK_DANGLING_ABSOLUTE_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, SOFT_LINK_DANGLING_ABSOLUTE_TEST_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(group_id, SOFT_LINK_DANGLING_ABSOLUTE_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset pointed to by soft link\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an external link can be created.
 */
static int
test_create_external_link(void)
{
    htri_t link_exists;
    hid_t  file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t  container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t  root_id = H5I_INVALID_HID;
    char   ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("create external link to existing object")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fcreate(ext_link_filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file for external link to reference\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, EXTERNAL_LINK_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id,
            EXTERNAL_LINK_TEST_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, EXTERNAL_LINK_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if ((root_id = H5Gopen2(group_id, EXTERNAL_LINK_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open root group of other file using external link\n");
        goto error;
    }

    if (H5Gclose(root_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(root_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an external link, which points to an
 * object that doesn't exist by using an absolute path, can
 * be created.
 */
static int
test_create_dangling_external_link(void)
{
    hsize_t dims[EXTERNAL_LINK_TEST_DANGLING_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, ext_file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;
    char    ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("create dangling external link")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((ext_file_id = H5Fcreate(ext_link_filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file for external link to reference\n");
        goto error;
    }

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, EXTERNAL_LINK_TEST_DANGLING_SUBGROUP_NAME,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/" EXTERNAL_LINK_TEST_DANGLING_DSET_NAME, group_id,
            EXTERNAL_LINK_TEST_DANGLING_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create dangling external link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, EXTERNAL_LINK_TEST_DANGLING_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Dopen2(group_id, EXTERNAL_LINK_TEST_DANGLING_LINK_NAME, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    opened non-existent dataset in other file using dangling external link!\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < EXTERNAL_LINK_TEST_DANGLING_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(EXTERNAL_LINK_TEST_DANGLING_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR


    if ((dset_id = H5Dcreate2(ext_file_id, EXTERNAL_LINK_TEST_DANGLING_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset in external file\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR


    if ((dset_id = H5Dopen2(group_id, EXTERNAL_LINK_TEST_DANGLING_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset in external file\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR
    if (H5Fclose(ext_file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
        H5Fclose(ext_file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a user-defined link can be created.
 */
static int
test_create_user_defined_link(void)
{
    ssize_t udata_size;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID;
    char    udata[UD_LINK_TEST_UDATA_MAX_SIZE];

    TESTING("create user-defined link")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((udata_size = snprintf(udata, UD_LINK_TEST_UDATA_MAX_SIZE, "udata")) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        err_ret = H5Lcreate_ud(container_group, UD_LINK_TEST_LINK_NAME, H5L_TYPE_HARD, udata, (size_t) udata_size,
                H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(container_group, UD_LINK_TEST_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    link existed!\n");
        goto error;
    }

    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a link can be deleted.
 */
static int
test_delete_link(void)
{
    hsize_t dims[LINK_DELETE_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID, dset_id2 = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;
    char    ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("delete link")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_DELETE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < LINK_DELETE_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(LINK_DELETE_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, LINK_DELETE_TEST_DSET_NAME1, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create first hard link\n");
        goto error;
    }

    if ((dset_id2 = H5Dcreate2(group_id, LINK_DELETE_TEST_DSET_NAME2, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create second hard link\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" LINK_DELETE_TEST_SUBGROUP_NAME "/" LINK_DELETE_TEST_DSET_NAME1,
            group_id, LINK_DELETE_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create first soft link\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" LINK_DELETE_TEST_SUBGROUP_NAME "/" LINK_DELETE_TEST_DSET_NAME2,
            group_id, LINK_DELETE_TEST_SOFT_LINK_NAME2, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create second soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create first external link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME2,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create second external link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    first hard link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second hard link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    first soft link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_SOFT_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second soft link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first external link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    first external link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second external link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second external link did not exist\n");
        goto error;
    }

    if (H5Ldelete(group_id, LINK_DELETE_TEST_DSET_NAME1, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't delete hard link using H5Ldelete\n");
        goto error;
    }

    if (H5Ldelete(group_id, LINK_DELETE_TEST_SOFT_LINK_NAME, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't delete soft link using H5Ldelete\n");
        goto error;
    }

    if (H5Ldelete(group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't delete external link using H5Ldelete\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Ldelete_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Ldelete_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Ldelete_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    /* Verify that all links have been deleted */
    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first hard link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    first hard link exists!\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second hard link did not exist!\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first soft link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    first soft link exists!\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_SOFT_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second soft link did not exist!\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first external link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    first external link exists!\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_DELETE_TEST_EXTERNAL_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second external link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    second external link did not exist!\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Dclose(dset_id2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a link can be copied using H5Lcopy.
 *
 * XXX: external links
 */
static int
test_copy_link(void)
{
    hsize_t dims[COPY_LINK_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   space_id = H5I_INVALID_HID;

    TESTING("copy a link")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, COPY_LINK_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    for (i = 0; i < COPY_LINK_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(COPY_LINK_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, COPY_LINK_TEST_DSET_NAME, dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* Try to copy a hard link */
    if (H5Lcreate_hard(group_id, COPY_LINK_TEST_DSET_NAME, group_id, COPY_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create hard link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, COPY_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    hard link did not exist\n");
        goto error;
    }

    /* Copy the link */
    H5E_BEGIN_TRY {
        err_ret = H5Lcopy(group_id, COPY_LINK_TEST_HARD_LINK_NAME, group_id, COPY_LINK_TEST_HARD_LINK_COPY_NAME, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    /* Verify the link has been copied */
    if ((link_exists = H5Lexists(group_id, COPY_LINK_TEST_HARD_LINK_COPY_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if hard link copy exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    hard link copy did not exist\n");
        goto error;
    }

    /* Try to copy a soft link */
    if (H5Lcreate_soft(COPY_LINK_TEST_SOFT_LINK_TARGET_PATH, group_id, COPY_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, COPY_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    soft link did not exist\n");
        goto error;
    }

    /* Copy the link */
    H5E_BEGIN_TRY {
        err_ret = H5Lcopy(group_id, COPY_LINK_TEST_SOFT_LINK_NAME, group_id, COPY_LINK_TEST_SOFT_LINK_COPY_NAME, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    /* Verify the link has been copied */
    if ((link_exists = H5Lexists(group_id, COPY_LINK_TEST_SOFT_LINK_COPY_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if soft link copy exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    soft link copy did not exist\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(space_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a link can be moved with H5Lmove.
 */
static int
test_move_link(void)
{
    hsize_t dims[MOVE_LINK_TEST_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   space_id = H5I_INVALID_HID;

    TESTING("move a link")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, MOVE_LINK_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    for (i = 0; i < MOVE_LINK_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(MOVE_LINK_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, MOVE_LINK_TEST_DSET_NAME, dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* Try to move a hard link */
    if (H5Lcreate_hard(group_id, MOVE_LINK_TEST_DSET_NAME, file_id, MOVE_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create hard link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(file_id, MOVE_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    hard link did not exist\n");
        goto error;
    }

    /* Move the link */
    H5E_BEGIN_TRY {
        err_ret = H5Lmove(file_id, MOVE_LINK_TEST_HARD_LINK_NAME, group_id, MOVE_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    /* Verify the link has been moved */
    if ((link_exists = H5Lexists(group_id, MOVE_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    hard link did not exist\n");
        goto error;
    }

    /* Verify the old link is gone */
    if ((link_exists = H5Lexists(file_id, MOVE_LINK_TEST_HARD_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if old hard link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    old hard link exists\n");
        goto error;
    }

    /* Try to move a soft link */
    if (H5Lcreate_soft(MOVE_LINK_TEST_SOFT_LINK_TARGET_PATH, file_id, MOVE_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(file_id, MOVE_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    soft link did not exist\n");
        goto error;
    }

    /* Move the link */
    H5E_BEGIN_TRY {
        err_ret = H5Lmove(file_id, MOVE_LINK_TEST_SOFT_LINK_NAME, group_id, MOVE_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    /* Verify the link has been moved */
    if ((link_exists = H5Lexists(group_id, MOVE_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    soft link did not exist\n");
        goto error;
    }

    /* Verify the old link is gone */
    if ((link_exists = H5Lexists(file_id, MOVE_LINK_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if old soft link exists\n");
        goto error;
    }

    if (link_exists) {
        H5_FAILED();
        printf("    old soft link exists\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(space_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of H5Lget_info and
 * H5Lget_info_by_idx.
 */
static int
test_get_link_info(void)
{
    H5L_info_t link_info;
    hsize_t    dims[GET_LINK_INFO_TEST_DSET_SPACE_RANK];
    size_t     i;
    htri_t     link_exists;
    herr_t     err_ret = -1;
    hid_t      file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t      container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t      dset_id = H5I_INVALID_HID;
    hid_t      dset_dtype = H5I_INVALID_HID;
    hid_t      dset_dspace = H5I_INVALID_HID;
    char       ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("get link info")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, GET_LINK_INFO_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < GET_LINK_INFO_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(GET_LINK_INFO_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, GET_LINK_INFO_TEST_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" GET_LINK_INFO_TEST_SUBGROUP_NAME "/" GET_LINK_INFO_TEST_DSET_NAME,
            group_id, GET_LINK_INFO_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id, GET_LINK_INFO_TEST_EXT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(group_id, GET_LINK_INFO_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if hard link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    hard link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, GET_LINK_INFO_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if soft link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    soft link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, GET_LINK_INFO_TEST_EXT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if external link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    external link did not exist\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(group_id, GET_LINK_INFO_TEST_DSET_NAME, &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get hard link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_HARD) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(file_id, "/" LINK_TEST_GROUP_NAME "/" GET_LINK_INFO_TEST_SUBGROUP_NAME "/" GET_LINK_INFO_TEST_SOFT_LINK_NAME,
            &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get soft link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_SOFT) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(group_id, GET_LINK_INFO_TEST_EXT_LINK_NAME, &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get external link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_EXTERNAL) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    H5E_BEGIN_TRY {
        err_ret = H5Lget_info_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, &link_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_HARD) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    H5E_BEGIN_TRY {
        err_ret = H5Lget_info_by_idx(file_id, "/" LINK_TEST_GROUP_NAME "/" GET_LINK_INFO_TEST_SUBGROUP_NAME,
                H5_INDEX_CRT_ORDER, H5_ITER_DEC, 1, &link_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_SOFT) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    H5E_BEGIN_TRY {
        err_ret = H5Lget_info_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_DEC, 2, &link_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_EXTERNAL) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a link's name can be correctly
 * retrieve by using H5Lget_name_by_idx.
 */
static int
test_get_link_name(void)
{
    hsize_t dims[GET_LINK_NAME_TEST_DSET_SPACE_RANK];
    ssize_t ret;
    size_t  i;
    htri_t  link_exists;
    size_t  link_name_buf_size = 0;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;
    char   *link_name_buf = NULL;

    TESTING("get link name")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, GET_LINK_NAME_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < GET_LINK_NAME_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(GET_LINK_NAME_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, GET_LINK_NAME_TEST_DSET_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* Verify the link has been created */
    if ((link_exists = H5Lexists(group_id, GET_LINK_NAME_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ret = H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, NULL, link_name_buf_size, H5P_DEFAULT);
    } H5E_END_TRY;

    if (ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    link_name_buf_size = (size_t) ret;
    if (NULL == (link_name_buf = (char *) malloc(link_name_buf_size)))
        TEST_ERROR

    H5E_BEGIN_TRY {
        ret = H5Lget_name_by_idx(group_id, ".", H5_INDEX_NAME, H5_ITER_INC, 0, link_name_buf, link_name_buf_size, H5P_DEFAULT);
    } H5E_END_TRY;

    if (ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    if (strcmp(link_name_buf, GET_LINK_NAME_TEST_DSET_NAME)) {
        H5_FAILED();
        printf("    link name did not match\n");
        goto error;
    }

    if (link_name_buf) {
        free(link_name_buf);
        link_name_buf = NULL;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        if (link_name_buf) free(link_name_buf);
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a soft or external link's value can
 * be retrieved by using H5Lget_val and H5Lget_val_by_idx.
 */
static int
test_get_link_val(void)
{
    H5L_info_t  link_info;
    const char *ext_link_filepath;
    const char *ext_link_val;
    unsigned    ext_link_flags;
    htri_t      link_exists;
    herr_t      err_ret = -1;
    size_t      link_val_buf_size;
    char       *link_val_buf = NULL;
    hid_t       file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t       container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    char        ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("get link value")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, GET_LINK_VAL_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" GET_LINK_VAL_TEST_SUBGROUP_NAME, group_id, GET_LINK_VAL_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id, GET_LINK_VAL_TEST_EXT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(group_id, GET_LINK_VAL_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, GET_LINK_VAL_TEST_EXT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if external link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    external link did not exist\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(group_id, GET_LINK_VAL_TEST_SOFT_LINK_NAME, &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get soft link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_SOFT) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    link_val_buf_size = link_info.u.val_size;
    if (NULL == (link_val_buf = (char *) malloc(link_val_buf_size)))
        TEST_ERROR

    if (H5Lget_val(group_id, GET_LINK_VAL_TEST_SOFT_LINK_NAME, link_val_buf, link_val_buf_size, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get soft link val\n");
        goto error;
    }

    if (strcmp(link_val_buf, "/" LINK_TEST_GROUP_NAME "/" GET_LINK_VAL_TEST_SUBGROUP_NAME)) {
        H5_FAILED();
        printf("    soft link value did not match\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(group_id, GET_LINK_VAL_TEST_EXT_LINK_NAME, &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get external link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_EXTERNAL) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    if (link_info.u.val_size > link_val_buf_size) {
        char *tmp_realloc;

        link_val_buf_size *= 2;

        if (NULL == (tmp_realloc = (char *) realloc(link_val_buf, link_val_buf_size)))
            TEST_ERROR
        link_val_buf = tmp_realloc;
    }

    if (H5Lget_val(group_id, GET_LINK_VAL_TEST_EXT_LINK_NAME, link_val_buf, link_val_buf_size, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get external link val\n");
        goto error;
    }

    if (H5Lunpack_elink_val(link_val_buf, link_val_buf_size, &ext_link_flags, &ext_link_filepath, &ext_link_val) < 0) {
        H5_FAILED();
        printf("    couldn't unpack external link value buffer\n");
        goto error;
    }

    if (strcmp(ext_link_filepath, ext_link_filename)) {
        H5_FAILED();
        printf("    external link target file did not match\n");
        goto error;
    }

    if (strcmp(ext_link_val, "/")) {
        H5_FAILED();
        printf("    external link value did not match\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    H5E_BEGIN_TRY {
        err_ret = H5Lget_info(group_id, GET_LINK_VAL_TEST_SOFT_LINK_NAME, &link_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret < 0) {
        H5_FAILED();
        printf("    couldn't get soft link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_SOFT) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    if (link_info.u.val_size > link_val_buf_size) {
        char *tmp_realloc;

        link_val_buf_size *= 2;

        if (NULL == (tmp_realloc = (char *) realloc(link_val_buf, link_val_buf_size)))
            TEST_ERROR
            link_val_buf = tmp_realloc;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Lget_val_by_idx(group_id, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, link_val_buf, link_val_buf_size, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (strcmp(link_val_buf, "/" LINK_TEST_GROUP_NAME "/" GET_LINK_VAL_TEST_SUBGROUP_NAME)) {
        H5_FAILED();
        printf("    soft link value did not match\n");
        goto error;
    }

    memset(&link_info, 0, sizeof(link_info));

    if (H5Lget_info(group_id, GET_LINK_VAL_TEST_EXT_LINK_NAME, &link_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get external link info\n");
        goto error;
    }

    if (link_info.type != H5L_TYPE_EXTERNAL) {
        H5_FAILED();
        printf("    incorrect link type returned\n");
        goto error;
    }

    if (link_info.u.val_size > link_val_buf_size) {
        char *tmp_realloc;

        link_val_buf_size *= 2;

        if (NULL == (tmp_realloc = (char *) realloc(link_val_buf, link_val_buf_size)))
            TEST_ERROR
            link_val_buf = tmp_realloc;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Lget_val_by_idx(group_id, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC, 0, link_val_buf, link_val_buf_size, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    {
        const char *link_filename_retrieved = NULL;

        if (H5Lunpack_elink_val(link_val_buf, link_val_buf_size, &ext_link_flags, &link_filename_retrieved, &ext_link_val) < 0) {
            H5_FAILED();
            printf("    couldn't unpack external link value buffer\n");
            goto error;
        }

        if (strcmp(ext_link_filename, ext_link_filename)) {
            H5_FAILED();
            printf("    external link target file did not match\n");
            goto error;
        }

        if (strcmp(ext_link_val, "/")) {
            H5_FAILED();
            printf("    external link value did not match\n");
            goto error;
        }
    }

    if (link_val_buf) {
        free(link_val_buf);
        link_val_buf = NULL;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        if (link_val_buf) free(link_val_buf);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}


/*
 * A test to check the functionality of link
 * iteration using H5Literate. Iteration is done
 * in increasing and decreasing order of both
 * link name and link creation order.
 */
static int
test_link_iterate(void)
{
    hsize_t dims[LINK_ITER_TEST_DSET_SPACE_RANK];
    hsize_t saved_idx = 0;
    size_t  i;
    htri_t  link_exists;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   gcpl_id = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   dset_dspace = H5I_INVALID_HID;
    int     halted = 0;
    char    ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("link iteration")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((gcpl_id = H5Pcreate(H5P_GROUP_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create GCPL for link creation order tracking\n");
        goto error;
    }

    if (H5Pset_link_creation_order(gcpl_id, H5P_CRT_ORDER_TRACKED) < 0) {
        H5_FAILED();
        printf("    couldn't set link creation order tracking\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_ITER_TEST_SUBGROUP_NAME, H5P_DEFAULT, gcpl_id, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < LINK_ITER_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_dspace = H5Screate_simple(LINK_ITER_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, LINK_ITER_TEST_HARD_LINK_NAME, dset_dtype, dset_dspace,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create hard link\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_SUBGROUP_NAME "/" LINK_ITER_TEST_HARD_LINK_NAME,
            group_id, LINK_ITER_TEST_SOFT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", group_id, LINK_ITER_TEST_EXT_LINK_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(group_id, LINK_ITER_TEST_HARD_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 1 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_ITER_TEST_SOFT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 2 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(group_id, LINK_ITER_TEST_EXT_LINK_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 3 did not exist\n");
        goto error;
    }

    /* Test basic link iteration capability using both index types and both index orders */
    if (H5Literate(group_id, H5_INDEX_NAME, H5_ITER_INC, NULL, link_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_NAME, H5_ITER_DEC, NULL, link_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, link_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, link_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_INC, NULL, link_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, NULL, link_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, link_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, link_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }

    /* Test the H5Literate index-saving capabilities */
    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, &saved_idx, link_iter_callback2, &halted) < 0) {
        H5_FAILED();
        printf("    H5Literate index-saving capability test failed\n");
        goto error;
    }

    if (saved_idx != 2) {
        H5_FAILED();
        printf("    saved index after iteration was wrong\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, &saved_idx, link_iter_callback2, &halted) < 0) {
        H5_FAILED();
        printf("    couldn't finish iterating\n");
        goto error;
    }

    saved_idx = LINK_ITER_TEST_NUM_LINKS - 1;
    halted = 0;

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, &saved_idx, link_iter_callback2, &halted) < 0) {
        H5_FAILED();
        printf("    H5Literate index-saving capability test failed\n");
        goto error;
    }

    if (saved_idx != 2) {
        H5_FAILED();
        printf("    saved index after iteration was wrong\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, &saved_idx, link_iter_callback2, &halted) < 0) {
        H5_FAILED();
        printf("    couldn't finish iterating\n");
        goto error;
    }

    if (H5Sclose(dset_dspace) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Pclose(gcpl_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(dset_dspace);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Pclose(gcpl_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that link iteration performed on a
 * group with no links in it is not problematic.
 */
static int
test_link_iterate_0_links(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t gcpl_id = H5I_INVALID_HID;

    TESTING("link iteration on group with 0 links")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((gcpl_id = H5Pcreate(H5P_GROUP_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create GCPL for link creation order tracking\n");
        goto error;
    }

    if (H5Pset_link_creation_order(gcpl_id, H5P_CRT_ORDER_TRACKED) < 0) {
        H5_FAILED();
        printf("    couldn't set link creation order tracking\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_ITER_TEST_0_LINKS_SUBGROUP_NAME, H5P_DEFAULT, gcpl_id, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    /* Test basic link iteration capability using both index types and both index orders */
    if (H5Literate(group_id, H5_INDEX_NAME, H5_ITER_INC, NULL, link_iter_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_NAME, H5_ITER_DEC, NULL, link_iter_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, link_iter_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Literate(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, link_iter_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Literate by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_INC, NULL, link_iter_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, NULL, link_iter_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, link_iter_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Literate_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_ITER_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, link_iter_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Literate_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Pclose(gcpl_id) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(gcpl_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of recursive
 * link iteration using H5Lvisit where there are no
 * cyclic links. Iteration is done in increasing and
 * decreasing order of both link name and link creation
 * order.
 */
static int
test_link_visit(void)
{
    hsize_t dims[LINK_VISIT_TEST_NO_CYCLE_DSET_SPACE_RANK];
    size_t  i;
    htri_t  link_exists;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t   subgroup1 = H5I_INVALID_HID, subgroup2 = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   dset_dtype = H5I_INVALID_HID;
    hid_t   fspace_id = H5I_INVALID_HID;
    char    ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("link visit without cycles")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((subgroup1 = H5Gcreate2(group_id, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create first subgroup\n");
        goto error;
    }

    if ((subgroup2 = H5Gcreate2(group_id, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create second subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < LINK_VISIT_TEST_NO_CYCLE_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(LINK_VISIT_TEST_NO_CYCLE_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(subgroup1, LINK_VISIT_TEST_NO_CYCLE_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create first dataset\n");
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(subgroup2, LINK_VISIT_TEST_NO_CYCLE_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create second dataset\n");
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if (H5Lcreate_hard(subgroup1, LINK_VISIT_TEST_NO_CYCLE_DSET_NAME, subgroup1, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME1,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create first hard link\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_NO_CYCLE_DSET_NAME,
            subgroup1, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME2, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", subgroup2, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME3, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    if (H5Lcreate_hard(subgroup2, LINK_VISIT_TEST_NO_CYCLE_DSET_NAME, subgroup2, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME4,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create second hard link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(subgroup1, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 1 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup1, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 2 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup2, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME3, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if third link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 3 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup2, LINK_VISIT_TEST_NO_CYCLE_LINK_NAME4, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if fourth link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 4 did not exist\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_INC, link_visit_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_INC, link_visit_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Gclose(subgroup1) < 0)
        TEST_ERROR
    if (H5Gclose(subgroup2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Sclose(fspace_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(subgroup1);
        H5Gclose(subgroup2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of recursive
 * link iteration using H5Lvisit where there are
 * cyclic links. Iteration is done in increasing
 * and decreasing order of both link name and link
 * creation order.
 */
static int
test_link_visit_cycles(void)
{
    htri_t link_exists;
    hid_t  file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t  container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t  subgroup1 = H5I_INVALID_HID, subgroup2 = H5I_INVALID_HID;
    char   ext_link_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("link visit with cycles")

    snprintf(ext_link_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", EXTERNAL_LINK_TEST_FILE_NAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((subgroup1 = H5Gcreate2(group_id, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create first subgroup\n");
        goto error;
    }

    if ((subgroup2 = H5Gcreate2(group_id, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create second subgroup\n");
        goto error;
    }

    if (H5Lcreate_hard(group_id, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2, subgroup1, LINK_VISIT_TEST_CYCLE_LINK_NAME1,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create first hard link\n");
        goto error;
    }

    if (H5Lcreate_soft("/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2,
            subgroup1, LINK_VISIT_TEST_CYCLE_LINK_NAME2, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create soft link\n");
        goto error;
    }

    if (H5Lcreate_external(ext_link_filename, "/", subgroup2, LINK_VISIT_TEST_CYCLE_LINK_NAME3, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create external link\n");
        goto error;
    }

    if (H5Lcreate_hard(group_id, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME3, subgroup2, LINK_VISIT_TEST_CYCLE_LINK_NAME4,
            H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create second hard link\n");
        goto error;
    }

    /* Verify the links have been created */
    if ((link_exists = H5Lexists(subgroup1, LINK_VISIT_TEST_CYCLE_LINK_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if first link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 1 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup1, LINK_VISIT_TEST_CYCLE_LINK_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if second link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 2 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup2, LINK_VISIT_TEST_CYCLE_LINK_NAME3, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if third link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 3 did not exist\n");
        goto error;
    }

    if ((link_exists = H5Lexists(subgroup2, LINK_VISIT_TEST_CYCLE_LINK_NAME4, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if fourth link exists\n");
        goto error;
    }

    if (!link_exists) {
        H5_FAILED();
        printf("    link 4 did not exist\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_INC, link_visit_callback2, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback2, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback2, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback2, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_INC, link_visit_callback2, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback2, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback2, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback2, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Gclose(subgroup1) < 0)
        TEST_ERROR
    if (H5Gclose(subgroup2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(subgroup1);
        H5Gclose(subgroup2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that recursive link iteration
 * performed on a group with no links in it is
 * not problematic.
 */
static int
test_link_visit_0_links(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t subgroup1 = H5I_INVALID_HID, subgroup2 = H5I_INVALID_HID;

    TESTING("link visit on group with subgroups containing 0 links")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((subgroup1 = H5Gcreate2(group_id, LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create first subgroup\n");
        goto error;
    }

    if ((subgroup2 = H5Gcreate2(group_id, LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME3, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create second subgroup\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_INC, link_visit_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit(group_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback3, NULL) < 0) {
        H5_FAILED();
        printf("    H5Lvisit by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_INC, link_visit_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, link_visit_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type name in decreasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, link_visit_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

    if (H5Lvisit_by_name(file_id, "/" LINK_TEST_GROUP_NAME "/" LINK_VISIT_TEST_0_LINKS_SUBGROUP_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, link_visit_callback3, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Lvisit_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }

    if (H5Gclose(subgroup1) < 0)
        TEST_ERROR
    if (H5Gclose(subgroup2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(subgroup1);
        H5Gclose(subgroup2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_unused_link_API_calls(void)
{
    TESTING("unused link API calls")

    /* None currently that aren't planned to be used */
    SKIPPED();

    return 0;
}

/*
 * Link iteration callback to simply iterate through all of the links in a
 * group and check to make sure their names and link classes match what is
 * expected.
 */
static herr_t
link_iter_callback1(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    UNUSED(group_id);

    if (!strcmp(name, LINK_ITER_TEST_HARD_LINK_NAME)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_ITER_TEST_SOFT_LINK_NAME)) {
        if (H5L_TYPE_SOFT != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_ITER_TEST_EXT_LINK_NAME)) {
        if (H5L_TYPE_EXTERNAL != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else {
        H5_FAILED();
        printf("    link name didn't match known names\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

/*
 * Link iteration callback to test that the index-saving behavior of H5Literate
 * works correctly.
 */
static herr_t
link_iter_callback2(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    int *broken = (int *) op_data;

    UNUSED(group_id);

    if (broken && !*broken && !strcmp(name, LINK_ITER_TEST_EXT_LINK_NAME)) {
        return (*broken = 1);
    }

    if (!strcmp(name, LINK_ITER_TEST_HARD_LINK_NAME)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_ITER_TEST_SOFT_LINK_NAME)) {
        if (H5L_TYPE_SOFT != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_ITER_TEST_EXT_LINK_NAME)) {
        if (H5L_TYPE_EXTERNAL != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else {
        H5_FAILED();
        printf("    link name didn't match known names\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

static herr_t
link_iter_callback3(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    UNUSED(group_id);
    UNUSED(name);
    UNUSED(info);
    UNUSED(op_data);

    return 0;
}

/*
 * Link visit callback to simply iterate recursively through all of the links in a
 * group and check to make sure their names and link classes match what is expected
 * expected.
 */
static herr_t
link_visit_callback1(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    UNUSED(group_id);

    if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_NO_CYCLE_DSET_NAME)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_NO_CYCLE_LINK_NAME1)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_NO_CYCLE_LINK_NAME2)) {
        if (H5L_TYPE_SOFT != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME3 "/" LINK_VISIT_TEST_NO_CYCLE_DSET_NAME)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME3 "/" LINK_VISIT_TEST_NO_CYCLE_LINK_NAME3)) {
        if (H5L_TYPE_EXTERNAL != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME3 "/" LINK_VISIT_TEST_NO_CYCLE_LINK_NAME4)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME2)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_NO_CYCLE_SUBGROUP_NAME3)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else {
        H5_FAILED();
        printf("    link name didn't match known names\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

static herr_t
link_visit_callback2(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    UNUSED(group_id);

    if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_CYCLE_LINK_NAME1)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2 "/" LINK_VISIT_TEST_CYCLE_LINK_NAME2)) {
        if (H5L_TYPE_SOFT != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME3 "/" LINK_VISIT_TEST_CYCLE_LINK_NAME3)) {
        if (H5L_TYPE_EXTERNAL != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME3 "/" LINK_VISIT_TEST_CYCLE_LINK_NAME4)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME2)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else if (!strcmp(name, LINK_VISIT_TEST_CYCLE_SUBGROUP_NAME3)) {
        if (H5L_TYPE_HARD != info->type) {
            H5_FAILED();
            printf("    link type did not match\n");
            goto error;
        }
    }
    else {
        H5_FAILED();
        printf("    link name didn't match known names\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

static herr_t
link_visit_callback3(hid_t group_id, const char *name, const H5L_info_t *info, void *op_data)
{
    UNUSED(group_id);
    UNUSED(name);
    UNUSED(info);
    UNUSED(op_data);

    return 0;
}

int
vol_link_test(void)
{
    size_t i;
    int    nerrors = 0;

    printf("**********************************************\n");
    printf("*                                            *\n");
    printf("*              VOL Link Tests                *\n");
    printf("*                                            *\n");
    printf("**********************************************\n\n");

    for (i = 0; i < ARRAY_LENGTH(link_tests); i++) {
        nerrors += (*link_tests[i])() ? 1 : 0;
    }

    printf("\n");

done:
    return nerrors;
}
