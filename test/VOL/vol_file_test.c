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

#include "vol_file_test.h"

static int test_create_file(void);
static int test_get_file_info(void);
static int test_nonexistent_file(void);
static int test_get_file_intent(void);
static int test_get_file_name(void);
static int test_file_reopen(void);
static int test_unused_file_API_calls(void);
static int test_file_property_lists(void);

/*
 * The array of file tests to be performed.
 */
static int (*file_tests[])(void) = {
        test_create_file,
        test_get_file_info,
        test_nonexistent_file,
        test_get_file_intent,
        test_get_file_name,
        test_file_reopen,
        test_unused_file_API_calls,
        test_file_property_lists,
        NULL
};

/*
 * Test that a file can be created.
 */
static int
test_create_file(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t group_id = -1;

    TESTING("create file");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file\n");
        goto error;
    }

    /* Setup container groups for the different classes of tests */
#ifdef VOL_TEST_DEBUG
    puts("Setting up test file container groups\n");
#endif

    if ((group_id = H5Gcreate2(file_id, GROUP_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for group tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for attribute tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for dataset tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for datatype tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for link tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for object tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    if ((group_id = H5Gcreate2(file_id, MISCELLANEOUS_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group for miscellaneous tests\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(group_id);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test for H5Fget_info.
 */
static int
test_get_file_info(void)
{
    H5F_info2_t file_info;
    hid_t       file_id = -1, fapl_id = -1;

    TESTING("retrieve file info")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_DEBUG
    puts("Retrieving file info\n");
#endif

    if (H5Fget_info2(file_id, &file_info) < 0) {
        H5_FAILED();
        printf("    couldn't get file info\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

int
vol_file_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_SIZE(file_tests); i++) {
        nerrors += (*file_tests[i]()) ? 1 : 0;
    }

done:
    return nerrors;
}
