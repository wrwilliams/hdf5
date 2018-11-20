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
static int test_file_property_lists(void);
static int test_unused_file_API_calls(void);

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
        test_file_property_lists,
        test_unused_file_API_calls,
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

    if ((file_id = H5Fcreate(vol_test_filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
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

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving file info\n");
#endif

    if (H5Fget_info2(file_id, &file_info) < 0) {
        H5_FAILED();
        printf("    couldn't get file info\n");
        goto error;
    }

    /*
     * XXX: Perhaps check some file info bits to make sure things are correct.
     */

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
 * A test to ensure that opening a file which doesn't exist will fail.
 */
static int
test_nonexistent_file(void)
{
    hid_t file_id = -1, fapl_id = -1;
    char  test_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("failure for opening non-existent file")

    snprintf(test_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", NONEXISTENT_FILENAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open non-existent file\n");
#endif

    H5E_BEGIN_TRY {
        file_id = H5Fopen(test_filename, H5F_ACC_RDWR, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    non-existent file was opened!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("File open call successfully failed for non-existent file\n");
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
 * A test to check the behavior of the file intent flags.
 *
 * XXX: Needs to cover more cases.
 */
static int
test_get_file_intent(void)
{
    unsigned file_intent;
    hsize_t  space_dims[FILE_INTENT_TEST_DSET_RANK];
    size_t   i;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    dset_id = -1;
    hid_t    dset_dtype = -1;
    hid_t    space_id = -1;
    char     test_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("retrieve file intent")

    snprintf(test_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_INTENT_TEST_FILENAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    /* Test that file intent works correctly for file create */
    if ((file_id = H5Fcreate(test_filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking to make sure H5F_ACC_TRUNC works correctly\n");
#endif

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDWR != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    /* Test that file intent works correctly for file open */
    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDONLY, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking to make sure H5F_ACC_RDONLY works correctly\n");
#endif

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDONLY != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent\n");
        goto error;
    }

    for (i = 0; i < FILE_INTENT_TEST_DSET_RANK; i++)
        space_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(FILE_INTENT_TEST_DSET_RANK, space_dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Checking to make sure we can't create an object when H5F_ACC_RDONLY is specified\n");
#endif

    /* Ensure that no objects can be created when a file is opened in read-only mode */
    /*
     * XXX: A VOL connector may not support dataset operations.
     */
    H5E_BEGIN_TRY {
        dset_id = H5Dcreate2(file_id, FILE_INTENT_TEST_DATASETNAME, dset_dtype, space_id,
                H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (dset_id >= 0) {
        H5_FAILED();
        printf("    read-only file was modified!\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking to make sure H5F_ACC_RDWR works correctly\n");
#endif

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDWR != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to ensure that a file's name can be retrieved.
 */
static int
test_get_file_name(void)
{
    ssize_t  file_name_buf_len = 0;
    hid_t    file_id = -1, fapl_id = -1;
    char    *file_name_buf = NULL;

    TESTING("get file name with H5Fget_name")

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
    puts("Retrieving size of file name\n");
#endif

    /* Retrieve the size of the file name */
    if ((file_name_buf_len = H5Fget_name(file_id, NULL, 0)) < 0)
        TEST_ERROR

    /* Allocate buffer for file name */
    if (NULL == (file_name_buf = (char *) malloc((size_t) file_name_buf_len + 1)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Retrieving file name\n");
#endif

    /* Retrieve the actual file name */
    if (H5Fget_name(file_id, file_name_buf, (size_t) file_name_buf_len + 1) < 0)
        TEST_ERROR

    if (strncmp(file_name_buf, vol_test_filename, file_name_buf_len)) {
        H5_FAILED();
        printf("    file name '%s' didn't match expected name '%s'\n", file_name_buf, vol_test_filename);
        goto error;
    }

    if (file_name_buf) {
        free(file_name_buf);
        file_name_buf = NULL;
    }

    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        if (file_name_buf) free(file_name_buf);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a file can be re-opened with H5Freopen.
 */
static int
test_file_reopen(void)
{
    hid_t file_id = -1, file_id2 = -1, fapl_id = -1;

    TESTING("re-open file with H5Freopen")

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
    puts("Re-opening file\n");
#endif

    if ((file_id2 = H5Freopen(file_id)) < 0) {
        H5_FAILED();
        printf("    couldn't re-open file\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id2) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl_id);
        H5Fclose(file_id);
        H5Fclose(file_id2);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a VOL connector stores and can return a valid copy
 * of a FAPL and FCPL used upon file access and creation time, respectively.
 */
static int
test_file_property_lists(void)
{
    hid_t file_id1 = -1, file_id2 = -1, fapl_id = -1;
    hid_t fcpl_id1 = -1, fcpl_id2 = -1;
    hid_t fapl_id1 = -1, fapl_id2 = -1;
    char  test_filename1[VOL_TEST_FILENAME_MAX_LENGTH], test_filename2[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("file property list operations")

    snprintf(test_filename1, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_PROPERTY_LIST_TEST_FNAME1);
    snprintf(test_filename2, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_PROPERTY_LIST_TEST_FNAME2);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR
#ifdef DAOS_SPECIFIC
    if (H5Pset_all_coll_metadata_ops(fapl_id, true) < 0)
        TEST_ERROR
#endif

    if ((fcpl_id1 = H5Pcreate(H5P_FILE_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create FCPL\n");
        goto error;
    }


    if ((file_id1 = H5Fcreate(test_filename1, H5F_ACC_TRUNC, fcpl_id1, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file\n");
        goto error;
    }

    if ((file_id2 = H5Fcreate(test_filename2, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file\n");
        goto error;
    }

    if (H5Pclose(fcpl_id1) < 0)
        TEST_ERROR

    /* Try to receive copies of the two property lists, one which has the property set and one which does not */
    if ((fcpl_id1 = H5Fget_create_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((fcpl_id2 = H5Fget_create_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Ensure that property list 1 has the property set and property list 2 does not */



    /* Due to the nature of needing to supply a FAPL with the VOL connector having been set on it to the H5Fcreate()
     * call, we cannot exactly test using H5P_DEFAULT as the FAPL for one of the create calls in this test. However,
     * the use of H5Fget_create_plist() will still be used to check that the FAPL is correct after both creating and
     * opening a file.
     */
    if ((fapl_id1 = H5Fget_access_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((fapl_id2 = H5Fget_access_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Now close the property lists and files and see if we can still retrieve copies of
     * the property lists upon opening (instead of creating) a file
     */
    if (H5Pclose(fcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fcpl_id2) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id2) < 0)
        TEST_ERROR
    if (H5Fclose(file_id1) < 0)
        TEST_ERROR
    if (H5Fclose(file_id2) < 0)
        TEST_ERROR

    if ((file_id1 = H5Fopen(test_filename1, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((file_id2 = H5Fopen(test_filename2, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    if ((fcpl_id1 = H5Fget_create_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((fcpl_id2 = H5Fget_create_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((fapl_id1 = H5Fget_access_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((fapl_id2 = H5Fget_access_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* For completeness' sake, check to make sure the VOL connector is set on each of the FAPLs */



    if (H5Pclose(fcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fcpl_id2) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id2) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id) < 0)
        TEST_ERROR
    if (H5Fclose(file_id1) < 0)
        TEST_ERROR
    if (H5Fclose(file_id2) < 0)
        TEST_ERROR

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fcpl_id1);
        H5Pclose(fcpl_id2);
        H5Pclose(fapl_id1);
        H5Pclose(fapl_id2);
        H5Pclose(fapl_id);
        H5Fclose(file_id1);
        H5Fclose(file_id2);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that the various native HDF5-specific API functions
 * will fail with a VOL connector that isn't the native one.
 *
 * XXX: Many of the calls here may not even be valid. We may need to create
 * separate tests for them.
 */
static int
test_unused_file_API_calls(void)
{
    H5AC_cache_config_t  mdc_config = { 0 };
    ssize_t              ssize_err_ret = -1;
    hsize_t              filesize;
    double               mdc_hit_rate;
    herr_t               err_ret = -1;
    size_t               file_image_buf_len = 0;
    hid_t                obj_id;
    void                *file_handle;
    hid_t                file_id = -1, fapl_id = -1;

    TESTING("unused File API calls")

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
    puts("Testing that all of the unused file API calls don't cause application issues\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Fis_accessible(vol_test_filename, fapl_id);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fflush(file_id, H5F_SCOPE_GLOBAL);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ssize_err_ret = H5Fget_obj_count(file_id, H5F_OBJ_DATASET);
    } H5E_END_TRY;

    if (ssize_err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ssize_err_ret = H5Fget_obj_ids(file_id, H5F_OBJ_DATASET, 0, &obj_id);
    } H5E_END_TRY;

    if (ssize_err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fmount(file_id, "/", file_id, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Funmount(file_id, "/");
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fclear_elink_file_cache(file_id);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ssize_err_ret = H5Fget_file_image(file_id, NULL, file_image_buf_len);
    } H5E_END_TRY;

    if (ssize_err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ssize_err_ret = H5Fget_free_sections(file_id, H5FD_MEM_DEFAULT, 0, NULL);
    } H5E_END_TRY;

    if (ssize_err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        ssize_err_ret = H5Fget_freespace(file_id);
    } H5E_END_TRY;

    if (ssize_err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fget_mdc_config(file_id, &mdc_config);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fget_mdc_hit_rate(file_id, &mdc_hit_rate);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fget_mdc_size(file_id, NULL, NULL, NULL, NULL);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fget_filesize(file_id, &filesize);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fget_vfd_handle(file_id, fapl_id, &file_handle);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Freset_mdc_hit_rate_stats(file_id);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Fset_mdc_config(file_id, &mdc_config);
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

int
vol_file_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_LENGTH(file_tests); i++) {
        nerrors += (*file_tests[i])() ? 1 : 0;
    }

done:
    return nerrors;
}
