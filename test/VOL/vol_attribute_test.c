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

#include "vol_attribute_test.h"

static int test_create_attribute_on_root(void);
static int test_create_attribute_on_dataset(void);
static int test_create_attribute_on_datatype(void);
static int test_create_attribute_with_null_space(void);
static int test_create_attribute_with_scalar_space(void);
static int test_get_attribute_info(void);
static int test_get_attribute_space_and_type(void);
static int test_get_attribute_name(void);
static int test_create_attribute_with_space_in_name(void);
static int test_delete_attribute(void);
static int test_write_attribute(void);
static int test_read_attribute(void);
static int test_rename_attribute(void);
static int test_get_number_attributes(void);
static int test_attribute_iterate(void);
static int test_attribute_iterate_0_attributes(void);
static int test_unused_attribute_API_calls(void);
static int test_attribute_property_lists(void);

static herr_t attr_iter_callback1(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data);
static herr_t attr_iter_callback2(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data);

/*
 * The array of attribute tests to be performed.
 */
static int (*attribute_tests[])(void) = {
        test_create_attribute_on_root,
        test_create_attribute_on_dataset,
        test_create_attribute_on_datatype,
        test_create_attribute_with_null_space,
        test_create_attribute_with_scalar_space,
        test_get_attribute_info,
        test_get_attribute_space_and_type,
        test_get_attribute_name,
        test_create_attribute_with_space_in_name,
        test_delete_attribute,
        test_write_attribute,
        test_read_attribute,
        test_rename_attribute,
        test_get_number_attributes,
        test_attribute_iterate,
        test_attribute_iterate_0_attributes,
        test_unused_attribute_API_calls,
        test_attribute_property_lists,
};

/*
 * A test to check that an attribute can be created on
 * the root group.
 */
static int
test_create_attribute_on_root(void)
{
    hsize_t dims[ATTRIBUTE_CREATE_ON_ROOT_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   attr_id = -1, attr_id2 = -1;
    hid_t   attr_dtype1 = -1, attr_dtype2 = -1;
    hid_t   space_id = -1;

    TESTING("create, open and close attribute on root group")

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

    for (i = 0; i < ATTRIBUTE_CREATE_ON_ROOT_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_CREATE_ON_ROOT_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype1 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype2 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on root group with H5Acreate2\n");
#endif

    if ((attr_id = H5Acreate2(file_id, ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME, attr_dtype1, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on root group with H5Acreate_by_name\n");
#endif

    if ((attr_id2 = H5Acreate_by_name(file_id, "/", ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME2, attr_dtype2, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute on object by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the attributes exist\n");
#endif

    /* Verify the attributes have been created */
    if ((attr_exists = H5Aexists(file_id, ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(file_id, ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME2)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists_by_name(file_id, "/", ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists by H5Aexists_by_name\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists_by_name(file_id, "/", ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists by H5Aexists_by_name\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    /* Now close the attributes and verify we can open them */
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen\n");
#endif

    if ((attr_id = H5Aopen(file_id, ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen(file_id, ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_name\n");
#endif

    if ((attr_id = H5Aopen_by_name(file_id, "/", ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen_by_name(file_id, "/", ATTRIBUTE_CREATE_ON_ROOT_ATTR_NAME2, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_idx\n");
#endif

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        attr_id = H5Aopen_by_idx(file_id, "/", H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT, H5P_DEFAULT);
        attr_id2 = H5Aopen_by_idx(file_id, "/", H5_INDEX_NAME, H5_ITER_INC, 1, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (attr_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (attr_id2 >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype1) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype2) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
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
        H5Tclose(attr_dtype1);
        H5Tclose(attr_dtype2);
        H5Aclose(attr_id);
        H5Aclose(attr_id2);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute can be created on
 * a dataset.
 */
static int
test_create_attribute_on_dataset(void)
{
    hsize_t dset_dims[ATTRIBUTE_CREATE_ON_DATASET_DSET_SPACE_RANK];
    hsize_t attr_dims[ATTRIBUTE_CREATE_ON_DATASET_ATTR_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   dset_id = -1;
    hid_t   attr_id = -1, attr_id2 = -1;
    hid_t   attr_dtype1 = -1, attr_dtype2 = -1;
    hid_t   dset_dtype = -1;
    hid_t   dset_space_id = -1;
    hid_t   attr_space_id = -1;

    TESTING("create attribute on dataset")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_CREATE_ON_DATASET_DSET_SPACE_RANK; i++)
        dset_dims[i] = (hsize_t) rand() % MAX_DIM_SIZE + 1;
    for (i = 0; i < ATTRIBUTE_CREATE_ON_DATASET_ATTR_SPACE_RANK; i++)
        attr_dims[i] = (hsize_t) rand() % MAX_DIM_SIZE + 1;

    if ((dset_space_id = H5Screate_simple(ATTRIBUTE_CREATE_ON_DATASET_DSET_SPACE_RANK, dset_dims, NULL)) < 0)
        TEST_ERROR
    if ((attr_space_id = H5Screate_simple(ATTRIBUTE_CREATE_ON_DATASET_ATTR_SPACE_RANK, attr_dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype1 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype2 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME, dset_dtype,
            dset_space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on dataset with H5Acreate2\n");
#endif

    if ((attr_id = H5Acreate2(dset_id, ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME, attr_dtype1,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on dataset with H5Acreate_by_name\n");
#endif

    if ((attr_id2 = H5Acreate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME,
            ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME2, attr_dtype2, attr_space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute on object by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the attributes exist\n");
#endif

    /* Verify the attributes have been created */
    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME2)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    /* Now close the attributes and verify we can open them */
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen\n");
#endif

    if ((attr_id = H5Aopen(dset_id, ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen(dset_id, ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_name\n");
#endif

    if ((attr_id = H5Aopen_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME,
            ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME,
            ATTRIBUTE_CREATE_ON_DATASET_ATTR_NAME2, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_idx\n");
#endif

    if ((attr_id = H5Aopen_by_idx(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME,
            H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by index\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen_by_idx(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATASET_DSET_NAME,
            H5_INDEX_NAME, H5_ITER_INC, 1, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by index\n");
        goto error;
    }

    if (H5Sclose(dset_space_id) < 0)
        TEST_ERROR
    if (H5Sclose(attr_space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype1) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
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
        H5Sclose(dset_space_id);
        H5Sclose(attr_space_id);
        H5Tclose(dset_dtype);
        H5Tclose(attr_dtype1);
        H5Tclose(attr_dtype2);
        H5Dclose(dset_id);
        H5Aclose(attr_id);
        H5Aclose(attr_id2);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute can be created on
 * a committed datatype.
 */
static int
test_create_attribute_on_datatype(void)
{
    hsize_t dims[ATTRIBUTE_CREATE_ON_DATATYPE_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   type_id = -1;
    hid_t   attr_id = -1, attr_id2 = -1;
    hid_t   attr_dtype1 = -1, attr_dtype2 = -1;
    hid_t   space_id = -1;

    TESTING("create attribute on committed datatype")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    {
        /* Temporary workaround for now since H5Tcommit2 doesn't return something public useable
         * for a VOL object */
        if (H5Tclose(type_id) < 0)
            TEST_ERROR

        if ((type_id = H5Topen2(container_group, ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open committed datatype\n");
            goto error;
        }
    }

    for (i = 0; i < ATTRIBUTE_CREATE_ON_DATATYPE_SPACE_RANK; i++)
        dims[i] = (hsize_t) rand() % MAX_DIM_SIZE + 1;

    if ((space_id = H5Screate_simple(ATTRIBUTE_CREATE_ON_DATATYPE_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype1 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype2 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on datatype with H5Acreate2\n");
#endif

    if ((attr_id = H5Acreate2(type_id, ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME, attr_dtype1,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating attribute on datatype with H5Acreate_by_name\n");
#endif

    if ((attr_id2 = H5Acreate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME,
            ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME2, attr_dtype2, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute on datatype by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the attributes exist\n");
#endif

    /* Verify the attributes have been created */
    if ((attr_exists = H5Aexists(type_id, ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(type_id, ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME2)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    /* Now close the attributes and verify we can open them */
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen\n");
#endif

    if ((attr_id = H5Aopen(type_id, ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen(type_id, ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_name\n");
#endif

    if ((attr_id = H5Aopen_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME,
            ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME,
            ATTRIBUTE_CREATE_ON_DATATYPE_ATTR_NAME2, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by name\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to open the attributes with H5Aopen_by_idx\n");
#endif

    if ((attr_id = H5Aopen_by_idx(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME,
            H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by index\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen_by_idx(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_CREATE_ON_DATATYPE_DTYPE_NAME,
            H5_INDEX_NAME, H5_ITER_INC, 1, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute by index\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype1) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype2) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR
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
        H5Sclose(space_id);
        H5Tclose(attr_dtype1);
        H5Tclose(attr_dtype2);
        H5Aclose(attr_id);
        H5Aclose(attr_id2);
        H5Tclose(type_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that creating an attribute with a
 * NULL dataspace is not problematic.
 */
static int
test_create_attribute_with_null_space(void)
{
    htri_t attr_exists;
    hid_t  file_id = -1, fapl_id = -1;
    hid_t  container_group = -1, group_id = -1;
    hid_t  attr_id = -1;
    hid_t  attr_dtype = -1;
    hid_t  space_id = -1;

    TESTING("create attribute with NULL dataspace")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, ATTRIBUTE_CREATE_NULL_DATASPACE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((space_id = H5Screate(H5S_NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    printf("Creating attribute with NULL dataspace\n");
#endif

    if ((attr_id = H5Acreate2(group_id, ATTRIBUTE_CREATE_NULL_DATASPACE_TEST_ATTR_NAME, attr_dtype, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(group_id, ATTRIBUTE_CREATE_NULL_DATASPACE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

    if ((attr_id = H5Aopen(group_id, ATTRIBUTE_CREATE_NULL_DATASPACE_TEST_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
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
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that creating an attribute with a
 * scalar dataspace is not problematic.
 */
static int
test_create_attribute_with_scalar_space(void)
{
    htri_t attr_exists;
    hid_t  file_id = -1, fapl_id = -1;
    hid_t  container_group = -1, group_id = -1;
    hid_t  attr_id = -1;
    hid_t  attr_dtype = -1;
    hid_t  space_id = -1;

    TESTING("create attribute with SCALAR dataspace")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, ATTRIBUTE_CREATE_SCALAR_DATASPACE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((space_id = H5Screate(H5S_SCALAR)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    printf("Creating attribute with SCALAR dataspace\n");
#endif

    if ((attr_id = H5Acreate2(group_id, ATTRIBUTE_CREATE_SCALAR_DATASPACE_TEST_ATTR_NAME, attr_dtype, space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(group_id, ATTRIBUTE_CREATE_SCALAR_DATASPACE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

    if ((attr_id = H5Aopen(group_id, ATTRIBUTE_CREATE_SCALAR_DATASPACE_TEST_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
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
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of H5Aget_info.
 */
static int
test_get_attribute_info(void)
{
    H5A_info_t attr_info;
    hsize_t    dims[ATTRIBUTE_GET_INFO_TEST_SPACE_RANK];
    size_t     i;
    htri_t     attr_exists;
    herr_t     err_ret = -1;
    hid_t      file_id = -1, fapl_id = -1;
    hid_t      container_group = -1;
    hid_t      attr_id = -1;
    hid_t      attr_dtype = -1;
    hid_t      space_id = -1;

    TESTING("retrieve attribute info")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_GET_INFO_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_GET_INFO_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_GET_INFO_TEST_ATTR_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_GET_INFO_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's info with H5Aget_info\n");
#endif

    if (H5Aget_info(attr_id, &attr_info) < 0) {
        H5_FAILED();
        printf("    couldn't get attribute info\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's info with H5Aget_info_by_name\n");
#endif

    if (H5Aget_info_by_name(container_group, ".", ATTRIBUTE_GET_INFO_TEST_ATTR_NAME, &attr_info, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't get attribute info by name\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's info with H5Aget_info_by_idx\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Aget_info_by_idx(container_group, "/", H5_INDEX_NAME, H5_ITER_INC, 0, &attr_info, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that valid copies of an attribute's
 * dataspace and datatype can be retrieved with
 * H5Aget_space and H5Aget_type, respectively.
 */
static int
test_get_attribute_space_and_type(void)
{
    hsize_t attr_dims[ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   attr_id = -1;
    hid_t   attr_dtype = -1;
    hid_t   attr_space_id = -1;
    hid_t   tmp_type_id = -1;
    hid_t   tmp_space_id = -1;

    TESTING("retrieve attribute dataspace and datatype")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK; i++)
        attr_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((attr_space_id = H5Screate_simple(ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK, attr_dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_GET_SPACE_TYPE_TEST_ATTR_NAME, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_GET_SPACE_TYPE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's datatype\n");
#endif

    /* Retrieve the attribute's datatype and dataspace and verify them */
    if ((tmp_type_id = H5Aget_type(attr_id)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute's datatype\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's dataspace\n");
#endif


    if ((tmp_space_id = H5Aget_space(attr_id)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute's dataspace\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking to make sure the attribute's datatype and dataspace match what was provided at creation time\n");
#endif

    {
        hsize_t space_dims[ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK];
        htri_t  types_equal = H5Tequal(tmp_type_id, attr_dtype);

        if (types_equal < 0) {
            H5_FAILED();
            printf("    datatype was invalid\n");
            goto error;
        }

        if (!types_equal) {
            H5_FAILED();
            printf("    attribute's datatype did not match\n");
            goto error;
        }

        if (H5Sget_simple_extent_dims(tmp_space_id, space_dims, NULL) < 0)
            TEST_ERROR

        for (i = 0; i < ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK; i++)
            if (space_dims[i] != attr_dims[i]) {
                H5_FAILED();
                printf("    dataspace dims didn't match\n");
                goto error;
            }
    }

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the previous checks hold true after closing and re-opening the attribute\n");
#endif

    /* Now close the attribute and verify that this still works after opening an
     * attribute instead of creating it
     */
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Tclose(tmp_type_id) < 0)
        TEST_ERROR
    if (H5Sclose(tmp_space_id) < 0)
        TEST_ERROR

    if ((attr_id = H5Aopen(container_group, ATTRIBUTE_GET_SPACE_TYPE_TEST_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((tmp_type_id = H5Aget_type(attr_id)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute's datatype\n");
        goto error;
    }

    if ((tmp_space_id = H5Aget_space(attr_id)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute's dataspace\n");
        goto error;
    }

    {
        hsize_t space_dims[ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK];
        htri_t  types_equal = H5Tequal(tmp_type_id, attr_dtype);

        if (types_equal < 0) {
            H5_FAILED();
            printf("    datatype was invalid\n");
            goto error;
        }

        /*
         * Disabled for now, as there seem to be issues with HDF5 comparing
         * certain datatypes
         */
        if (!types_equal) {
            H5_FAILED();
            printf("    attribute's datatype did not match\n");
            goto error;
        }

        if (H5Sget_simple_extent_dims(tmp_space_id, space_dims, NULL) < 0)
            TEST_ERROR

        for (i = 0; i < ATTRIBUTE_GET_SPACE_TYPE_TEST_SPACE_RANK; i++) {
            if (space_dims[i] != attr_dims[i]) {
                H5_FAILED();
                printf("    dataspace dims didn't match\n");
                goto error;
            }
        }
    }

    if (H5Sclose(tmp_space_id) < 0)
        TEST_ERROR
    if (H5Sclose(attr_space_id) < 0)
        TEST_ERROR
    if (H5Tclose(tmp_type_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(tmp_space_id);
        H5Sclose(attr_space_id);
        H5Tclose(tmp_type_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute's name can be
 * correctly retrieved with H5Aget_name and
 * H5Aget_name_by_idx.
 */
static int
test_get_attribute_name(void)
{
    hsize_t  dims[ATTRIBUTE_GET_NAME_TEST_SPACE_RANK];
    ssize_t  name_buf_size;
    size_t   i;
    htri_t   attr_exists;
    herr_t   err_ret = -1;
    char    *name_buf = NULL;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    attr_id = -1;
    hid_t    attr_dtype = -1;
    hid_t    space_id = -1;

    TESTING("retrieve attribute name")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_GET_NAME_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_GET_NAME_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving size of attribute's name\n");
#endif

    /* Retrieve the name buffer size */
    if ((name_buf_size = H5Aget_name(attr_id, 0, NULL)) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve name buf size\n");
        goto error;
    }

    if (NULL == (name_buf = (char *) malloc((size_t) name_buf_size + 1)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Retrieving attribute's name\n");
#endif

    if (H5Aget_name(attr_id, (size_t) name_buf_size + 1, name_buf) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute name\n");
    }

    if (strcmp(name_buf, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME)) {
        H5_FAILED();
        printf("    retrieved attribute name '%s' didn't match '%s'\n", name_buf, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME);
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Verifying that this still works after closing and re-opening the attribute\n");
#endif

    /* Now close the attribute and verify that we can still retrieve the attribute's name after
     * opening (instead of creating) it
     */
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

    if ((attr_id = H5Aopen(container_group, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if (H5Aget_name(attr_id, (size_t) name_buf_size + 1, name_buf) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute name\n");
        goto error;
    }

    if (strcmp(name_buf, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME)) {
        H5_FAILED();
        printf("    attribute name didn't match\n");
        goto error;
    }

    if (H5Aget_name_by_idx(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC,
            0, name_buf, (size_t) name_buf_size + 1, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve attribute name by index\n");
        goto error;
    }

    if (strcmp(name_buf, ATTRIBUTE_GET_NAME_TEST_ATTRIBUTE_NAME)) {
        H5_FAILED();
        printf("    attribute name didn't match\n");
        goto error;
    }

    if (name_buf) {
        free(name_buf);
        name_buf = NULL;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        if (name_buf) free(name_buf);
        H5Sclose(space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a space in an attribute's name
 * is not problematic.
 */
static int
test_create_attribute_with_space_in_name(void)
{
    hsize_t dims[ATTRIBUTE_CREATE_WITH_SPACE_IN_NAME_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   attr_id = -1;
    hid_t   attr_dtype = -1;
    hid_t   space_id = -1;

    TESTING("create attribute with a space in its name")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_CREATE_WITH_SPACE_IN_NAME_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_CREATE_WITH_SPACE_IN_NAME_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to create an attribute with a space in its name\n");
#endif

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_CREATE_WITH_SPACE_IN_NAME_ATTR_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_CREATE_WITH_SPACE_IN_NAME_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute can be deleted.
 */
static int
test_delete_attribute(void)
{
    hsize_t dims[ATTRIBUTE_DELETION_TEST_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   attr_id = -1;
    hid_t   attr_dtype = -1;
    hid_t   space_id = -1;

    TESTING("delete an attribute")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_DELETION_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_DELETION_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    /* Test H5Adelete */
    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute didn't exists\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to delete attribute with H5Adelete\n");
#endif

    /* Delete the attribute */
    if (H5Adelete(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME) < 0) {
        H5_FAILED();
        printf("    failed to delete attribute\n");
        goto error;
    }

    /* Verify the attribute has been deleted */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (attr_exists) {
        H5_FAILED();
        printf("    attribute exists!\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to delete attribute with H5Adelete_by_name\n");
#endif

    /* Test H5Adelete_by_name */
    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute didn't exists\n");
        goto error;
    }

    /* Delete the attribute */
    if (H5Adelete_by_name(file_id, ATTRIBUTE_TEST_GROUP_NAME, ATTRIBUTE_DELETION_TEST_ATTR_NAME, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    failed to delete attribute\n");
        goto error;
    }

    /* Verify the attribute has been deleted */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (attr_exists) {
        H5_FAILED();
        printf("    attribute exists!\n");
        goto error;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Attempting to delete attribute with H5Adelete_by_idx\n");
#endif

    /* Test H5Adelete_by_idx */
    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute didn't exists\n");
        goto error;
    }

    if (H5Adelete_by_idx(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    failed to delete attribute by index number\n");
        goto error;
    }

    /* Verify the attribute has been deleted */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_DELETION_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (attr_exists) {
        H5_FAILED();
        printf("    attribute exists!\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a simple write to an attribute
 * can be made.
 */
static int
test_write_attribute(void)
{
    hsize_t  dims[ATTRIBUTE_WRITE_TEST_SPACE_RANK];
    size_t   i, data_size;
    htri_t   attr_exists;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    attr_id = -1;
    hid_t    space_id = -1;
    void    *data = NULL;

    TESTING("write data to an attribute")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_WRITE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_WRITE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_WRITE_TEST_ATTR_NAME, ATTRIBUTE_WRITE_TEST_ATTR_DTYPE,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_WRITE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < ATTRIBUTE_WRITE_TEST_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= ATTRIBUTE_WRITE_TEST_ATTR_DTYPE_SIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / ATTRIBUTE_WRITE_TEST_ATTR_DTYPE_SIZE; i++)
        ((int *) data)[i] = (int) i;

#ifdef VOL_TEST_DEBUG
    puts("Writing to the attribute\n");
#endif

    if (H5Awrite(attr_id, ATTRIBUTE_WRITE_TEST_ATTR_DTYPE, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to attribute\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

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
        H5Sclose(space_id);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that simple data can be read back
 * and verified after it has been written to an
 * attribute.
 */
static int
test_read_attribute(void)
{
    hsize_t  dims[ATTRIBUTE_READ_TEST_SPACE_RANK];
    size_t   i, data_size;
    htri_t   attr_exists;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    attr_id = -1;
    hid_t    space_id = -1;
    void    *data = NULL;
    void    *read_buf = NULL;

    TESTING("read data from an attribute")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_READ_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_READ_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_READ_TEST_ATTR_NAME, ATTRIBUTE_READ_TEST_ATTR_DTYPE,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_READ_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < ATTRIBUTE_READ_TEST_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= ATTRIBUTE_READ_TEST_ATTR_DTYPE_SIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR
    if (NULL == (read_buf = calloc(1, data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / ATTRIBUTE_READ_TEST_ATTR_DTYPE_SIZE; i++)
        ((int *) data)[i] = (int) i;

#ifdef VOL_TEST_DEBUG
    puts("Writing to the attribute\n");
#endif

    if (H5Awrite(attr_id, ATTRIBUTE_READ_TEST_ATTR_DTYPE, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to attribute\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (H5Aclose(attr_id) < 0)
        TEST_ERROR

    if ((attr_id = H5Aopen(container_group, ATTRIBUTE_READ_TEST_ATTR_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Reading from the attribute\n");
#endif

    if (H5Aread(attr_id, ATTRIBUTE_READ_TEST_ATTR_DTYPE, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from attribute\n");
        goto error;
    }

    for (i = 0; i < data_size / ATTRIBUTE_READ_TEST_ATTR_DTYPE_SIZE; i++) {
        if (((int *) read_buf)[i] != (int) i) {
            H5_FAILED();
            printf("    data verification failed\n");
            goto error;
        }
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

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
        if (data) free(data);
        if (read_buf) free(read_buf);
        H5Sclose(space_id);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an attribute can be renamed
 * with H5Arename and H5Arename_by_name.
 */
static int
test_rename_attribute(void)
{
    hsize_t attr_dims[ATTRIBUTE_RENAME_TEST_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   attr_id = -1;
    hid_t   attr_dtype = -1;
    hid_t   attr_space_id = -1;

    TESTING("rename an attribute")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_RENAME_TEST_SPACE_RANK; i++)
        attr_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((attr_space_id = H5Screate_simple(ATTRIBUTE_RENAME_TEST_SPACE_RANK, attr_dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_RENAME_TEST_ATTR_NAME, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_RENAME_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to rename the attribute with H5Arename\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Arename(container_group, ATTRIBUTE_RENAME_TEST_ATTR_NAME, ATTRIBUTE_RENAME_TEST_NEW_NAME);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to rename the attribute with H5Arename_by_name\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Arename_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME, ATTRIBUTE_RENAME_TEST_ATTR_NAME, ATTRIBUTE_RENAME_TEST_NEW_NAME, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(attr_space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(attr_space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that the number of attributes attached
 * to an object (group, dataset, datatype) can be retrieved.
 *
 * XXX: Cover all of the cases.
 */
static int
test_get_number_attributes(void)
{
    H5O_info_t obj_info;
    hsize_t    dims[ATTRIBUTE_GET_NUM_ATTRS_TEST_SPACE_RANK];
    size_t     i;
    htri_t     attr_exists;
    herr_t     err_ret = -1;
    hid_t      file_id = -1, fapl_id = -1;
    hid_t      container_group = -1;
    hid_t      attr_id = -1;
    hid_t      attr_dtype = -1;
    hid_t      space_id = -1;

    TESTING("retrieve the number of attributes on an object")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_GET_NUM_ATTRS_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_GET_NUM_ATTRS_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_GET_NUM_ATTRS_TEST_ATTRIBUTE_NAME, attr_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    /* Verify the attribute has been created */
    if ((attr_exists = H5Aexists(container_group, ATTRIBUTE_GET_NUM_ATTRS_TEST_ATTRIBUTE_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to retrieve the number of attributes on a group with H5Oget_info\n");
#endif

    /* Now get the number of attributes from the group */
    if (H5Oget_info2(container_group, &obj_info, H5O_INFO_ALL) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve root group info\n");
        goto error;
    }

    if (obj_info.num_attrs < 1) {
        H5_FAILED();
        printf("    invalid number of attributes received\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to retrieve the number of attributes on a group with H5Oget_info_by_name\n");
#endif

    if (H5Oget_info_by_name2(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME, &obj_info, H5O_INFO_ALL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve root group info\n");
        goto error;
    }

    if (obj_info.num_attrs < 1) {
        H5_FAILED();
        printf("    invalid number of attributes received\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to retrieve the number of attributes on a group with H5Oget_info_by_idx\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Oget_info_by_idx2(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, 0, &obj_info, H5O_INFO_ALL, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (obj_info.num_attrs < 1) {
        H5_FAILED();
        printf("    invalid number of attributes received\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of attribute
 * iteration using H5Aiterate. Iteration is done
 * in increasing and decreasing order of both
 * attribute name and attribute creation order.
 */
static int
test_attribute_iterate(void)
{
    hsize_t dset_dims[ATTRIBUTE_ITERATE_TEST_DSET_SPACE_RANK];
    hsize_t attr_dims[ATTRIBUTE_ITERATE_TEST_ATTR_SPACE_RANK];
    size_t  i;
    htri_t  attr_exists;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id = -1;
    hid_t   attr_id = -1, attr_id2 = -1, attr_id3 = -1, attr_id4 = -1;
    hid_t   dset_dtype = -1;
    hid_t   attr_dtype = -1;
    hid_t   dset_space_id = -1;
    hid_t   attr_space_id = -1;

    TESTING("attribute iteration")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, ATTRIBUTE_ITERATE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < ATTRIBUTE_ITERATE_TEST_DSET_SPACE_RANK; i++)
        dset_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);
    for (i = 0; i < ATTRIBUTE_ITERATE_TEST_ATTR_SPACE_RANK; i++)
        attr_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_space_id = H5Screate_simple(ATTRIBUTE_ITERATE_TEST_DSET_SPACE_RANK, dset_dims, NULL)) < 0)
        TEST_ERROR
    if ((attr_space_id = H5Screate_simple(ATTRIBUTE_ITERATE_TEST_ATTR_SPACE_RANK, attr_dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, ATTRIBUTE_ITERATE_TEST_DSET_NAME, dset_dtype,
            dset_space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Creating attributes on dataset\n");
#endif

    if ((attr_id = H5Acreate2(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Acreate2(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME2, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    if ((attr_id3 = H5Acreate2(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME3, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    if ((attr_id4 = H5Acreate2(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME4, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Verifying that the attributes exist\n");
#endif

    /* Verify the attributes have been created */
    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME2)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME3)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(dset_id, ATTRIBUTE_ITERATE_TEST_ATTR_NAME4)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by attribute name in increasing order with H5Aiterate2\n");
#endif

    /* Test basic attribute iteration capability using both index types and both index orders */
    if (H5Aiterate2(dset_id, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Aiterate2 by index type name in increasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by attribute name in decreasing order with H5Aiterate2\n");
#endif

    if (H5Aiterate2(dset_id, H5_INDEX_NAME, H5_ITER_DEC, NULL, attr_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Aiterate2 by index type name in decreasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by creation order in increasing order with H5Aiterate2\n");
#endif

    if (H5Aiterate2(dset_id, H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, attr_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Aiterate2 by index type creation order in increasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by creation order in decreasing order with H5Aiterate2\n");
#endif

    if (H5Aiterate2(dset_id, H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, attr_iter_callback1, NULL) < 0) {
        H5_FAILED();
        printf("    H5Aiterate2 by index type creation order in decreasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by attribute name in increasing order with H5Aiterate_by_name\n");
#endif

    if (H5Aiterate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_SUBGROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_DSET_NAME,
            H5_INDEX_NAME, H5_ITER_INC, NULL, attr_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Aiterate_by_name by index type name in increasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by attribute name in decreasing order with H5Aiterate_by_name\n");
#endif

    if (H5Aiterate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_SUBGROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_DSET_NAME,
            H5_INDEX_NAME, H5_ITER_DEC, NULL, attr_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Aiterate_by_name by index type name in decreasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by creation order in increasing order with H5Aiterate_by_name\n");
#endif

    if (H5Aiterate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_SUBGROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_DSET_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_INC, NULL, attr_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Aiterate_by_name by index type creation order in increasing order failed\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    printf("Iterating over attributes by creation order in decreasing order with H5Aiterate_by_name\n");
#endif

    if (H5Aiterate_by_name(file_id, "/" ATTRIBUTE_TEST_GROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_SUBGROUP_NAME "/" ATTRIBUTE_ITERATE_TEST_DSET_NAME,
            H5_INDEX_CRT_ORDER, H5_ITER_DEC, NULL, attr_iter_callback1, NULL, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    H5Aiterate_by_name by index type creation order in decreasing order failed\n");
        goto error;
    }


    /* XXX: Test the H5Aiterate index-saving capabilities */


    if (H5Sclose(dset_space_id) < 0)
        TEST_ERROR
    if (H5Sclose(attr_space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id3) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id4) < 0)
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
        H5Sclose(dset_space_id);
        H5Sclose(attr_space_id);
        H5Tclose(dset_dtype);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Aclose(attr_id2);
        H5Aclose(attr_id3);
        H5Aclose(attr_id4);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that attribute iteration performed
 * on an object with no attributes attached to it is
 * not problematic.
 */
static int
test_attribute_iterate_0_attributes(void)
{
    hsize_t dset_dims[ATTRIBUTE_ITERATE_TEST_0_ATTRIBUTES_DSET_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   dset_space_id = -1;

    TESTING("attribute iteration on object with 0 attributes")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, ATTRIBUTE_ITERATE_TEST_0_ATTRIBUTES_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    for (i = 0; i < ATTRIBUTE_ITERATE_TEST_0_ATTRIBUTES_DSET_SPACE_RANK; i++)
        dset_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((dset_space_id = H5Screate_simple(ATTRIBUTE_ITERATE_TEST_0_ATTRIBUTES_DSET_SPACE_RANK, dset_dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, ATTRIBUTE_ITERATE_TEST_0_ATTRIBUTES_DSET_NAME, dset_dtype,
            dset_space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Aiterate2(dset_id, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_iter_callback2, NULL) < 0) {
        H5_FAILED();
        printf("    H5Aiterate2 by index type name in increasing order failed\n");
        goto error;
    }

    if (H5Sclose(dset_space_id) < 0)
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
        H5Sclose(dset_space_id);
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
 * A test to check that a VOL connector stores and can
 * retrieve a valid copy of an ACPL used at attribute
 * creation time.
 */
static int
test_attribute_property_lists(void)
{
    H5T_cset_t encoding = H5T_CSET_UTF8;
    hsize_t    dims[ATTRIBUTE_PROPERTY_LIST_TEST_SPACE_RANK];
    size_t     i;
    htri_t     attr_exists;
    hid_t      file_id = -1, fapl_id = -1;
    hid_t      container_group = -1, group_id = -1;
    hid_t      attr_id1 = -1, attr_id2 = -1;
    hid_t      attr_dtype1 = -1, attr_dtype2 = -1;
    hid_t      acpl_id1 = -1, acpl_id2 = -1;
    hid_t      space_id = -1;

    TESTING("attribute property list operations")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, ATTRIBUTE_PROPERTY_LIST_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_PROPERTY_LIST_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(ATTRIBUTE_PROPERTY_LIST_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype1 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((attr_dtype2 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((acpl_id1 = H5Pcreate(H5P_ATTRIBUTE_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create ACPL\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Setting property on ACPL\n");
#endif

    if (H5Pset_char_encoding(acpl_id1, encoding) < 0) {
        H5_FAILED();
        printf("    couldn't set ACPL property value\n");
        goto error;
    }

    if ((attr_id1 = H5Acreate2(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME1, attr_dtype1,
            space_id, acpl_id1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Acreate2(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME2, attr_dtype2,
            space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

    if (H5Pclose(acpl_id1) < 0)
        TEST_ERROR

    /* Verify the attributes have been created */
    if ((attr_exists = H5Aexists(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME1)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    if ((attr_exists = H5Aexists(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME2)) < 0) {
        H5_FAILED();
        printf("    couldn't determine if attribute exists\n");
        goto error;
    }

    if (!attr_exists) {
        H5_FAILED();
        printf("    attribute did not exist\n");
        goto error;
    }

    /* Try to retrieve copies of the two property lists, one which ahs the property set and one which does not */
    if ((acpl_id1 = H5Aget_create_plist(attr_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((acpl_id2 = H5Aget_create_plist(attr_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Ensure that property list 1 has the property list set and property list 2 does not */
    encoding = H5T_CSET_ERROR;

    if (H5Pget_char_encoding(acpl_id1, &encoding) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve ACPL property value\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Checking that property set on ACPL was retrieved correctly\n");
#endif

    if (H5T_CSET_UTF8 != encoding) {
        H5_FAILED();
        printf("   ACPL property value was incorrect\n");
        goto error;
    }

    encoding = H5T_CSET_ERROR;

    if (H5Pget_char_encoding(acpl_id2, &encoding) < 0) {
        H5_FAILED();
        printf("    couldn't retrieve ACPL property value\n");
        goto error;
    }

    if (H5T_CSET_UTF8 == encoding) {
        H5_FAILED();
        printf("    ACPL property value was set!\n");
        goto error;
    }

    /* Now close the property lists and attribute and see if we can still retrieve copies of
     * the property lists upon opening (instead of creating) an attribute
     */
    if (H5Pclose(acpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(acpl_id2) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id1) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
        TEST_ERROR

    if ((attr_id1 = H5Aopen(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((attr_id2 = H5Aopen(group_id, ATTRIBUTE_PROPERTY_LIST_TEST_ATTRIBUTE_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open attribute\n");
        goto error;
    }

    if ((acpl_id1 = H5Aget_create_plist(attr_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((acpl_id2 = H5Aget_create_plist(attr_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if (H5Pclose(acpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(acpl_id2) < 0)
        TEST_ERROR
    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype1) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype2) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id1) < 0)
        TEST_ERROR
    if (H5Aclose(attr_id2) < 0)
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
        H5Pclose(acpl_id1);
        H5Pclose(acpl_id2);
        H5Sclose(space_id);
        H5Tclose(attr_dtype1);
        H5Tclose(attr_dtype2);
        H5Aclose(attr_id1);
        H5Aclose(attr_id2);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that the native HDF5-specific API calls
 * are not problematic for a VOL connector which is not the
 * native one.
 *
 * XXX: The calls may not be valid here. May need to create
 * separate tests for them.
 */
static int
test_unused_attribute_API_calls(void)
{
    hsize_t attr_dims[ATTRIBUTE_UNUSED_APIS_TEST_SPACE_RANK];
    size_t  i;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   attr_id = -1;
    hid_t   attr_dtype = -1;
    hid_t   attr_space_id = -1;

    TESTING("unused attribute API calls")

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

    if ((container_group = H5Gopen2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < ATTRIBUTE_UNUSED_APIS_TEST_SPACE_RANK; i++)
        attr_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((attr_space_id = H5Screate_simple(ATTRIBUTE_UNUSED_APIS_TEST_SPACE_RANK, attr_dims, NULL)) < 0)
        TEST_ERROR

    if ((attr_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((attr_id = H5Acreate2(container_group, ATTRIBUTE_UNUSED_APIS_TEST_ATTR_NAME, attr_dtype,
            attr_space_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create attribute\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Testing that all of the unused attribute API calls don't cause application issues\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Aget_storage_size(attr_id);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(attr_space_id) < 0)
        TEST_ERROR
    if (H5Tclose(attr_dtype) < 0)
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
        H5Sclose(attr_space_id);
        H5Tclose(attr_dtype);
        H5Aclose(attr_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static herr_t
attr_iter_callback1(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data)
{
    if (!strcmp(attr_name, ATTRIBUTE_ITERATE_TEST_ATTR_NAME)) {
        if (ainfo->corder != 0) {
            H5_FAILED();
            printf("    attribute corder didn't match\n");
            goto error;
        }

        if (ainfo->corder_valid != 0) {
            H5_FAILED();
            printf("    attribute corder_valid didn't match\n");
            goto error;
        }

        if (ainfo->cset != 0) {
            H5_FAILED();
            printf("    attribute cset didn't match\n");
            goto error;
        }

        if (ainfo->data_size != 0) {
            H5_FAILED();
            printf("    attribute data_size didn't match\n");
            goto error;
        }
    }
    else if (!strcmp(attr_name, ATTRIBUTE_ITERATE_TEST_ATTR_NAME2)) {
        if (ainfo->corder != 0) {
            H5_FAILED();
            printf("    attribute corder didn't match\n");
            goto error;
        }

        if (ainfo->corder_valid != 0) {
            H5_FAILED();
            printf("    attribute corder_valid didn't match\n");
            goto error;
        }

        if (ainfo->cset != 0) {
            H5_FAILED();
            printf("    attribute cset didn't match\n");
            goto error;
        }

        if (ainfo->data_size != 0) {
            H5_FAILED();
            printf("    attribute data_size didn't match\n");
            goto error;
        }
    }
    else if (!strcmp(attr_name, ATTRIBUTE_ITERATE_TEST_ATTR_NAME3)) {
        if (ainfo->corder != 0) {
            H5_FAILED();
            printf("    attribute corder didn't match\n");
            goto error;
        }

        if (ainfo->corder_valid != 0) {
            H5_FAILED();
            printf("    attribute corder_valid didn't match\n");
            goto error;
        }

        if (ainfo->cset != 0) {
            H5_FAILED();
            printf("    attribute cset didn't match\n");
            goto error;
        }

        if (ainfo->data_size != 0) {
            H5_FAILED();
            printf("    attribute data_size didn't match\n");
            goto error;
        }
    }
    else if (!strcmp(attr_name, ATTRIBUTE_ITERATE_TEST_ATTR_NAME4)) {
        if (ainfo->corder != 0) {
            H5_FAILED();
            printf("    attribute corder didn't match\n");
            goto error;
        }

        if (ainfo->corder_valid != 0) {
            H5_FAILED();
            printf("    attribute corder_valid didn't match\n");
            goto error;
        }

        if (ainfo->cset != 0) {
            H5_FAILED();
            printf("    attribute cset didn't match\n");
            goto error;
        }

        if (ainfo->data_size != 0) {
            H5_FAILED();
            printf("    attribute data_size didn't match\n");
            goto error;
        }
    }
    else {
        H5_FAILED();
        printf("    attribute name didn't match known names\n");
        goto error;
    }

    return 0;

error:
    return -1;
}

static herr_t
attr_iter_callback2(hid_t location_id, const char *attr_name, const H5A_info_t *ainfo, void *op_data)
{
    return 0;
}

int
vol_attribute_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_LENGTH(attribute_tests); i++) {
        nerrors += (*attribute_tests[i])() ? 1 : 0;
    }

done:
    return nerrors;
}
