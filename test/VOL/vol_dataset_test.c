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

#include "vol_dataset_test.h"

static int test_create_dataset_under_root(void);
static int test_create_anonymous_dataset(void);
static int test_create_dataset_under_existing_group(void);
static int test_create_dataset_null_space(void);
static int test_create_dataset_scalar_space(void);
static int test_create_dataset_predefined_types(void);
static int test_create_dataset_string_types(void);
static int test_create_dataset_compound_types(void);
static int test_create_dataset_enum_types(void);
static int test_create_dataset_array_types(void);
static int test_create_dataset_shapes(void);
static int test_create_dataset_creation_properties(void);
static int test_write_dataset_small_all(void);
static int test_write_dataset_small_hyperslab(void);
static int test_write_dataset_small_point_selection(void);
#ifndef NO_LARGE_TESTS
static int test_write_dataset_large_all(void);
static int test_write_dataset_large_hyperslab(void);
static int test_write_dataset_large_point_selection(void);
#endif
static int test_read_dataset_small_all(void);
static int test_read_dataset_small_hyperslab(void);
static int test_read_dataset_small_point_selection(void);
#ifndef NO_LARGE_TESTS
static int test_read_dataset_large_all(void);
static int test_read_dataset_large_hyperslab(void);
static int test_read_dataset_large_point_selection(void);
#endif
static int test_write_dataset_data_verification(void);
static int test_dataset_set_extent(void);
static int test_dataset_property_lists(void);
static int test_unused_dataset_API_calls(void);

/*
 * The array of dataset tests to be performed.
 */
static int (*dataset_tests[])(void) = {
        test_create_dataset_under_root,
        test_create_anonymous_dataset,
        test_create_dataset_under_existing_group,
        test_create_dataset_null_space,
        test_create_dataset_scalar_space,
        test_create_dataset_predefined_types,
        test_create_dataset_string_types,
        test_create_dataset_compound_types,
        test_create_dataset_enum_types,
        test_create_dataset_array_types,
        test_create_dataset_shapes,
        test_create_dataset_creation_properties,
        test_write_dataset_small_all,
        test_write_dataset_small_hyperslab,
        test_write_dataset_small_point_selection,
#ifndef NO_LARGE_TESTS
        test_write_dataset_large_all,
        test_write_dataset_large_hyperslab,
        test_write_dataset_large_point_selection,
#endif
        test_read_dataset_small_all,
        test_read_dataset_small_hyperslab,
        test_read_dataset_small_point_selection,
#ifndef NO_LARGE_TESTS
        test_read_dataset_large_all,
        test_read_dataset_large_hyperslab,
        test_read_dataset_large_point_selection,
#endif
        test_write_dataset_data_verification,
        test_dataset_set_extent,
        test_dataset_property_lists,
        test_unused_dataset_API_calls,
};

/*
 * A test to check that a dataset can be created under
 * the root group.
 */
static int
test_create_dataset_under_root(void)
{
    hsize_t dims[DATASET_CREATE_UNDER_ROOT_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("create dataset under root group")

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

    for (i = 0; i < DATASET_CREATE_UNDER_ROOT_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_CREATE_UNDER_ROOT_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating a dataset under the root group\n");
#endif

    /* Create the Dataset under the root group of the file */
    if ((dset_id = H5Dcreate2(file_id, DATASET_CREATE_UNDER_ROOT_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
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
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that an anonymous dataset can be created.
 */
static int
test_create_anonymous_dataset(void)
{
    hsize_t dims[DATASET_CREATE_ANONYMOUS_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("create anonymous dataset")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < DATASET_CREATE_ANONYMOUS_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_CREATE_ANONYMOUS_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating an anonymous dataset\n");
#endif

    if ((dset_id = H5Dcreate_anon(container_group, dset_dtype, fspace_id, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Linking the anonymous dataset into the file structure\n");
#endif

    if (H5Olink(dset_id, container_group, DATASET_CREATE_ANONYMOUS_DATASET_NAME, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't link anonymous dataset into file structure\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Sclose(fspace_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created under
 * a group that is not the root group.
 */
static int
test_create_dataset_under_existing_group(void)
{
    hsize_t dims[DATASET_CREATE_UNDER_EXISTING_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   group_id = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("create dataset under existing group")

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

    if ((group_id = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group\n");
        goto error;
    }

    for (i = 0; i < DATASET_CREATE_UNDER_EXISTING_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_CREATE_UNDER_EXISTING_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating dataset under non-root group\n");
#endif

    if ((dset_id = H5Dcreate2(group_id, DATASET_CREATE_UNDER_EXISTING_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
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
        H5Sclose(fspace_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that creating a dataset with a NULL
 * dataspace is not problematic.
 */
static int
test_create_dataset_null_space(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t container_group = -1, group_id = -1;
    hid_t dset_id = -1;
    hid_t dset_dtype = -1;
    hid_t fspace_id = -1;

    TESTING("create dataset with a NULL dataspace")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_CREATE_NULL_DATASPACE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((fspace_id = H5Screate(H5S_NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    printf("Creating dataset with NULL dataspace\n");
#endif

    if ((dset_id = H5Dcreate2(group_id, DATASET_CREATE_NULL_DATASPACE_TEST_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(group_id, DATASET_CREATE_NULL_DATASPACE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
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
        H5Sclose(fspace_id);
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
 * A test to check that creating a dataset with a scalar
 * dataspace is not problematic.
 */
static int
test_create_dataset_scalar_space(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t container_group = -1, group_id = -1;
    hid_t dset_id = -1;
    hid_t dset_dtype = -1;
    hid_t fspace_id = -1;

    TESTING("create dataset with a SCALAR dataspace")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_CREATE_SCALAR_DATASPACE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((fspace_id = H5Screate(H5S_SCALAR)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    printf("Creating dataset with SCALAR dataspace\n");
#endif

    if ((dset_id = H5Dcreate2(group_id, DATASET_CREATE_SCALAR_DATASPACE_TEST_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(group_id, DATASET_CREATE_SCALAR_DATASPACE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
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
        H5Sclose(fspace_id);
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
 * A test to check that a dataset can be created using
 * each of the predefined integer and floating-point
 * datatypes.
 */
static int
test_create_dataset_predefined_types(void)
{
    size_t i;
    hid_t  file_id = -1, fapl_id = -1;
    hid_t  container_group = -1, group_id = -1;
    hid_t  fspace_id = -1;
    hid_t  dset_id = -1;
    hid_t  predefined_type_test_table[] = {
            H5T_STD_U8LE,   H5T_STD_U8BE,   H5T_STD_I8LE,   H5T_STD_I8BE,
            H5T_STD_U16LE,  H5T_STD_U16BE,  H5T_STD_I16LE,  H5T_STD_I16BE,
            H5T_STD_U32LE,  H5T_STD_U32BE,  H5T_STD_I32LE,  H5T_STD_I32BE,
            H5T_STD_U64LE,  H5T_STD_U64BE,  H5T_STD_I64LE,  H5T_STD_I64BE,
            H5T_IEEE_F32LE, H5T_IEEE_F32BE, H5T_IEEE_F64LE, H5T_IEEE_F64BE
    };

    TESTING("dataset creation w/ predefined datatypes")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_PREDEFINED_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create sub-container group\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating datasets with the different predefined integer/floating-point datatypes\n");
#endif

    for (i = 0; i < ARRAY_LENGTH(predefined_type_test_table); i++) {
        hsize_t dims[DATASET_PREDEFINED_TYPE_TEST_SPACE_RANK];
        size_t  j;
        char    name[100];

        for (j = 0; j < DATASET_PREDEFINED_TYPE_TEST_SPACE_RANK; j++)
            dims[j] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

        if ((fspace_id = H5Screate_simple(DATASET_PREDEFINED_TYPE_TEST_SPACE_RANK, dims, NULL)) < 0)
            TEST_ERROR

        sprintf(name, "%s%zu", DATASET_PREDEFINED_TYPE_TEST_BASE_NAME, i);

        if ((dset_id = H5Dcreate2(group_id, name, predefined_type_test_table[i], fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Sclose(fspace_id) < 0)
            TEST_ERROR
        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, name, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    failed to open dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
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
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created using
 * string datatypes.
 */
static int
test_create_dataset_string_types(void)
{
    hsize_t dims[DATASET_STRING_TYPE_TEST_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id_fixed = -1, dset_id_variable = -1;
    hid_t   type_id_fixed = -1, type_id_variable = -1;
    hid_t   fspace_id = -1;

    TESTING("dataset creation w/ string types")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_STRING_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    if ((type_id_fixed = H5Tcreate(H5T_STRING, DATASET_STRING_TYPE_TEST_STRING_LENGTH)) < 0) {
        H5_FAILED();
        printf("    couldn't create fixed-length string type\n");
        goto error;
    }

    if ((type_id_variable = H5Tcreate(H5T_STRING, H5T_VARIABLE)) < 0) {
        H5_FAILED();
        printf("    couldn't create variable-length string type\n");
        goto error;
    }

    for (i = 0; i < DATASET_STRING_TYPE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_STRING_TYPE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating dataset with fixed-length string datatype\n");
#endif

    if ((dset_id_fixed = H5Dcreate2(group_id, DATASET_STRING_TYPE_TEST_DSET_NAME1, type_id_fixed, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create fixed-length string dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating dataset with variable-length string datatype\n");
#endif

    if ((dset_id_variable = H5Dcreate2(group_id, DATASET_STRING_TYPE_TEST_DSET_NAME2, type_id_variable, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create variable-length string dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to re-open the datasets\n");
#endif

    if (H5Dclose(dset_id_fixed) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_variable) < 0)
        TEST_ERROR

    if ((dset_id_fixed = H5Dopen2(group_id, DATASET_STRING_TYPE_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if ((dset_id_variable = H5Dopen2(group_id, DATASET_STRING_TYPE_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to opend dataset\n");
        goto error;
    }

    if (H5Tclose(type_id_fixed) < 0)
        TEST_ERROR
    if (H5Tclose(type_id_variable) < 0)
        TEST_ERROR
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_fixed) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_variable) < 0)
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
        H5Tclose(type_id_fixed);
        H5Tclose(type_id_variable);
        H5Sclose(fspace_id);
        H5Dclose(dset_id_fixed);
        H5Dclose(dset_id_variable);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created using
 * a variety of compound datatypes.
 */
static int
test_create_dataset_compound_types(void)
{
    hsize_t  dims[DATASET_COMPOUND_TYPE_TEST_DSET_RANK];
    size_t   i, j;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1, group_id = -1;
    hid_t    compound_type = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    hid_t    type_pool[DATASET_COMPOUND_TYPE_TEST_MAX_SUBTYPES];
    int      num_passes;

    TESTING("dataset creation w/ compound datatypes")

    for (j = 0; j < DATASET_COMPOUND_TYPE_TEST_MAX_SUBTYPES; j++)
        type_pool[j] = -1;

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_COMPOUND_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < DATASET_COMPOUND_TYPE_TEST_DSET_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_COMPOUND_TYPE_TEST_DSET_RANK, dims, NULL)) < 0)
        TEST_ERROR

    num_passes = (rand() % DATASET_COMPOUND_TYPE_TEST_MAX_PASSES) + 1;

#ifdef VOL_TEST_DEBUG
    puts("Creating datasets with a variety of randomly-generated compound datatypes\n");
#endif

    for (i = 0; i < (size_t) num_passes; i++) {
        size_t num_subtypes;
        size_t compound_size = 0;
        size_t next_offset = 0;
        char   dset_name[256];

        for (j = 0; j < DATASET_COMPOUND_TYPE_TEST_MAX_SUBTYPES; j++)
            type_pool[j] = -1;

        num_subtypes = (size_t) (rand() % DATASET_COMPOUND_TYPE_TEST_MAX_SUBTYPES) + 1;

        if ((compound_type = H5Tcreate(H5T_COMPOUND, 1)) < 0) {
            H5_FAILED();
            printf("    couldn't create compound datatype\n");
            goto error;
        }

        /* Start adding subtypes to the compound type */
        for (j = 0; j < num_subtypes; j++) {
            size_t member_size;
            char   member_name[256];

            snprintf(member_name, 256, "member%zu", j);

            if ((type_pool[j] = generate_random_datatype(H5T_NO_CLASS)) < 0) {
                H5_FAILED();
                printf("    couldn't create compound datatype member %zu\n", j);
                goto error;
            }

            if (!(member_size = H5Tget_size(type_pool[j]))) {
                H5_FAILED();
                printf("    couldn't get compound member %zu size\n", j);
                goto error;
            }

            compound_size += member_size;

            if (H5Tset_size(compound_type, compound_size) < 0)
                TEST_ERROR

            if (H5Tinsert(compound_type, member_name, next_offset, type_pool[j]) < 0)
                TEST_ERROR

            next_offset += member_size;
        }

        if (H5Tpack(compound_type) < 0)
            TEST_ERROR

        snprintf(dset_name, sizeof(dset_name), "%s%zu", DATASET_COMPOUND_TYPE_TEST_DSET_NAME, i);

        if ((dset_id = H5Dcreate2(group_id, dset_name, compound_type, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, dset_name, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    failed to open dataset\n");
            goto error;
        }

        for (j = 0; j < num_subtypes; j++)
            if (type_pool[j] >= 0 && H5Tclose(type_pool[j]) < 0)
                TEST_ERROR
        if (H5Tclose(compound_type) < 0)
            TEST_ERROR
        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
    }

    if (H5Sclose(fspace_id) < 0)
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
        for (i = 0; i < DATASET_COMPOUND_TYPE_TEST_MAX_SUBTYPES; i++)
            H5Tclose(type_pool[i]);
        H5Tclose(compound_type);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created with
 * enum datatypes.
 */
static int
test_create_dataset_enum_types(void)
{
    hsize_t     dims[DATASET_ENUM_TYPE_TEST_SPACE_RANK];
    size_t      i;
    hid_t       file_id = -1, fapl_id = -1;
    hid_t       container_group = -1, group_id = -1;
    hid_t       dset_id_native = -1, dset_id_non_native = -1;
    hid_t       fspace_id = -1;
    hid_t       enum_native = -1, enum_non_native = -1;
    const char *enum_type_test_table[] = {
            "RED",    "GREEN",  "BLUE",
            "BLACK",  "WHITE",  "PURPLE",
            "ORANGE", "YELLOW", "BROWN"
    };

    TESTING("dataset creation w/ enum types")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_ENUM_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    if ((enum_native = H5Tcreate(H5T_ENUM, sizeof(int))) < 0) {
        H5_FAILED();
        printf("    couldn't create native enum type\n");
        goto error;
    }

    for (i = 0; i < ARRAY_LENGTH(enum_type_test_table); i++)
        if (H5Tenum_insert(enum_native, enum_type_test_table[i], &i) < 0)
            TEST_ERROR

    if ((enum_non_native = H5Tenum_create(H5T_STD_U32LE)) < 0) {
        H5_FAILED();
        printf("    couldn't create non-native enum type\n");
        goto error;
    }

    for (i = 0; i < DATASET_ENUM_TYPE_TEST_NUM_MEMBERS; i++) {
        char val_name[15];

        sprintf(val_name, "%s%zu", DATASET_ENUM_TYPE_TEST_VAL_BASE_NAME, i);

        if (H5Tenum_insert(enum_non_native, val_name, &i) < 0)
            TEST_ERROR
    }

    for (i = 0; i < DATASET_ENUM_TYPE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_ENUM_TYPE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating dataset with native enum datatype\n");
#endif

    if ((dset_id_native = H5Dcreate2(group_id, DATASET_ENUM_TYPE_TEST_DSET_NAME1, enum_native, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create native enum dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Creating dataset with non-native enum datatype\n");
#endif

    if ((dset_id_non_native = H5Dcreate2(group_id, DATASET_ENUM_TYPE_TEST_DSET_NAME2, enum_non_native, fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create non-native enum dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to re-open the datasets\n");
#endif

    if (H5Dclose(dset_id_native) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_non_native) < 0)
        TEST_ERROR

    if ((dset_id_native = H5Dopen2(group_id, DATASET_ENUM_TYPE_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if ((dset_id_non_native = H5Dopen2(group_id, DATASET_ENUM_TYPE_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if (H5Tclose(enum_native) < 0)
        TEST_ERROR
    if (H5Tclose(enum_non_native) < 0)
        TEST_ERROR
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_native) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id_non_native) < 0)
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
        H5Tclose(enum_native);
        H5Tclose(enum_non_native);
        H5Sclose(fspace_id);
        H5Dclose(dset_id_native);
        H5Dclose(dset_id_non_native);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created using
 * array datatypes.
 */
static int
test_create_dataset_array_types(void)
{
    hsize_t dset_dims[DATASET_ARRAY_TYPE_TEST_SPACE_RANK];
    hsize_t array_dims1[DATASET_ARRAY_TYPE_TEST_RANK1];
    hsize_t array_dims2[DATASET_ARRAY_TYPE_TEST_RANK2];
    hsize_t array_dims3[DATASET_ARRAY_TYPE_TEST_RANK3];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id1 = -1, dset_id2 = -1;
    hid_t   fspace_id = -1;
    hid_t   array_type_id1 = -1, array_type_id2 = -1;
    hid_t   array_base_type_id1 = -1, array_base_type_id2 = -1;
    hid_t   array_base_type_id3 = -1;
    hid_t   array_type_id3 = -1;
    hid_t   nested_type_id = -1;
    hid_t   dset_id3 = -1;
    hid_t   non_predefined_type_id = -1;

    TESTING("dataset creation w/ array types")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_ARRAY_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    /* Test creation of array with some different types */
    for (i = 0; i < DATASET_ARRAY_TYPE_TEST_RANK1; i++)
        array_dims1[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((array_base_type_id1 = generate_random_datatype(H5T_ARRAY)) < 0)
        TEST_ERROR

    if ((array_type_id1 = H5Tarray_create(array_base_type_id1, DATASET_ARRAY_TYPE_TEST_RANK1, array_dims1)) < 0) {
        H5_FAILED();
        printf("    couldn't create predefined integer array type\n");
        goto error;
    }

    for (i = 0; i < DATASET_ARRAY_TYPE_TEST_RANK2; i++)
        array_dims2[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((array_base_type_id2 = generate_random_datatype(H5T_ARRAY)) < 0)
        TEST_ERROR

    if ((array_type_id2 = H5Tarray_create(array_base_type_id2, DATASET_ARRAY_TYPE_TEST_RANK2, array_dims2)) < 0) {
        H5_FAILED();
        printf("    couldn't create predefined floating-point array type\n");
        goto error;
    }

    /* Test nested arrays */
    for (i = 0; i < DATASET_ARRAY_TYPE_TEST_RANK3; i++)
        array_dims3[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((array_base_type_id3 = generate_random_datatype(H5T_ARRAY)) < 0)
        TEST_ERROR

    if ((nested_type_id = H5Tarray_create(array_base_type_id3, DATASET_ARRAY_TYPE_TEST_RANK3, array_dims3)) < 0) {
        H5_FAILED();
        printf("    couldn't create nested array base type\n");
        goto error;
    }

    if ((array_type_id3 = H5Tarray_create(nested_type_id, DATASET_ARRAY_TYPE_TEST_RANK3, array_dims3)) < 0) {
        H5_FAILED();
        printf("    couldn't create nested array type\n");
        goto error;
    }

    for (i = 0; i < DATASET_ARRAY_TYPE_TEST_SPACE_RANK; i++)
        dset_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_ARRAY_TYPE_TEST_SPACE_RANK, dset_dims, NULL)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating datasets with a variety of randomly-generated array datatypes\n");
#endif

    if ((dset_id1 = H5Dcreate2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME1, array_type_id1, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create array type dataset\n");
        goto error;
    }

    if ((dset_id2 = H5Dcreate2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME2, array_type_id2, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create array type dataset\n");
        goto error;
    }

    if ((dset_id3 = H5Dcreate2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME3, array_type_id3, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create nested array type dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Attempting to re-open the datasets\n");
#endif

    if (H5Dclose(dset_id1) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id3) < 0)
        TEST_ERROR

    if ((dset_id1 = H5Dopen2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if ((dset_id2 = H5Dopen2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if ((dset_id3 = H5Dopen2(group_id, DATASET_ARRAY_TYPE_TEST_DSET_NAME3, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    failed to open dataset\n");
        goto error;
    }

    if (H5Tclose(array_type_id1) < 0)
        TEST_ERROR
    if (H5Tclose(array_type_id2) < 0)
        TEST_ERROR
    if (H5Tclose(array_type_id3) < 0)
        TEST_ERROR
    if (H5Tclose(nested_type_id) < 0)
        TEST_ERROR
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id1) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id3) < 0)
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
        H5Tclose(array_type_id1);
        H5Tclose(array_type_id2);
        H5Tclose(array_type_id3);
        H5Tclose(nested_type_id);
        H5Tclose(non_predefined_type_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id1);
        H5Dclose(dset_id2);
        H5Dclose(dset_id3);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset can be created with
 * a variety of different dataspace shapes.
 */
static int
test_create_dataset_shapes(void)
{
    hsize_t *dims = NULL;
    size_t   i;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1, group_id = -1;
    hid_t    dset_id = -1, space_id = -1;
    hid_t    dset_dtype = -1;

    TESTING("dataset creation w/ random dimension sizes")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_SHAPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating datasets with a variety of randomly-generated dataspace shapes\n");
#endif

    for (i = 0; i < DATASET_SHAPE_TEST_NUM_ITERATIONS; i++) {
        size_t j;
        char   name[100];
        int    ndims = rand() % DATASET_SHAPE_TEST_MAX_DIMS + 1;

        if (NULL == (dims = (hsize_t *) malloc((size_t) ndims * sizeof(*dims)))) {
            H5_FAILED();
            printf("    couldn't allocate space for dataspace dimensions\n");
            goto error;
        }

        for (j = 0; j < (size_t) ndims; j++)
            dims[j] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

        if ((space_id = H5Screate_simple(ndims, dims, NULL)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataspace\n");
            goto error;
        }

        sprintf(name, "%s%zu", DATASET_SHAPE_TEST_DSET_BASE_NAME, i + 1);

        if ((dset_id = H5Dcreate2(group_id, name, dset_dtype, space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (dims) {
            free(dims);
            dims = NULL;
        }

        if (H5Sclose(space_id) < 0)
            TEST_ERROR
        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
    }

    if (H5Tclose(dset_dtype) < 0)
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
        if (dims) free(dims);
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
 * A test to check the functionality of the different
 * dataset creation properties.
 */
static int
test_create_dataset_creation_properties(void)
{
    hsize_t dims[DATASET_CREATION_PROPERTIES_TEST_SHAPE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id = -1, dcpl_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("dataset creation properties")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_CREATION_PROPERTIES_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    for (i = 0; i < DATASET_CREATION_PROPERTIES_TEST_SHAPE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_CREATION_PROPERTIES_TEST_SHAPE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Creating a variety of datasets with different creation properties\n");
#endif

    /* Test the alloc time property */
    {
        H5D_alloc_time_t alloc_times[] = {
                H5D_ALLOC_TIME_DEFAULT, H5D_ALLOC_TIME_EARLY,
                H5D_ALLOC_TIME_INCR, H5D_ALLOC_TIME_LATE
        };

#ifdef VOL_TEST_DEBUG
    puts("Testing the alloc time property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        for (i = 0; i < ARRAY_LENGTH(alloc_times); i++) {
            char name[100];

            if (H5Pset_alloc_time(dcpl_id, alloc_times[i]) < 0)
                TEST_ERROR

            sprintf(name, "%s%zu", DATASET_CREATION_PROPERTIES_TEST_ALLOC_TIMES_BASE_NAME, i);

            if ((dset_id = H5Dcreate2(group_id, name, dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't create dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR

            if ((dset_id = H5Dopen2(group_id, name, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't open dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR
        }

        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* Test the attribute creation order property */
    {
        unsigned creation_orders[] = {
                H5P_CRT_ORDER_TRACKED,
                H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED
        };

#ifdef VOL_TEST_DEBUG
    puts("Testing the attribute creation order property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        for (i = 0; i < ARRAY_LENGTH(creation_orders); i++) {
            char name[100];

            if (H5Pset_attr_creation_order(dcpl_id, creation_orders[i]) < 0)
                TEST_ERROR

            sprintf(name, "%s%zu", DATASET_CREATION_PROPERTIES_TEST_CRT_ORDER_BASE_NAME, i);

            if ((dset_id = H5Dcreate2(group_id, name, dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't create dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR

            if ((dset_id = H5Dopen2(group_id, name, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't open dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR
        }

        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* Test the attribute phase change property */
    {
#ifdef VOL_TEST_DEBUG
    puts("Testing the attribute phase change property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        if (H5Pset_attr_phase_change(dcpl_id,
                DATASET_CREATION_PROPERTIES_TEST_MAX_COMPACT, DATASET_CREATION_PROPERTIES_TEST_MIN_DENSE) < 0)
            TEST_ERROR

        if ((dset_id = H5Dcreate2(group_id, DATASET_CREATION_PROPERTIES_TEST_PHASE_CHANGE_DSET_NAME,
                dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, DATASET_CREATION_PROPERTIES_TEST_PHASE_CHANGE_DSET_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* Test the fill time property */
    {
        H5D_fill_time_t fill_times[] = {
                H5D_FILL_TIME_IFSET, H5D_FILL_TIME_ALLOC,
                H5D_FILL_TIME_NEVER
        };

#ifdef VOL_TEST_DEBUG
    puts("Testing the fill time property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        for (i = 0; i < ARRAY_LENGTH(fill_times); i++) {
            char name[100];

            if (H5Pset_fill_time(dcpl_id, fill_times[i]) < 0)
                TEST_ERROR

            sprintf(name, "%s%zu", DATASET_CREATION_PROPERTIES_TEST_FILL_TIMES_BASE_NAME, i);

            if ((dset_id = H5Dcreate2(group_id, name, dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't create dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR

            if ((dset_id = H5Dopen2(group_id, name, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't open dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR
        }

        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* TODO: Test the fill value property */
    {

    }

    {
#ifdef VOL_TEST_DEBUG
        puts("Testing dataset filters\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        /* Set all of the available filters on the DCPL */
        if (H5Pset_deflate(dcpl_id, 7) < 0)
            TEST_ERROR
        if (H5Pset_shuffle(dcpl_id) < 0)
            TEST_ERROR
        if (H5Pset_fletcher32(dcpl_id) < 0)
            TEST_ERROR
        if (H5Pset_nbit(dcpl_id) < 0)
            TEST_ERROR
        if (H5Pset_scaleoffset(dcpl_id, H5Z_SO_FLOAT_ESCALE, 2) < 0)
            TEST_ERROR

        if ((dset_id = H5Dcreate2(group_id, DATASET_CREATION_PROPERTIES_TEST_FILTERS_DSET_NAME, dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, DATASET_CREATION_PROPERTIES_TEST_FILTERS_DSET_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* Test the storage layout property */
    {
        H5D_layout_t layouts[] = {
                H5D_COMPACT, H5D_CONTIGUOUS, H5D_CHUNKED
        };

#ifdef VOL_TEST_DEBUG
        puts("Testing the storage layout property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        for (i = 0; i < ARRAY_LENGTH(layouts); i++) {
            char name[100];

            if (H5Pset_layout(dcpl_id, layouts[i]) < 0)
                TEST_ERROR

            if (H5D_CHUNKED == layouts[i]) {
                hsize_t chunk_dims[DATASET_CREATION_PROPERTIES_TEST_CHUNK_DIM_RANK];
                size_t  j;

                for (j = 0; j < DATASET_CREATION_PROPERTIES_TEST_CHUNK_DIM_RANK; j++)
                    chunk_dims[j] = (hsize_t) (rand() % (int) dims[j] + 1);

                if (H5Pset_chunk(dcpl_id , DATASET_CREATION_PROPERTIES_TEST_CHUNK_DIM_RANK, chunk_dims) < 0)
                    TEST_ERROR
            }

            sprintf(name, "%s%zu", DATASET_CREATION_PROPERTIES_TEST_LAYOUTS_BASE_NAME, i);

            if ((dset_id = H5Dcreate2(group_id, name, dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't create dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR

            if ((dset_id = H5Dopen2(group_id, name, H5P_DEFAULT)) < 0) {
                H5_FAILED();
                printf("    couldn't open dataset\n");
                goto error;
            }

            if (H5Dclose(dset_id) < 0)
                TEST_ERROR
        }

        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    /* Test the "track object times" property */
    {
#ifdef VOL_TEST_DEBUG
    puts("Testing the object time tracking property\n");
#endif

        if ((dcpl_id = H5Pcreate(H5P_DATASET_CREATE)) < 0)
            TEST_ERROR

        if (H5Pset_obj_track_times(dcpl_id, true) < 0)
            TEST_ERROR

        if ((dset_id = H5Dcreate2(group_id, DATASET_CREATION_PROPERTIES_TEST_TRACK_TIMES_YES_DSET_NAME,
                dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, DATASET_CREATION_PROPERTIES_TEST_TRACK_TIMES_YES_DSET_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if (H5Pset_obj_track_times(dcpl_id, false) < 0)
            TEST_ERROR

        if ((dset_id = H5Dcreate2(group_id, DATASET_CREATION_PROPERTIES_TEST_TRACK_TIMES_NO_DSET_NAME,
                dset_dtype, fspace_id, H5P_DEFAULT, dcpl_id, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't create dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR

        if ((dset_id = H5Dopen2(group_id, DATASET_CREATION_PROPERTIES_TEST_TRACK_TIMES_NO_DSET_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open dataset\n");
            goto error;
        }

        if (H5Dclose(dset_id) < 0)
            TEST_ERROR
        if (H5Pclose(dcpl_id) < 0)
            TEST_ERROR
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Pclose(dcpl_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a small write can be
 * made to a dataset using an H5S_ALL selection.
 */
static int
test_write_dataset_small_all(void)
{
    hssize_t  space_npoints;
    hsize_t   dims[DATASET_SMALL_WRITE_TEST_ALL_DSET_SPACE_RANK] = { 10, 5, 3 };
    size_t    i;
    hid_t     file_id = -1, fapl_id = -1;
    hid_t     container_group = -1;
    hid_t     dset_id = -1;
    hid_t     fspace_id = -1;
    void     *data = NULL;

    TESTING("small write to dataset w/ H5S_ALL")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_WRITE_TEST_ALL_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_WRITE_TEST_ALL_DSET_NAME, DATASET_SMALL_WRITE_TEST_ALL_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* Close the dataset and dataspace to ensure that retrieval of file space ID is working */
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR;
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(file_id, "/" DATASET_TEST_GROUP_NAME "/" DATASET_SMALL_WRITE_TEST_ALL_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((fspace_id = H5Dget_space(dset_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataset dataspace\n");
        goto error;
    }

    if ((space_npoints = H5Sget_simple_extent_npoints(fspace_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataspace num points\n");
        goto error;
    }

    if (NULL == (data = malloc((hsize_t) space_npoints * DATASET_SMALL_WRITE_TEST_ALL_DSET_DTYPESIZE)))
        TEST_ERROR

    for (i = 0; i < (hsize_t) space_npoints; i++)
        ((int *) data)[i] = (int) i;

#ifdef VOL_TEST_DEBUG
    puts("Writing to entire dataset with a small amount of data\n");
#endif

    if (H5Dwrite(dset_id, DATASET_SMALL_WRITE_TEST_ALL_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

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
        if (data) free(data);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a small write can be made
 * to a dataset using a hyperslab selection.
 */
static int
test_write_dataset_small_hyperslab(void)
{
    hsize_t  start[DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  stride[DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  count[DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  block[DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK] = { 10, 5, 3 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    mspace_id = -1, fspace_id = -1;
    void    *data = NULL;

    TESTING("small write to dataset w/ hyperslab")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR
    if ((mspace_id = H5Screate_simple(DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK - 1, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_NAME, DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK - 1; i++)
        data_size *= dims[i];
    data_size *= DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_DTYPESIZE; i++)
        ((int *) data)[i] = (int) i;

    for (i = 0; i < DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK; i++) {
        start[i] = 0;
        stride[i] = 1;
        count[i] = dims[i];
        block[i] = 1;
    }

    count[2] = 1;

    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, stride, count, block) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Writing small amount of data to dataset using a hyperslab selection\n");
#endif

    if (H5Dwrite(dset_id, DATASET_SMALL_WRITE_TEST_HYPERSLAB_DSET_DTYPE, mspace_id, fspace_id, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (H5Sclose(mspace_id) < 0)
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
        if (data) free(data);
        H5Sclose(mspace_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a small write can be made
 * to a dataset using a point selection.
 */
static int
test_write_dataset_small_point_selection(void)
{
    hsize_t  points[DATASET_SMALL_WRITE_TEST_POINT_SELECTION_NUM_POINTS * DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_SPACE_RANK] = { 10, 10, 10 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    void    *data = NULL;

    TESTING("small write to dataset w/ point selection")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_NAME, DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    data_size = DATASET_SMALL_WRITE_TEST_POINT_SELECTION_NUM_POINTS * DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_DTYPESIZE; i++)
        ((int *) data)[i] = (int) i;

    for (i = 0; i < DATASET_SMALL_WRITE_TEST_POINT_SELECTION_NUM_POINTS; i++) {
        size_t j;

        for (j = 0; j < DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_SPACE_RANK; j++)
            points[(i * DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_SPACE_RANK) + j] = i;
    }

    if (H5Sselect_elements(fspace_id, H5S_SELECT_SET, DATASET_SMALL_WRITE_TEST_POINT_SELECTION_NUM_POINTS, points) < 0) {
        H5_FAILED();
        printf("    couldn't select points\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing a small amount of data to dataset using a point selection\n");
#endif

    if (H5Dwrite(dset_id, DATASET_SMALL_WRITE_TEST_POINT_SELECTION_DSET_DTYPE, H5S_ALL, fspace_id, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

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
        if (data) free(data);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

#ifndef NO_LARGE_TESTS
/*
 * A test to check that a large write can be made
 * to a dataset using an H5S_ALL selection.
 */
static int
test_write_dataset_large_all(void)
{
    hssize_t  space_npoints;
    hsize_t   dims[DATASET_LARGE_WRITE_TEST_ALL_DSET_SPACE_RANK] = { 600, 600, 600 };
    size_t    i, data_size;
    hid_t     file_id = -1, fapl_id = -1;
    hid_t     container_group = -1;
    hid_t     dset_id = -1;
    hid_t     fspace_id = -1;
    void     *data = NULL;

    TESTING("write to large dataset w/ H5S_ALL")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_LARGE_WRITE_TEST_ALL_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_LARGE_WRITE_TEST_ALL_DSET_NAME, DATASET_LARGE_WRITE_TEST_ALL_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* Close the dataset and dataspace to ensure that retrieval of file space ID is working */
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(file_id, "/" DATASET_TEST_GROUP_NAME "/" DATASET_LARGE_WRITE_TEST_ALL_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((fspace_id = H5Dget_space(dset_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataset dataspace\n");
        goto error;
    }

    if ((space_npoints = H5Sget_simple_extent_npoints(fspace_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataspace num points\n");
        goto error;
    }

    if (NULL == (data = malloc((hsize_t) space_npoints * DATASET_LARGE_WRITE_TEST_ALL_DSET_DTYPESIZE)))
        TEST_ERROR

    for (i = 0; i < (hsize_t) space_npoints; i++)
        ((int *) data)[i] = (int) i;

#ifdef VOL_TEST_DEBUG
    puts("Writing to entire dataset with a large amount of data\n");
#endif

    if (H5Dwrite(dset_id, DATASET_LARGE_WRITE_TEST_ALL_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

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
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a large write can be made
 * to a dataset using a hyperslab selection.
 */
static int
test_write_dataset_large_hyperslab(void)
{
    hsize_t  start[DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  stride[DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  count[DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  block[DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK] = { 600, 600, 600 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    mspace_id = -1, fspace_id = -1;
    void    *data = NULL;

    TESTING("write to large dataset w/ hyperslab selection")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR
    if ((mspace_id = H5Screate_simple(DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_NAME, DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_DTYPESIZE; i++)
        ((int *) data)[i] = (int) i;

    for (i = 0; i < DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_SPACE_RANK; i++) {
        start[i] = 0;
        stride[i] = 1;
        count[i] = dims[i];
        block[i] = 1;
    }

    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, stride, count, block) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Writing large amount of data to dataset using a hyperslab selection\n");
#endif

    if (H5Dwrite(dset_id, DATASET_LARGE_WRITE_TEST_HYPERSLAB_DSET_DTYPE, mspace_id, fspace_id, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (H5Sclose(mspace_id) < 0)
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
        if (data) free(data);
        H5Sclose(mspace_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a large write can be made
 * to a dataset using a point selection.
 */
static int
test_write_dataset_large_point_selection(void)
{
    TESTING("write to large dataset w/ point selection")

    SKIPPED();

    return 0;

error:
    return 1;
}
#endif

/*
 * A test to check that a small amount of data can be
 * read back from a dataset using an H5S_ALL selection
 * and then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_small_all(void)
{
    hsize_t  dims[DATASET_SMALL_READ_TEST_ALL_DSET_SPACE_RANK] = { 10, 5, 3 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    void    *read_buf = NULL;

    TESTING("small read from dataset w/ H5S_ALL")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_READ_TEST_ALL_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_READ_TEST_ALL_DSET_NAME, DATASET_SMALL_READ_TEST_ALL_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < DATASET_SMALL_READ_TEST_ALL_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_SMALL_READ_TEST_ALL_DSET_DTYPESIZE;

    if (NULL == (read_buf = malloc(data_size)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Reading entirety of small dataset\n");
#endif

    if (H5Dread(dset_id, DATASET_SMALL_READ_TEST_ALL_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

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
        if (read_buf) free(read_buf);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a small amount of data can be
 * read back from a dataset using a hyperslab selection
 * and then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_small_hyperslab(void)
{
    hsize_t  start[DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  stride[DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  count[DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  block[DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK] = { 10, 5, 3 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    mspace_id = -1, fspace_id = -1;
    void    *read_buf = NULL;

    TESTING("small read from dataset w/ hyperslab")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR
    if ((mspace_id = H5Screate_simple(DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK - 1, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_NAME, DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0; i < DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK; i++) {
        start[i] = 0;
        stride[i] = 1;
        count[i] = dims[i];
        block[i] = 1;
    }

    count[2] = 1;

    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, stride, count, block) < 0)
        TEST_ERROR

    for (i = 0, data_size = 1; i < DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_SPACE_RANK - 1; i++)
        data_size *= dims[i];
    data_size *= DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_DTYPESIZE;

    if (NULL == (read_buf = malloc(data_size)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Reading portion of small dataset using hyperslab selection\n");
#endif

    if (H5Dread(dset_id, DATASET_SMALL_READ_TEST_HYPERSLAB_DSET_DTYPE, mspace_id, fspace_id, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

    if (H5Sclose(mspace_id) < 0)
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
        if (read_buf) free(read_buf);
        H5Sclose(mspace_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a small amount of data can be
 * read back from a dataset using a point selection and
 * then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_small_point_selection(void)
{
    hsize_t  points[DATASET_SMALL_READ_TEST_POINT_SELECTION_NUM_POINTS * DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK] = { 10, 10, 10 };
    hsize_t  mspace_dims[] = { DATASET_SMALL_READ_TEST_POINT_SELECTION_NUM_POINTS };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    hid_t    mspace_id = -1;
    void    *data = NULL;

    TESTING("small read from dataset w/ point selection")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR
    if ((mspace_id = H5Screate_simple(1, mspace_dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_NAME, DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    data_size = DATASET_SMALL_READ_TEST_POINT_SELECTION_NUM_POINTS * DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < DATASET_SMALL_READ_TEST_POINT_SELECTION_NUM_POINTS; i++) {
        size_t j;

        for (j = 0; j < DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK; j++)
            points[(i * DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK) + j] = i;
    }

    if (H5Sselect_elements(fspace_id, H5S_SELECT_SET, DATASET_SMALL_READ_TEST_POINT_SELECTION_NUM_POINTS, points) < 0) {
        H5_FAILED();
        printf("    couldn't select points\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Reading portion of small dataset using a point selection\n");
#endif

    if (H5Dread(dset_id, DATASET_SMALL_READ_TEST_POINT_SELECTION_DSET_DTYPE, mspace_id, fspace_id, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (H5Sclose(mspace_id) < 0)
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
        if (data) free(data);
        H5Sclose(mspace_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

#ifndef NO_LARGE_TESTS
/*
 * A test to check that a large amount of data can be
 * read back from a dataset using an H5S_ALL selection
 * and then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_large_all(void)
{
    hsize_t  dims[DATASET_LARGE_READ_TEST_ALL_DSET_SPACE_RANK] = { 600, 600, 600 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    void    *read_buf = NULL;

    TESTING("read from large dataset w/ H5S_ALL")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_LARGE_READ_TEST_ALL_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_LARGE_READ_TEST_ALL_DSET_NAME, DATASET_LARGE_READ_TEST_ALL_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < DATASET_LARGE_READ_TEST_ALL_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_LARGE_READ_TEST_ALL_DSET_DTYPESIZE;

    if (NULL == (read_buf = malloc(data_size)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Reading entirety of large dataset\n");
#endif

    if (H5Dread(dset_id, DATASET_LARGE_READ_TEST_ALL_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

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
        if (read_buf) free(read_buf);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a large amount of data can be
 * read back from a dataset using a hyperslab selection
 * and then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_large_hyperslab(void)
{
    hsize_t  start[DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  stride[DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  count[DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  block[DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK];
    hsize_t  dims[DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK] = { 600, 600, 600 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    mspace_id = -1, fspace_id = -1;
    void    *read_buf = NULL;

    TESTING("read from large dataset w/ hyperslab selection")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0){
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR
    if ((mspace_id = H5Screate_simple(DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_NAME, DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0; i < DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK; i++) {
        start[i] = 0;
        stride[i] = 1;
        count[i] = dims[i];
        block[i] = 1;
    }

    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, stride, count, block) < 0)
        TEST_ERROR

    for (i = 0, data_size = 1; i < DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_DTYPESIZE;

    if (NULL == (read_buf = malloc(data_size)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Reading portion of large dataset using hyperslab selection\n");
#endif

    if (H5Dread(dset_id, DATASET_LARGE_READ_TEST_HYPERSLAB_DSET_DTYPE, mspace_id, fspace_id, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

    if (H5Sclose(mspace_id) < 0)
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
        H5Sclose(mspace_id);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a large amount of data can be
 * read back from a dataset using a large point selection
 * and then verified.
 *
 * XXX: Add dataset write and data verification.
 */
static int
test_read_dataset_large_point_selection(void)
{
    hsize_t *points = NULL;
    hsize_t  dims[DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK] = { 600, 600, 600 };
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    void    *data = NULL;

    TESTING("read from large dataset w/ point selection")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_NAME, DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, data_size = 1; i < DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size = DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR
    if (NULL == (points = malloc(data_size / DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPESIZE)))
        TEST_ERROR

    /* Select the entire dataspace */
    for (i = 0; i < data_size / DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPESIZE; i++) {
        points[(i * DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK)] = (i % (dims[0] * dims[1])) % dims[1];
        points[(i * DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK) + 1] = (i % (dims[0] * dims[1])) / dims[0];
        points[(i * DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_SPACE_RANK) + 2] = (i / (dims[0] * dims[1]));
    }

    if (H5Sselect_elements(fspace_id, H5S_SELECT_SET, data_size / DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPESIZE, points) < 0) {
        H5_FAILED();
        printf("    couldn't select points\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Reading portion of large dataset using a point selection\n");
#endif

    if (H5Dread(dset_id, DATASET_LARGE_READ_TEST_POINT_SELECTION_DSET_DTYPE, H5S_ALL, fspace_id, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (points) {
        free(points);
        points = NULL;
    }

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
        if (data) free(data);
        if (points) free(points);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}
#endif

/*
 * A test to ensure that data is read back correctly from
 * a dataset after it has been written.
 */
static int
test_write_dataset_data_verification(void)
{
    hssize_t space_npoints;
    hsize_t  dims[DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK] = { 10, 10, 10 };
    hsize_t  start[DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK];
    hsize_t  stride[DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK];
    hsize_t  count[DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK];
    hsize_t  block[DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK];
    hsize_t  points[DATASET_DATA_VERIFY_WRITE_TEST_NUM_POINTS * DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK];
    size_t   i, data_size;
    hid_t    file_id = -1, fapl_id = -1;
    hid_t    container_group = -1;
    hid_t    dset_id = -1;
    hid_t    fspace_id = -1;
    void    *data = NULL;
    void    *write_buf = NULL;
    void    *read_buf = NULL;

    TESTING("verification of dataset data after write then read")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((fspace_id = H5Screate_simple(DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_DATA_VERIFY_WRITE_TEST_DSET_NAME, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing to dataset using H5S_ALL\n");
#endif

    for (i = 0, data_size = 1; i < DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE; i++)
        ((int *) data)[i] = (int) i;

    if (H5Dwrite(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(file_id, "/" DATASET_TEST_GROUP_NAME "/" DATASET_DATA_VERIFY_WRITE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((fspace_id = H5Dget_space(dset_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataset dataspace\n");
        goto error;
    }

    if ((space_npoints = H5Sget_simple_extent_npoints(fspace_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataspace num points\n");
        goto error;
    }

    if (NULL == (data = malloc((hsize_t) space_npoints * DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the data that comes back is correct after writing to entire dataset\n");
#endif

    if (H5Dread(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    for (i = 0; i < (hsize_t) space_npoints; i++)
        if (((int *) data)[i] != (int) i) {
            H5_FAILED();
            printf("    ALL selection data verification failed\n");
            goto error;
        }

    if (data) {
        free(data);
        data = NULL;
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing to dataset using hyperslab selection\n");
#endif

    data_size = dims[1] * 2 * DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE;

    if (NULL == (write_buf = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE; i++)
        ((int *) write_buf)[i] = 56;

    for (i = 0, data_size = 1; i < DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    if (H5Dread(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    for (i = 0; i < 2; i++) {
        size_t j;

        for (j = 0; j < dims[1]; j++)
            ((int *) data)[(i * dims[1] * dims[2]) + (j * dims[2])] = 56;
    }

    /* Write to first two rows of dataset */
    start[0] = start[1] = start[2] = 0;
    stride[0] = stride[1] = stride[2] = 1;
    count[0] = 2; count[1] = dims[1]; count[2] = 1;
    block[0] = block[1] = block[2] = 1;

    if (H5Sselect_hyperslab(fspace_id, H5S_SELECT_SET, start, stride, count, block) < 0)
        TEST_ERROR

    if (H5Dwrite(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, fspace_id, H5P_DEFAULT, write_buf) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(file_id, "/" DATASET_TEST_GROUP_NAME "/" DATASET_DATA_VERIFY_WRITE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((fspace_id = H5Dget_space(dset_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataset dataspace\n");
        goto error;
    }

    if ((space_npoints = H5Sget_simple_extent_npoints(fspace_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataspace num points\n");
        goto error;
    }

    if (NULL == (read_buf = malloc((hsize_t) space_npoints * DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the data that comes back is correct after writing to the dataset using a hyperslab selection\n");
#endif

    if (H5Dread(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (memcmp(data, read_buf, data_size)) {
        H5_FAILED();
        printf("    hyperslab selection data verification failed\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (write_buf) {
        free(write_buf);
        write_buf = NULL;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing to dataset using point selection\n");
#endif

    data_size = DATASET_DATA_VERIFY_WRITE_TEST_NUM_POINTS * DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE;

    if (NULL == (write_buf = malloc(data_size)))
        TEST_ERROR

    for (i = 0; i < data_size / DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE; i++)
        ((int *) write_buf)[i] = 13;

    for (i = 0, data_size = 1; i < DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK; i++)
        data_size *= dims[i];
    data_size *= DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE;

    if (NULL == (data = malloc(data_size)))
        TEST_ERROR

    if (H5Dread(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, data) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    for (i = 0; i < dims[0]; i++) {
        size_t j;

        for (j = 0; j < dims[1]; j++) {
            size_t k;

            for (k = 0; k < dims[2]; k++) {
                if (i == j && j == k)
                    ((int *) data)[(i * dims[1] * dims[2]) + (j * dims[2]) + k] = 13;
            }
        }
    }


    /* Select a series of 10 points in the dataset */
    for (i = 0; i < DATASET_DATA_VERIFY_WRITE_TEST_NUM_POINTS; i++) {
        size_t j;

        for (j = 0; j < DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK; j++)
            points[(i * DATASET_DATA_VERIFY_WRITE_TEST_DSET_SPACE_RANK) + j] = i;
    }

    if (H5Sselect_elements(fspace_id, H5S_SELECT_SET, DATASET_DATA_VERIFY_WRITE_TEST_NUM_POINTS, points) < 0)
        TEST_ERROR

    if (H5Dwrite(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, fspace_id, H5P_DEFAULT, write_buf) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(file_id, "/" DATASET_TEST_GROUP_NAME "/" DATASET_DATA_VERIFY_WRITE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((fspace_id = H5Dget_space(dset_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataset dataspace\n");
        goto error;
    }

    if ((space_npoints = H5Sget_simple_extent_npoints(fspace_id)) < 0) {
        H5_FAILED();
        printf("    couldn't get dataspace num points\n");
        goto error;
    }

    if (NULL == (read_buf = malloc((hsize_t) space_npoints * DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPESIZE)))
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Verifying that the data that comes back is correct after writing to dataset using point selection\n");
#endif

    if (H5Dread(dset_id, DATASET_DATA_VERIFY_WRITE_TEST_DSET_DTYPE, H5S_ALL, H5S_ALL, H5P_DEFAULT, read_buf) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    if (memcmp(data, read_buf, data_size)) {
        H5_FAILED();
        printf("    point selection data verification failed\n");
        goto error;
    }

    if (data) {
        free(data);
        data = NULL;
    }

    if (write_buf) {
        free(write_buf);
        write_buf = NULL;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }

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
        if (data) free(data);
        if (write_buf) free(write_buf);
        if (read_buf) free(read_buf);
        H5Sclose(fspace_id);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a dataset's extent can be changed
 * by using H5Dset_extent.
 */
static int
test_dataset_set_extent(void)
{
    hsize_t dims[DATASET_SET_EXTENT_TEST_SPACE_RANK];
    hsize_t new_dims[DATASET_SET_EXTENT_TEST_SPACE_RANK];
    size_t  i;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("set dataset extent")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < DATASET_SET_EXTENT_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);
    for (i = 0; i < DATASET_SET_EXTENT_TEST_SPACE_RANK; i++)
        new_dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_SET_EXTENT_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_SET_EXTENT_TEST_DSET_NAME, dset_dtype,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Testing use of H5Dset_extent to change dataset's extent\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Dset_extent(dset_id, new_dims);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Sclose(fspace_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a VOL connector stores and can
 * retrieve valid copies of a DAPL and DCPL used at
 * dataset access and dataset creation, respectively.
 */
static int
test_dataset_property_lists(void)
{
    const char *path_prefix = "/test_prefix";
    hsize_t     dims[DATASET_PROPERTY_LIST_TEST_SPACE_RANK];
    hsize_t     chunk_dims[DATASET_PROPERTY_LIST_TEST_SPACE_RANK];
    size_t      i;
    herr_t      err_ret = -1;
    hid_t       file_id = -1, fapl_id = -1;
    hid_t       container_group = -1, group_id = -1;
    hid_t       dset_id1 = -1, dset_id2 = -1, dset_id3 = -1, dset_id4 = -1;
    hid_t       dcpl_id1 = -1, dcpl_id2 = -1;
    hid_t       dapl_id1 = -1, dapl_id2 = -1;
    hid_t       dset_dtype1 = -1, dset_dtype2 = -1, dset_dtype3 = -1, dset_dtype4 = -1;
    hid_t       space_id = -1;
    char       *tmp_prefix = NULL;

    TESTING("dataset property list operations")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, DATASET_PROPERTY_LIST_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < DATASET_PROPERTY_LIST_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);
    for (i = 0; i < DATASET_PROPERTY_LIST_TEST_SPACE_RANK; i++)
        chunk_dims[i] = (hsize_t) (rand() % (int) dims[i] + 1);

    if ((space_id = H5Screate_simple(DATASET_PROPERTY_LIST_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype1 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((dset_dtype2 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((dset_dtype3 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR
    if ((dset_dtype4 = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dcpl_id1 = H5Pcreate(H5P_DATASET_CREATE)) < 0) {
        H5_FAILED();
        printf("    couldn't create DCPL\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Setting property on DCPL\n");
#endif

    if (H5Pset_chunk(dcpl_id1, DATASET_PROPERTY_LIST_TEST_SPACE_RANK, chunk_dims) < 0) {
        H5_FAILED();
        printf("    couldn't set DCPL property\n");
        goto error;
    }

    if ((dset_id1 = H5Dcreate2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME1, dset_dtype1,
            space_id, H5P_DEFAULT, dcpl_id1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if ((dset_id2 = H5Dcreate2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME2, dset_dtype2,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Pclose(dcpl_id1) < 0)
        TEST_ERROR

    /* Try to receive copies of the two property lists, one which has the property set and one which does not */
    if ((dcpl_id1 = H5Dget_create_plist(dset_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((dcpl_id2 = H5Dget_create_plist(dset_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Ensure that property list 1 has the property set and property list 2 does not */
    {
        hsize_t tmp_chunk_dims[DATASET_PROPERTY_LIST_TEST_SPACE_RANK];

        memset(tmp_chunk_dims, 0, sizeof(tmp_chunk_dims));

        if (H5Pget_chunk(dcpl_id1, DATASET_PROPERTY_LIST_TEST_SPACE_RANK, tmp_chunk_dims) < 0) {
            H5_FAILED();
            printf("    couldn't get DCPL property value\n");
            goto error;
        }

#ifdef VOL_TEST_DEBUG
    puts("Ensuring that the property on the DCPL was received back correctly\n");
#endif

        for (i = 0; i < DATASET_PROPERTY_LIST_TEST_SPACE_RANK; i++)
            if (tmp_chunk_dims[i] != chunk_dims[i]) {
                H5_FAILED();
                printf("    DCPL property values were incorrect\n");
                goto error;
            }

        H5E_BEGIN_TRY {
            err_ret = H5Pget_chunk(dcpl_id2, DATASET_PROPERTY_LIST_TEST_SPACE_RANK, tmp_chunk_dims);
        } H5E_END_TRY;

        if (err_ret >= 0) {
            H5_FAILED();
            printf("    property list 2 shouldn't have had chunk dimensionality set (not a chunked layout)\n");
            goto error;
        }
    }

    if ((dapl_id1 = H5Pcreate(H5P_DATASET_ACCESS)) < 0) {
        H5_FAILED();
        printf("    couldn't create DAPL\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Setting property on DAPL\n");
#endif

    if (H5Pset_efile_prefix(dapl_id1, path_prefix) < 0) {
        H5_FAILED();
        printf("    couldn't set DAPL property\n");
        goto error;
    }

    if ((dset_id3 = H5Dcreate2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME3, dset_dtype3,
            space_id, H5P_DEFAULT, H5P_DEFAULT, dapl_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if ((dset_id4 = H5Dcreate2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME4, dset_dtype4,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Pclose(dapl_id1) < 0)
        TEST_ERROR

    /* Try to receive copies of the two property lists, one which has the property set and one which does not */
    if ((dapl_id1 = H5Dget_access_plist(dset_id3)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((dapl_id2 = H5Dget_access_plist(dset_id4)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    /* Ensure that property list 1 has the property set and property list 2 does not */
    {
        ssize_t buf_size = 0;

#ifdef VOL_TEST_DEBUG
    puts("Ensuring that the property on the DAPL was received back correctly\n");
#endif

        if ((buf_size = H5Pget_efile_prefix(dapl_id1, NULL, 0)) < 0) {
            H5_FAILED();
            printf("    couldn't retrieve size for property value buffer\n");
            goto error;
        }

        if (NULL == (tmp_prefix = (char *) calloc(1, (size_t) buf_size + 1)))
            TEST_ERROR

        if (H5Pget_efile_prefix(dapl_id1, tmp_prefix, (size_t) buf_size + 1) < 0) {
            H5_FAILED();
            printf("    couldn't retrieve property list value\n");
            goto error;
        }

        if (strcmp(tmp_prefix, path_prefix)) {
            H5_FAILED();
            printf("    DAPL values were incorrect!\n");
            goto error;
        }

        memset(tmp_prefix, 0, (size_t) buf_size + 1);

        if (H5Pget_efile_prefix(dapl_id2, tmp_prefix, (size_t) buf_size) < 0) {
            H5_FAILED();
            printf("    couldn't retrieve property list value\n");
            goto error;
        }

        if (!strcmp(tmp_prefix, path_prefix)) {
            H5_FAILED();
            printf("    DAPL property value was set!\n");
            goto error;
        }
    }

    /* Now close the property lists and datasets and see if we can still retrieve copies of
     * the property lists upon opening (instead of creating) a dataset
     */
    if (H5Pclose(dcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(dcpl_id2) < 0)
        TEST_ERROR
    if (H5Pclose(dapl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(dapl_id2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id1) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id3) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id4) < 0)
        TEST_ERROR

    if ((dset_id1 = H5Dopen2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME1, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((dset_id2 = H5Dopen2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME2, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((dset_id3 = H5Dopen2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME3, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((dset_id4 = H5Dopen2(group_id, DATASET_PROPERTY_LIST_TEST_DSET_NAME4, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

    if ((dcpl_id1 = H5Dget_create_plist(dset_id1)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((dcpl_id2 = H5Dget_create_plist(dset_id2)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((dapl_id1 = H5Dget_access_plist(dset_id3)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if ((dapl_id2 = H5Dget_create_plist(dset_id4)) < 0) {
        H5_FAILED();
        printf("    couldn't get property list\n");
        goto error;
    }

    if (tmp_prefix) {
        free(tmp_prefix);
        tmp_prefix = NULL;
    }

    if (H5Pclose(dcpl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(dcpl_id2) < 0)
        TEST_ERROR
    if (H5Pclose(dapl_id1) < 0)
        TEST_ERROR
    if (H5Pclose(dapl_id2) < 0)
        TEST_ERROR
    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype1) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype2) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype3) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype4) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id1) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id2) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id3) < 0)
        TEST_ERROR
    if (H5Dclose(dset_id4) < 0)
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
        free(tmp_prefix);
        H5Pclose(dcpl_id1);
        H5Pclose(dcpl_id2);
        H5Pclose(dapl_id1);
        H5Pclose(dapl_id2);
        H5Sclose(space_id);
        H5Tclose(dset_dtype1);
        H5Tclose(dset_dtype2);
        H5Tclose(dset_dtype3);
        H5Tclose(dset_dtype4);
        H5Dclose(dset_id1);
        H5Dclose(dset_id2);
        H5Dclose(dset_id3);
        H5Dclose(dset_id4);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_unused_dataset_API_calls(void)
{
    H5D_space_status_t allocation;
    hsize_t            dims[DATASET_UNUSED_APIS_TEST_SPACE_RANK];
    hsize_t            fake_storage_size;
    haddr_t            offset = HADDR_UNDEF;
    herr_t             err_ret = -1;
    size_t             i;
    hid_t              file_id = -1, fapl_id = -1;
    hid_t              container_group = -1;
    hid_t              dset_id = -1;
    hid_t              dset_dtype = -1;
    hid_t              fspace_id = -1;

    TESTING("unused dataset API calls")

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

    if ((container_group = H5Gopen2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < DATASET_UNUSED_APIS_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(DATASET_UNUSED_APIS_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, DATASET_UNUSED_APIS_TEST_DSET_NAME, dset_dtype,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Testing that all of the unused dataset API calls don't cause application issues\n");
#endif

    H5E_BEGIN_TRY {
        fake_storage_size = H5Dget_storage_size(dset_id);
        err_ret = fake_storage_size;
    } H5E_END_TRY;

    if (err_ret > 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Dget_space_status(dset_id, &allocation);
    } H5E_END_TRY;

    if (err_ret > 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        offset = H5Dget_offset(dset_id);
    } H5E_END_TRY;
    if (offset != HADDR_UNDEF) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Sclose(fspace_id);
        H5Tclose(dset_dtype);
        H5Dclose(dset_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

int
vol_dataset_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_LENGTH(dataset_tests); i++) {
        nerrors += (*dataset_tests[i])() ? 1 : 0;
    }

done:
    return nerrors;
}
