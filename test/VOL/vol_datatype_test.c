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

#include "vol_datatype_test.h"

static int test_create_committed_datatype(void);
static int test_create_anonymous_committed_datatype(void);
static int test_create_dataset_with_committed_type(void);
static int test_create_attribute_with_committed_type(void);
static int test_delete_committed_type(void);
static int test_datatype_property_lists(void);
static int test_unused_datatype_API_calls(void);

/*
 * The array of datatype tests to be performed.
 */
static int (*datatype_tests[])(void) = {
        test_create_committed_datatype,
        test_create_anonymous_committed_datatype,
        test_create_dataset_with_committed_type,
        test_create_attribute_with_committed_type,
        test_delete_committed_type,
        test_datatype_property_lists,
        test_unused_datatype_API_calls,
};

/*
 * A test to check that a committed datatype can be created.
 */
static int
test_create_committed_datatype(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t container_group = H5I_INVALID_HID;
    hid_t type_id = H5I_INVALID_HID;

    TESTING("creation of committed datatype")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, DATATYPE_CREATE_TEST_TYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
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
        H5Tclose(type_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an anonymous committed datatype
 * can be created.
 */
static int
test_create_anonymous_committed_datatype(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t container_group = H5I_INVALID_HID;
    hid_t type_id = H5I_INVALID_HID;

    TESTING("creation of anonymous committed datatype")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit_anon(container_group, type_id, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit anonymous datatype\n");
        goto error;
    }

    if (H5Olink(type_id, container_group, DATATYPE_CREATE_ANONYMOUS_TYPE_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't link anonymous datatype into file structure\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
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
        H5Tclose(type_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created using
 * a committed datatype.
 */
static int
test_create_dataset_with_committed_type(void)
{
    hsize_t dims[DATASET_CREATE_WITH_DATATYPE_TEST_DATASET_DIMS];
    size_t  i;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID;
    hid_t   dset_id = H5I_INVALID_HID;
    hid_t   type_id = H5I_INVALID_HID;
    hid_t   fspace_id = H5I_INVALID_HID;

    TESTING("dataset creation w/ committed datatype")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, DATASET_CREATE_WITH_DATATYPE_TEST_TYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
        TEST_ERROR
    if (H5Gclose(container_group) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = H5Topen2(container_group, DATASET_CREATE_WITH_DATATYPE_TEST_TYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open committed datatype\n");
        goto error;
    }

    for (i = 0; i < DATATYPE_CREATE_TEST_DATASET_DIMS; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATATYPE_CREATE_TEST_DATASET_DIMS, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_CREATE_WITH_DATATYPE_TEST_DSET_NAME, type_id, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset using variable-length string committed datatype\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(container_group, DATASET_CREATE_WITH_DATATYPE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
        TEST_ERROR
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
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
        H5Tclose(type_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute can be created
 * using a committed datatype.
 */
static int
test_create_attribute_with_committed_type(void)
{
    hsize_t dims[ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   container_group = H5I_INVALID_HID;
    hid_t   attr_id = H5I_INVALID_HID;
    hid_t   type_id = H5I_INVALID_HID;
    hid_t   space_id = H5I_INVALID_HID;

    TESTING("attribute creation w/ committed datatype")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_DTYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
        TEST_ERROR

    if ((type_id = H5Topen2(container_group, ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_DTYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open committed datatype\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_ATTR_NAME, type_id,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_CREATE_WITH_DATATYPE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
        TEST_ERROR
    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
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
        H5Tclose(type_id);
        H5Sclose(space_id);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a committed datatype can
 * be deleted.
 */
static int
test_delete_committed_type(void)
{
    htri_t type_exists;
    hid_t  file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t  container_group = H5I_INVALID_HID;
    hid_t  type_id = H5I_INVALID_HID;

    TESTING("delete committed datatype")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, DATATYPE_DELETE_TEST_DTYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if ((type_exists = H5Lexists(container_group, DATATYPE_DELETE_TEST_DTYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if datatype exists\n");
        goto error;
    }

    if (!type_exists) {
        H5_FAILED();
        printf("    datatype didn't exist\n");
        goto error;
    }

    if (H5Ldelete(container_group, DATATYPE_DELETE_TEST_DTYPE_NAME, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't delete datatype\n");
        goto error;
    }

    if ((type_exists = H5Lexists(container_group, DATATYPE_DELETE_TEST_DTYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if datatype exists\n");
        goto error;
    }

    if (type_exists) {
        H5_FAILED();
        printf("    datatype exists\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
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
        H5Tclose(type_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a VOL connector stores and can
 * retrieve a valid copy of a TCPL used during committed
 * datatype creation time.
 */
static int
test_datatype_property_lists(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t container_group = H5I_INVALID_HID, group_id = H5I_INVALID_HID;
    hid_t type_id1 = H5I_INVALID_HID, type_id2 = H5I_INVALID_HID;
    hid_t tcpl_id1 = H5I_INVALID_HID, tcpl_id2 = H5I_INVALID_HID;

    TESTING("datatype property list operations")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((container_group = H5Gopen2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATATYPE_PROPERTY_LIST_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    if ((type_id1 = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if ((type_id2 = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if ((tcpl_id1 = H5Pcreate(H5P_DATATYPE_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create TCPL\n");
        goto error;
    }

    /* Currently no TCPL routines are defined */

    if (H5Tcommit2(group_id, DATATYPE_PROPERTY_LIST_TEST_DATATYPE_NAME1, type_id1, H5P_DEFAULT, tcpl_id1, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Tcommit2(group_id, DATATYPE_PROPERTY_LIST_TEST_DATATYPE_NAME2, type_id2, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0 ) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Pclose(tcpl_id1) < 0)
        TEST_ERROR

    /* Try to receive copies for the two property lists */
    if ((tcpl_id1 = H5Tget_create_plist(type_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((tcpl_id2 = H5Tget_create_plist(type_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Now close the property lists and datatypes and see if we can still retieve copies of
     * the property lists upon opening (instead of creating) a datatype
     */
    if (H5Pclose(tcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(tcpl_id2) < 0)
        TEST_ERROR
    if (H5Tclose(type_id1) < 0)
        TEST_ERROR
    if (H5Tclose(type_id2) < 0)
        TEST_ERROR

    if ((type_id1 = H5Topen2(group_id, DATATYPE_PROPERTY_LIST_TEST_DATATYPE_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open datatype\n");
        goto error;
    }

    if ((type_id2 = H5Topen2(group_id, DATATYPE_PROPERTY_LIST_TEST_DATATYPE_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open datatype\n");
        goto error;
    }

    if ((tcpl_id1 = H5Tget_create_plist(type_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((tcpl_id2 = H5Tget_create_plist(type_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if (H5Pclose(tcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(tcpl_id2) < 0)
        TEST_ERROR
    if (H5Tclose(type_id1) < 0)
        TEST_ERROR
    if (H5Tclose(type_id2) < 0)
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
        H5Pclose(tcpl_id1);
        H5Pclose(tcpl_id2);
        H5Tclose(type_id1);
        H5Tclose(type_id2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_unused_datatype_API_calls(void)
{
    TESTING("unused datatype API calls")

    /* None currently that aren't planned to be used */

    SKIPPED();

    return 0;
}

int
vol_datatype_test(void)
{
    size_t i;
    int    nerrors = 0;

    printf("**********************************************\n");
    printf("*                                            *\n");
    printf("*             VOL Datatype Tests             *\n");
    printf("*                                            *\n");
    printf("**********************************************\n\n");

    for (i = 0; i < ARRAY_LENGTH(datatype_tests); i++) {
        nerrors += (*datatype_tests[i])() ? 1 : 0;
    }

    printf("\n");

done:
    return nerrors;
}
