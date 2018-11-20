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

#include "vol_group_test.h"

static int test_create_group_invalid_loc_id(void);
static int test_create_group_under_root(void);
static int test_create_group_under_existing_group(void);
static int test_create_anonymous_group(void);
static int test_get_group_info(void);
static int test_nonexistent_group(void);
static int test_group_property_lists(void);
static int test_unused_group_API_calls(void);

/*
 * The array of group tests to be performed.
 */
static int (*group_tests[])(void) = {
        test_create_group_invalid_loc_id,
        test_create_group_under_root,
        test_create_group_under_existing_group,
        test_create_anonymous_group,
        test_get_group_info,
        test_nonexistent_group,
        test_group_property_lists,
        test_unused_group_API_calls,
};

/*
 * A test to check that a group can't be created when the loc_id
 * is invalid.
 */
static int
test_create_group_invalid_loc_id(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t group_id = -1;

    TESTING("create group with invalid loc_id")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

#ifdef VOL_TEST_DEBUG
    puts("Trying to create a group with an invalid loc_id\n");
#endif

    H5E_BEGIN_TRY {
        group_id = H5Gcreate2(file_id, GROUP_CREATE_INVALID_LOC_ID_GNAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (group_id >= 0) {
        H5_FAILED();
        printf("    created group in invalid loc_id!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Group create call successfully failed with invalid loc_id\n");
#endif

    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a group can be created under the root group.
 */
static int
test_create_group_under_root(void)
{
    hid_t file_id = -1, group_id = -1, fapl_id = -1;

    TESTING("create group under root group")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating group under root group\n");
#endif

    /* Create the group under the root group of the file */
    if ((group_id = H5Gcreate2(file_id, GROUP_CREATE_UNDER_ROOT_GNAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
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
 * A test to check that a group can be created under an existing
 * group which is not the root group.
 */
static int
test_create_group_under_existing_group(void)
{
    hid_t file_id = -1;
    hid_t parent_group_id = -1, new_group_id = -1;
    hid_t fapl_id = -1;

    TESTING("create group under existing group using relative path")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    /* Open the already-existing parent group in the file */
    if ((parent_group_id = H5Gopen2(file_id, GROUP_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating group under non-root group\n");
#endif

    /* Create a new Group under the already-existing parent Group using a relative path */
    if ((new_group_id = H5Gcreate2(parent_group_id, GROUP_CREATE_UNDER_GROUP_REL_GNAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group using relative path\n");
        goto error;
    }

    if (H5Gclose(parent_group_id) < 0)
        TEST_ERROR
    if (H5Gclose(new_group_id) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Gclose(new_group_id);
        H5Gclose(parent_group_id);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a group can be created with H5Gcreate_anon
 */
static int
test_create_anonymous_group(void)
{
    hid_t file_id = -1;
    hid_t container_group = -1, new_group_id = -1;
    hid_t fapl_id = -1;

    TESTING("create anonymous group")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, GROUP_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating anonymous group\n");
#endif

    if ((new_group_id = H5Gcreate_anon(file_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create anonymous group\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Linking anonymous group into file structure\n");
#endif

    if (H5Olink(new_group_id, container_group, GROUP_CREATE_ANONYMOUS_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't link anonymous group into file structure\n");
        goto error;
    }

    if (H5Gclose(new_group_id) < 0)
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
        H5Gclose(new_group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test for the functionality of H5Gget_info.
 */
static int
test_get_group_info(void)
{
    H5G_info_t group_info;
    herr_t     err_ret = -1;
    hid_t      file_id = -1, fapl_id = -1;

    TESTING("retrieve group info")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving group info with H5Gget_info\n");
#endif

    if (H5Gget_info(file_id, &group_info) < 0) {
        H5_FAILED();
        printf("    couldn't get group info\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving group info with H5Gget_info_by_name\n");
#endif

    if (H5Gget_info_by_name(file_id, "/", &group_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get group info by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving group info with H5Gget_info_by_idx\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Gget_info_by_idx(file_id, "/", H5_INDEX_NAME, H5_ITER_INC, 0, &group_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
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

static int
test_nonexistent_group(void)
{
    hid_t file_id = -1, group_id = -1, fapl_id = -1;

    TESTING("failure for opening nonexistent group")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open a non-existent group\n");
#endif

    H5E_BEGIN_TRY {
        group_id = H5Gopen2(file_id, NONEXISTENT_GROUP_TEST_GNAME, H5P_DEFAULT);
    } H5E_END_TRY;

    if (group_id >= 0) {
        H5_FAILED();
        printf("    opened non-existent group!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Group open call successfully failed for non-existent group\n");
#endif

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

/*
 * A test to check that a VOL connector stores and can retrieve a valid
 * copy of a GCPL used at group creation time.
 */
static int
test_group_property_lists(void)
{
    size_t dummy_prop_val = GROUP_PROPERTY_LIST_TEST_DUMMY_VAL;
    hid_t  file_id = -1, fapl_id = -1;
    hid_t  container_group = -1;
    hid_t  group_id1 = -1, group_id2 = -1;
    hid_t  gcpl_id1 = -1, gcpl_id2 = -1;

    TESTING("group property list operations")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, GROUP_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((gcpl_id1 = H5Pcreate(H5P_GROUP_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create GCPL\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Setting property on GCPL\n");
#endif

    if (H5Pset_local_heap_size_hint(gcpl_id1, dummy_prop_val) < 0) {
        H5_FAILED();
        printf("    couldn't set property on GCPL\n");
        goto error;
    }

    /* Create the group in the file */
    if ((group_id1 = H5Gcreate2(container_group, GROUP_PROPERTY_LIST_TEST_GROUP_NAME1, H5P_DEFAULT, gcpl_id1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    /* Create the second group using H5P_DEFAULT for the GCPL */
    if ((group_id2 = H5Gcreate2(container_group, GROUP_PROPERTY_LIST_TEST_GROUP_NAME2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    if (H5Pclose(gcpl_id1) < 0)
        TEST_ERROR

    /* Try to retrieve copies of the two property lists, one which has the property set and one which does not */
    if ((gcpl_id1 = H5Gget_create_plist(group_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((gcpl_id2 = H5Gget_create_plist(group_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Ensure that property list 1 has the property set and property list 2 does not */
    dummy_prop_val = 0;

    if (H5Pget_local_heap_size_hint(gcpl_id1, &dummy_prop_val) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve GCPL property value\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking that property value is retrieved correctly\n");
#endif

    if (dummy_prop_val != GROUP_PROPERTY_LIST_TEST_DUMMY_VAL) {
        H5_FAILED();
        printf("    GCPL property value was incorrect\n");
        goto error;
    }

    dummy_prop_val = 0;

    if (H5Pget_local_heap_size_hint(gcpl_id2, &dummy_prop_val) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve GCPL property value\n");
        goto error;
    }

    if (dummy_prop_val == GROUP_PROPERTY_LIST_TEST_DUMMY_VAL) {
        H5_FAILED();
        printf("    GCPL property value was set!\n");
        goto error;
    }

    /* Now close the property lists and groups and see if we can still retrieve copies of
     * the property lists upon opening (instead of creating) a group
     */
    if (H5Pclose(gcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(gcpl_id2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id1) < 0)
        TEST_ERROR
    if (H5Gclose(group_id2) < 0)
        TEST_ERROR

    if ((group_id1 = H5Gopen2(container_group, GROUP_PROPERTY_LIST_TEST_GROUP_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group\n");
        goto error;
    }

    if ((group_id2 = H5Gopen2(container_group, GROUP_PROPERTY_LIST_TEST_GROUP_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group\n");
        goto error;
    }

    if ((gcpl_id1 = H5Gget_create_plist(group_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((gcpl_id2 = H5Gget_create_plist(group_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if (H5Pclose(gcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(gcpl_id2) < 0)
        TEST_ERROR
    if (H5Gclose(group_id1) < 0)
        TEST_ERROR
    if (H5Gclose(group_id2) < 0)
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
        H5Pclose(gcpl_id1);
        H5Pclose(gcpl_id2);
        H5Gclose(group_id1);
        H5Gclose(group_id2);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_unused_group_API_calls(void)
{
    TESTING("unused group API calls")

    /* None currently that aren't planned to be used */
#ifdef VOL_TEST_DEBUG
    puts("Currently no APIs to test here\n");
#endif

    SKIPPED();

    return 0;
}

int
vol_group_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_LENGTH(group_tests); i++) {
        nerrors += (*group_tests[i])() ? 1 : 0;
    }

done:
    return nerrors;
}
