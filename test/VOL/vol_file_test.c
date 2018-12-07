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
static int test_create_file_invalid_params(void);
static int test_create_file_excl(void);
static int test_open_file(void);
static int test_open_file_invalid_params(void);
static int test_open_nonexistent_file(void);
static int test_reopen_file(void);
static int test_close_file_invalid_id(void);
static int test_flush_file(void);
static int test_file_is_accessible(void);
static int test_file_property_lists(void);
static int test_get_file_intent(void);
static int test_get_file_obj_count(void);
static int test_get_file_obj_ids(void);
static int test_get_file_vfd_handle(void);
static int test_file_mounts(void);
static int test_get_file_freespace(void);
static int test_get_file_size(void);
static int test_get_file_image(void);
static int test_get_file_name(void);
static int test_get_file_info(void);

/*
 * The array of file tests to be performed.
 */
static int (*file_tests[])(void) = {
        test_create_file,
        test_create_file_invalid_params,
        test_create_file_excl,
        test_open_file,
        test_open_file_invalid_params,
        test_open_nonexistent_file,
        test_reopen_file,
        test_close_file_invalid_id,
        test_flush_file,
        test_file_is_accessible,
        test_file_property_lists,
        test_get_file_intent,
        test_get_file_obj_count,
        test_get_file_obj_ids,
        test_get_file_vfd_handle,
        test_file_mounts,
        test_get_file_freespace,
        test_get_file_size,
        test_get_file_image,
        test_get_file_name,
        test_get_file_info,
};

/*
 * Tests that a file can be created with the VOL connector.
 */
static int
test_create_file(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fcreate");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fcreate(FILE_CREATE_TEST_FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file '%s'\n", FILE_CREATE_TEST_FILENAME);
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

/*
 * Tests that a file can't be created when H5Fcreate is passed
 * invalid parameters.
 */
static int
test_create_file_invalid_params(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fcreate with invalid parameters"); puts("");

    TESTING_2("H5Fcreate with invalid file name");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(NULL, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with an invalid name!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fcreate("", H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with an invalid name!\n");
        goto error;
    }

    PASSED();

    TESTING_2("H5Fcreate with invalid flags");

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_INVALID_PARAMS_FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with invalid flag H5F_ACC_RDONLY!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_INVALID_PARAMS_FILE_NAME, H5F_ACC_RDWR, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with invalid flag H5F_ACC_RDWR!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_INVALID_PARAMS_FILE_NAME, H5F_ACC_CREAT, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with invalid flag H5F_ACC_CREAT!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_INVALID_PARAMS_FILE_NAME, H5F_ACC_SWMR_READ, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with invalid flag H5F_ACC_SWMR_READ!\n");
        goto error;
    }

    PASSED();

    TESTING_2("H5Fcreate with invalid FCPL");

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_INVALID_PARAMS_FILE_NAME, H5F_ACC_TRUNC, H5I_INVALID_HID, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was created with invalid FCPL!\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
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
 * Tests that file creation will fail when a file is created
 * using the H5F_ACC_EXCL flag while the file already exists.
 */
static int
test_create_file_excl(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fcreate with H5F_ACC_EXCL flag");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fcreate(FILE_CREATE_EXCL_FILE_NAME, H5F_ACC_EXCL, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create first file\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        file_id = H5Fcreate(FILE_CREATE_EXCL_FILE_NAME, H5F_ACC_EXCL, H5P_DEFAULT, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    created already existing file using H5F_ACC_EXCL flag!\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
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
 * Tests that a file can be opened with the VOL connector.
 */
static int
test_open_file(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fopen");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDONLY, fapl_id)) < 0) {
        H5_FAILED();
        printf("    unable to open file '%s' in read-only mode\n", vol_test_filename);
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    unable to open file '%s' in read-write mode\n", vol_test_filename);
        goto error;
    }

    /*
     * XXX: SWMR open flags
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
 * Tests that a file can't be opened when H5Fopen is given
 * invalid parameters.
 */
static int
test_open_file_invalid_params(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fopen with invalid parameters"); puts("");

    TESTING_2("H5Fopen with invalid file name");

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        file_id = H5Fopen(NULL, H5F_ACC_RDWR, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was opened with an invalid name!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fopen("", H5F_ACC_RDWR, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was opened with an invalid name!\n");
        goto error;
    }

    PASSED();

    TESTING_2("H5Fopen with invalid flags");

    H5E_BEGIN_TRY {
        file_id = H5Fopen(vol_test_filename, H5F_ACC_TRUNC, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was opened with invalid flag H5F_ACC_TRUNC!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        file_id = H5Fopen(vol_test_filename, H5F_ACC_EXCL, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    file was opened with invalid flag H5F_ACC_EXCL!\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
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
test_open_nonexistent_file(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    char  test_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("for failure when opening a non-existent file")

    snprintf(test_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", NONEXISTENT_FILENAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        file_id = H5Fopen(test_filename, H5F_ACC_RDWR, fapl_id);
    } H5E_END_TRY;

    if (file_id >= 0) {
        H5_FAILED();
        printf("    non-existent file was opened!\n");
        goto error;
    }

    if (H5Pclose(fapl_id) < 0)
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
 * A test to check that a file can be re-opened with H5Freopen.
 */
static int
test_reopen_file(void)
{
    hid_t file_id = H5I_INVALID_HID, file_id2 = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("re-open of a file with H5Freopen")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

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
 * A test to check that H5Fclose doesn't succeed for an
 * invalid file ID */
static int
test_close_file_invalid_id(void)
{
    herr_t err_ret = -1;
    hid_t  fapl_id = H5I_INVALID_HID;

    TESTING("H5Fclose with an invalid ID")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        err_ret = H5Fclose(H5I_INVALID_HID);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    closed an invalid file ID!\n");
        goto error;
    }

    PASSED();

    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a file can be flushed using H5Fflush.
 */
static int
test_flush_file(void)
{
    hid_t file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("H5Fflush")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    unable to open file '%s'\n", vol_test_filename);
        goto error;
    }

    /*
     * XXX: Nothing really to flush here.
     */
    if (H5Fflush(file_id, H5F_SCOPE_LOCAL) < 0) {
        H5_FAILED();
        printf("    unable to flush file with scope H5F_SCOPE_LOCAL\n");
        goto error;
    }

    if (H5Fflush(file_id, H5F_SCOPE_GLOBAL) < 0) {
        H5_FAILED();
        printf("    unable to flush file with scope H5F_SCOPE_GLOBAL\n");
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

/*
 * A test for H5Fis_accessible.
 */
static int
test_file_is_accessible(void)
{
    htri_t is_accessible;
    hid_t  fapl_id = H5I_INVALID_HID;

    TESTING("H5Fis_accessible")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((is_accessible = H5Fis_accessible(vol_test_filename, fapl_id)) < 0) {
        H5_FAILED();
        printf("    file '%s' is not accessible with VOL connector\n", vol_test_filename);
        goto error;
    }

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
 * A test to check that a VOL connector stores and can return a valid copy
 * of a FAPL and FCPL used upon file access and creation time, respectively.
 */
static int
test_file_property_lists(void)
{
    hsize_t prop_val = 0;
    hid_t   file_id1 = H5I_INVALID_HID, file_id2 = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    hid_t   fcpl_id1 = H5I_INVALID_HID, fcpl_id2 = H5I_INVALID_HID;
    hid_t   fapl_id1 = H5I_INVALID_HID, fapl_id2 = H5I_INVALID_HID;
    char    test_filename1[VOL_TEST_FILENAME_MAX_LENGTH], test_filename2[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("file property list operations")

    snprintf(test_filename1, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_PROPERTY_LIST_TEST_FNAME1);
    snprintf(test_filename2, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_PROPERTY_LIST_TEST_FNAME2);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((fcpl_id1 = H5Pcreate(H5P_FILE_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create FCPL\n");
        goto error;
    }

    if (H5Pset_userblock(fcpl_id1, FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL) < 0) {
        H5_FAILED();
        printf("    failed to set test property on FCPL\n");
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
        printf("    couldn't get FCPL\n");
        goto error;
    }

    if ((fcpl_id2 = H5Fget_create_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get FCPL\n");
        goto error;
    }

    /* Ensure that property list 1 has the property set and property list 2 does not */
    if (H5Pget_userblock(fcpl_id1, &prop_val) < 0) {
        H5_FAILED();
        printf("    failed to retrieve test property from FCPL\n");
        goto error;
    }

    if (prop_val != FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL) {
        H5_FAILED();
        printf("    retrieved test property value '%llu' did not match expected value '%llu'\n",
                (long long unsigned) prop_val, (long long unsigned) FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL);
        goto error;
    }

    if (H5Pget_userblock(fcpl_id2, &prop_val) < 0) {
        H5_FAILED();
        printf("    failed to retrieve test property from FCPL\n");
        goto error;
    }

    if (prop_val == FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL) {
        printf("    retrieved test property value '%llu' matched control value '%llu' when it shouldn't have\n",
                        (long long unsigned) prop_val, (long long unsigned) FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL);
        goto error;
    }

    if (H5Pclose(fcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fcpl_id2) < 0)
        TEST_ERROR


    /* Due to the nature of needing to supply a FAPL with the VOL connector having been set on it to the H5Fcreate()
     * call, we cannot exactly test using H5P_DEFAULT as the FAPL for one of the create calls in this test. However,
     * the use of H5Fget_access_plist() will still be used to check that the FAPL is correct after both creating and
     * opening a file.
     */
    if ((fapl_id1 = H5Fget_access_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get FAPL\n");
        goto error;
    }

    if ((fapl_id2 = H5Fget_access_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get FAPL\n");
        goto error;
    }

    if (H5Pclose(fapl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(fapl_id2) < 0)
        TEST_ERROR

    /* Now see if we can still retrieve copies of the property lists upon opening
     * (instead of creating) a file. If they were reconstructed properly upon file
     * open, the creation property lists should also have the same test values
     * as set before.
     */
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
        printf("    couldn't get FCPL\n");
        goto error;
    }

    if ((fcpl_id2 = H5Fget_create_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get FCPL\n");
        goto error;
    }

    /* Check the values of the test property */
    if (H5Pget_userblock(fcpl_id1, &prop_val) < 0) {
        H5_FAILED();
        printf("    failed to retrieve test property from FCPL\n");
        goto error;
    }

    if (prop_val != FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL) {
        H5_FAILED();
        printf("    retrieved test property value '%llu' did not match expected value '%llu'\n",
                (long long unsigned) prop_val, (long long unsigned) FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL);
        goto error;
    }

    if (H5Pget_userblock(fcpl_id2, &prop_val) < 0) {
        H5_FAILED();
        printf("    failed to retrieve test property from FCPL\n");
        goto error;
    }

    if (prop_val == FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL) {
        printf("    retrieved test property value '%llu' matched control value '%llu' when it shouldn't have\n",
                (long long unsigned) prop_val, (long long unsigned) FILE_PROPERTY_LIST_TEST_FCPL_PROP_VAL);
        goto error;
    }

    if ((fapl_id1 = H5Fget_access_plist(file_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get FAPL\n");
        goto error;
    }

    if ((fapl_id2 = H5Fget_access_plist(file_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get FAPL\n");
        goto error;
    }

    /* XXX: For completeness' sake, check to make sure the VOL connector is set on each of the FAPLs */



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
 * A test to check that the file intent flags can be retrieved.
 */
static int
test_get_file_intent(void)
{
    unsigned file_intent;
    hid_t    file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    char     test_filename[VOL_TEST_FILENAME_MAX_LENGTH];

    TESTING("retrieval of file intent with H5Fget_intent")

    snprintf(test_filename, VOL_TEST_FILENAME_MAX_LENGTH, "%s", FILE_INTENT_TEST_FILENAME);

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    /* Test that file intent retrieval works correctly for file create */
    if ((file_id = H5Fcreate(test_filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't create file '%s'\n", test_filename);
        goto error;
    }

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDWR != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent for file creation\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    /* Test that file intent retrieval works correctly for file open */
    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDONLY, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file '%s'\n", vol_test_filename);
        goto error;
    }

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDONLY != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent for read-only file open\n");
        goto error;
    }

    if (H5Fclose(file_id) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file '%s'\n", vol_test_filename);
        goto error;
    }

    if (H5Fget_intent(file_id, &file_intent) < 0)
        TEST_ERROR

    if (H5F_ACC_RDWR != file_intent) {
        H5_FAILED();
        printf("    received incorrect file intent\n");
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

/*
 * A test to check that the number of open objects in a file
 * can be retrieved.
 */
static int
test_get_file_obj_count(void)
{
    ssize_t obj_count;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("retrieval of number of objects in file with H5Fget_obj_count")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file '%s'\n", vol_test_filename);
        goto error;
    }

    if ((obj_count = H5Fget_obj_count(file_id, H5F_OBJ_ALL)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve number of objects in file '%s'\n", vol_test_filename);
        goto error;
    }

    if (obj_count != 1) {
        H5_FAILED();
        printf("    incorrect object count\n");
        goto error;
    }

    /* Retrieve object count for all open HDF5 files */
    if ((obj_count = H5Fget_obj_count(H5F_OBJ_ALL, H5F_OBJ_ALL)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve number of objects in file '%s'\n", vol_test_filename);
        goto error;
    }

    if (obj_count != 1) {
        H5_FAILED();
        printf("    incorrect object count\n");
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

/*
 * A test to check that the IDs of the open objects in a file
 * can be retrieved.
 */
static int
test_get_file_obj_ids(void)
{
    TESTING("retrieval of open file object IDs")

    /*
     * XXX
     */
    SKIPPED();

    return 0;
}

/*
 * A test to check that the VFD handle can be retrieved using
 * the native VOL connector.
 */
static int
test_get_file_vfd_handle(void)
{
    TESTING("retrieval of VFD handle")

    /*
     * XXX
     */
    SKIPPED();

    return 0;
}

/*
 * A test to check that file mounting and unmounting works
 * correctly.
 */
static int
test_file_mounts(void)
{
    TESTING("file mounting/unmounting")

    /*
     * XXX
     */
    SKIPPED();

    return 0;
}

/*
 * A test for H5Fget_freespace.
 */
static int
test_get_file_freespace(void)
{
    hssize_t free_space;
    hid_t    file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("retrieval of file free space")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file '%s'\n", vol_test_filename);
        goto error;
    }

    if ((free_space = H5Fget_freespace(file_id)) < 0) {
        H5_FAILED();
        printf("    unable to get file freespace\n");
        goto error;
    }

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
 * A test for H5Fget_filesize.
 */
static int
test_get_file_size(void)
{
    hsize_t file_size;
    hid_t   file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("retrieval of file size")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file '%s'\n", vol_test_filename);
        goto error;
    }

    if (H5Fget_filesize(file_id, &file_size) < 0) {
        H5_FAILED();
        printf("    unable to get file size\n");
        goto error;
    }

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
 * A test for H5Fget_file_image.
 */
static int
test_get_file_image(void)
{
    TESTING("retrieval of file image")

    /*
     * XXX
     */
    SKIPPED();

    return 0;
}

/*
 * A test to ensure that a file's name can be retrieved.
 */
static int
test_get_file_name(void)
{
    ssize_t  file_name_buf_len = 0;
    hid_t    file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;
    char    *file_name_buf = NULL;

    TESTING("retrieval of file name")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

    /* Retrieve the size of the file name */
    if ((file_name_buf_len = H5Fget_name(file_id, NULL, 0)) < 0)
        TEST_ERROR

    /* Allocate buffer for file name */
    if (NULL == (file_name_buf = (char *) malloc((size_t) file_name_buf_len + 1)))
        TEST_ERROR

    /* Retrieve the actual file name */
    if (H5Fget_name(file_id, file_name_buf, (size_t) file_name_buf_len + 1) < 0)
        TEST_ERROR

    if (strncmp(file_name_buf, vol_test_filename, (size_t) file_name_buf_len)) {
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
 * A test for H5Fget_info.
 */
static int
test_get_file_info(void)
{
    H5F_info2_t file_info;
    hid_t       file_id = H5I_INVALID_HID, fapl_id = H5I_INVALID_HID;

    TESTING("retrieval of file info with H5Fget_info")

    if ((fapl_id = h5_fileaccess()) < 0)
        TEST_ERROR

    if ((file_id = H5Fopen(vol_test_filename, H5F_ACC_RDWR, fapl_id)) < 0) {
        H5_FAILED();
        printf("    couldn't open file\n");
        goto error;
    }

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

int
vol_file_test(void)
{
    size_t i;
    int    nerrors = 0;

    printf("**********************************************\n");
    printf("*                                            *\n");
    printf("*               VOL File Tests               *\n");
    printf("*                                            *\n");
    printf("**********************************************\n\n");

    for (i = 0; i < ARRAY_LENGTH(file_tests); i++) {
        nerrors += (*file_tests[i])() ? 1 : 0;
    }

    printf("\n");

done:
    return nerrors;
}
