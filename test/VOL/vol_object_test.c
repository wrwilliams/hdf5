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

#include "vol_object_test.h"

/*
 * XXX: Difficult to implement right now.
 */
#define NO_REF_TESTS

static int test_open_dataset_generically(void);
static int test_open_group_generically(void);
static int test_open_datatype_generically(void);
static int test_object_exists(void);
static int test_incr_decr_refcount(void);
static int test_h5o_copy(void);
static int test_h5o_close(void);
static int test_object_visit(void);
#ifndef NO_REF_TESTS
static int test_create_obj_ref(void);
static int test_dereference_reference(void);
static int test_get_ref_type(void);
static int test_get_ref_name(void);
static int test_get_region(void);
static int test_write_dataset_w_obj_refs(void);
static int test_read_dataset_w_obj_refs(void);
static int test_write_dataset_w_obj_refs_empty_data(void);
#endif
static int test_unused_object_API_calls(void);

static herr_t object_visit_callback(hid_t o_id, const char *name, const H5O_info_t *object_info, void *op_data);

/*
 * The array of object tests to be performed.
 */
static int (*object_tests[])(void) = {
        test_open_dataset_generically,
        test_open_group_generically,
        test_open_datatype_generically,
        test_object_exists,
        test_incr_decr_refcount,
        test_h5o_copy,
        test_h5o_close,
        test_object_visit,
#ifndef NO_REF_TESTS
        test_create_obj_ref,
        test_dereference_reference,
        test_get_ref_type,
        test_get_ref_name,
        test_get_region,
        test_write_dataset_w_obj_refs,
        test_read_dataset_w_obj_refs,
        test_write_dataset_w_obj_refs_empty_data,
#endif
        test_unused_object_API_calls,
};

/*
 * A test to check that a dataset can be opened by
 * using H5Oopen, H5Oopen_by_idx and H5Oopen_by_addr.
 */
static int
test_open_dataset_generically(void)
{
    hsize_t dims[GENERIC_DATASET_OPEN_TEST_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;
    hid_t   fspace_id = -1;

    TESTING("open dataset generically w/ H5Oopen()")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < GENERIC_DATASET_OPEN_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(GENERIC_DATASET_OPEN_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, GENERIC_DATASET_OPEN_TEST_DSET_NAME, dset_dtype,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Opening dataset with H5Oopen\n");
#endif

    if ((dset_id = H5Oopen(file_id, "/" OBJECT_TEST_GROUP_NAME "/" GENERIC_DATASET_OPEN_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset with H5Oopen()\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Opening dataset with H5Oopen_by_idx\n");
#endif

#ifndef PROBLEMATIC_TESTS
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        dset_id = H5Oopen_by_idx(file_id, "/" OBJECT_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (dset_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }
#endif

#ifdef VOL_TEST_DEBUG
    puts("Opening dataset with H5Oopen_by_addr\n");
#endif

    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        dset_id = H5Oopen_by_addr(file_id, 0);
    } H5E_END_TRY;

    if (dset_id >= 0) {
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
 * A test to check that a group can be opened by
 * using H5Oopen, H5Oopen_by_idx and H5Oopen_by_addr.
 */
static int
test_open_group_generically(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t container_group = -1;
    hid_t group_id = -1;

    TESTING("open group generically w/ H5Oopen()")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, GENERIC_GROUP_OPEN_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create group\n");
        goto error;
    }

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Opening group with H5Oopen\n");
#endif

    if ((group_id = H5Oopen(file_id, "/" OBJECT_TEST_GROUP_NAME "/" GENERIC_GROUP_OPEN_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group with H5Oopen()\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Opening group with H5Oopen_by_idx\n");
#endif

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        group_id = H5Oopen_by_idx(file_id, "/" OBJECT_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (group_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Opening group with H5Oopen_by_addr\n");
#endif

    if (H5Gclose(group_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        group_id = H5Oopen_by_addr(file_id, 0);
    } H5E_END_TRY;

    if (group_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
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
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a committed datatype can be
 * opened by using H5Oopen, H5Oopen_by_idx and H5Oopen_by_addr.
 */
static int
test_open_datatype_generically(void)
{
    hid_t file_id = -1, fapl_id = -1;
    hid_t container_group = -1;
    hid_t type_id = -1;

    TESTING("open datatype generically w/ H5Oopen()")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((type_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, GENERIC_DATATYPE_OPEN_TEST_TYPE_NAME, type_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    if (H5Tclose(type_id) < 0)
        TEST_ERROR

#ifdef VOL_TEST_DEBUG
    puts("Opening datatype with H5Oopen\n");
#endif

    if ((type_id = H5Oopen(file_id, "/" OBJECT_TEST_GROUP_NAME "/" GENERIC_DATATYPE_OPEN_TEST_TYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open datatype generically w/ H5Oopen()\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Opening datatype with H5Oopen_by_idx\n");
#endif

    if (H5Tclose(type_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        type_id = H5Oopen_by_idx(file_id, "/" OBJECT_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, 0, H5P_DEFAULT);
    } H5E_END_TRY;

    if (type_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Opening datatype with H5Oopen_by_addr\n");
#endif

    if (H5Tclose(type_id) < 0)
        TEST_ERROR

    H5E_BEGIN_TRY {
        type_id = H5Oopen_by_addr(file_id, 0);
    } H5E_END_TRY;

    if (type_id >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
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
 * A test to check that a group, dataset and committed
 * datatype exist by using H5Oexists_by_name.
 */
static int
test_object_exists(void)
{
    hsize_t dims[OBJECT_EXISTS_TEST_DSET_SPACE_RANK];
    size_t  i;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id = -1;
    hid_t   dtype_id = -1;
    hid_t   fspace_id = -1;
    hid_t   dset_dtype = -1;

    TESTING("object exists by name")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJECT_EXISTS_TEST_SUBGROUP_NAME,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    if ((dtype_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
    }

    if (H5Tcommit2(group_id, OBJECT_EXISTS_TEST_DTYPE_NAME, dtype_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }

    for (i = 0; i < OBJECT_EXISTS_TEST_DSET_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(OBJECT_EXISTS_TEST_DSET_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, OBJECT_EXISTS_TEST_DSET_NAME, dset_dtype, fspace_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    /* H5Oexists_by_name for hard links should always succeed. */
    /* Checking for a soft link may fail if the link doesn't resolve. */

    H5E_BEGIN_TRY {
        err_ret = H5Oexists_by_name(file_id, OBJECT_TEST_GROUP_NAME "/" OBJECT_EXISTS_TEST_SUBGROUP_NAME, H5P_DEFAULT);
    } H5E_END_TRY;

    /* Check if the group exists */
    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Oexists_by_name(file_id, OBJECT_TEST_GROUP_NAME "/" OBJECT_EXISTS_TEST_SUBGROUP_NAME "/" OBJECT_EXISTS_TEST_DTYPE_NAME,
                H5P_DEFAULT);
    } H5E_END_TRY;

    /* Check if the datatype exists */
    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Oexists_by_name(file_id, OBJECT_TEST_GROUP_NAME "/" OBJECT_EXISTS_TEST_SUBGROUP_NAME "/" OBJECT_EXISTS_TEST_DSET_NAME,
                H5P_DEFAULT);
    } H5E_END_TRY;

    /* Check if the dataset exists */
    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(dtype_id) < 0)
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
        H5Tclose(dtype_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of H5Oincr_refcount
 * and H5Odecr_refcount.
 */
static int
test_incr_decr_refcount(void)
{
    herr_t err_ret = -1;
    hid_t  file_id = -1, fapl_id = -1;

    TESTING("H5Oincr/decr_refcount")

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
    puts("Testing unsupported APIs H5Oincr/decr_refcount\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Oincr_refcount(file_id);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded!\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Odecr_refcount(file_id);
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

/*
 * A test to check that a group, dataset and committed
 * datatype can each be copied by using H5Ocopy.
 */
static int
test_h5o_copy(void)
{
    hsize_t dims[OBJECT_COPY_TEST_SPACE_RANK];
    size_t  i;
    herr_t  err_ret = -1;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   dset_id = -1;
    hid_t   space_id = -1;

    TESTING("object copy")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJECT_COPY_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container subgroup\n");
        goto error;
    }

    for (i = 0; i < OBJECT_COPY_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((space_id = H5Screate_simple(OBJECT_COPY_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, OBJECT_COPY_TEST_DSET_NAME, OBJECT_COPY_TEST_DSET_DTYPE, space_id,
            H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Copying object with H5Ocopy\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Ocopy(group_id, OBJECT_COPY_TEST_DSET_NAME, group_id, OBJECT_COPY_TEST_DSET_NAME2, H5P_DEFAULT, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    if (H5Sclose(space_id) < 0)
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
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check that a group, dataset and committed
 * datatype can each be closed by using H5Oclose.
 */
static int
test_h5o_close(void)
{
    hsize_t dims[H5O_CLOSE_TEST_SPACE_RANK];
    size_t  i;
    hid_t   file_id = -1, fapl_id = -1;
    hid_t   container_group = -1, group_id = -1;
    hid_t   fspace_id = -1;
    hid_t   dtype_id = -1;
    hid_t   dset_id = -1;
    hid_t   dset_dtype = -1;

    TESTING("H5Oclose")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    for (i = 0; i < H5O_CLOSE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % MAX_DIM_SIZE + 1);

    if ((fspace_id = H5Screate_simple(H5O_CLOSE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(container_group, H5O_CLOSE_TEST_DSET_NAME, dset_dtype,
            fspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    if ((dtype_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(container_group, H5O_CLOSE_TEST_TYPE_NAME, dtype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't commit datatype\n");
        goto error;
    }


    if (H5Dclose(dset_id) < 0)
        TEST_ERROR
    if (H5Tclose(dtype_id) < 0)
        TEST_ERROR

    if ((group_id = H5Oopen(file_id, "/", H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open group with H5Oopen()\n");
        goto error;
    }

    if ((dset_id = H5Oopen(file_id, "/" OBJECT_TEST_GROUP_NAME "/" H5O_CLOSE_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset with H5Oopen()\n");
        goto error;
    }

    if ((dtype_id = H5Oopen(file_id, "/" OBJECT_TEST_GROUP_NAME "/" H5O_CLOSE_TEST_TYPE_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open datatype with H5Oopen()\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Making sure H5Oclose does its job correctly\n");
#endif

    if (H5Oclose(group_id) < 0)
        TEST_ERROR
    if (H5Oclose(dtype_id) < 0)
        TEST_ERROR
    if (H5Oclose(dset_id) < 0)
        TEST_ERROR

    if (H5Sclose(fspace_id) < 0)
        TEST_ERROR
    if (H5Tclose(dset_dtype) < 0)
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
        H5Tclose(dtype_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

/*
 * A test to check the functionality of H5Ovisit and
 * H5Ovisit_by_name for groups, datasets and committed
 * datatypes.
 */
static int
test_object_visit(void)
{
    herr_t err_ret = -1;
    hid_t  file_id = -1, fapl_id = -1;
    hid_t  container_group = -1;

    TESTING("H5Ovisit")

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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Visiting objects with H5Ovisit\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Ovisit2(container_group, H5_INDEX_NAME, H5_ITER_INC, object_visit_callback, NULL, H5O_INFO_ALL);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Visiting objects with H5Ovisit_by_name\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Ovisit_by_name2(file_id, "/" OBJECT_TEST_GROUP_NAME, H5_INDEX_NAME, H5_ITER_INC, object_visit_callback, NULL, H5O_INFO_ALL, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
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

#ifndef NO_REF_TESTS
static int
test_create_obj_ref(void)
{
    vol_test_obj_ref_t ref;
    hid_t              file_id = -1, fapl_id = -1;

    TESTING("create an object reference")

    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
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
    puts("Creating an object reference\n");
#endif

    if (H5Rcreate((void *) &ref, file_id, "/", H5R_OBJECT, -1) < 0) {
        H5_FAILED();
        printf("    couldn't create obj. ref\n");
        goto error;
    }

    if (H5R_OBJECT != ref.ref_type) TEST_ERROR
    if (H5I_GROUP != ref.ref_obj_type) TEST_ERROR

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
test_dereference_reference(void)
{
    TESTING("dereference a reference")

    /* H5Rdereference2 */

    SKIPPED();

    return 0;
}

static int
test_get_ref_type(void)
{
    vol_test_obj_ref_t ref_array[3];
    H5O_type_t         obj_type;
    hsize_t            dims[OBJ_REF_GET_TYPE_TEST_SPACE_RANK];
    size_t             i;
    hid_t              file_id = -1, fapl_id = -1;
    hid_t              container_group = -1, group_id = -1;
    hid_t              ref_dset_id = -1;
    hid_t              ref_dtype_id = -1;
    hid_t              ref_dset_dtype = -1;
    hid_t              space_id = -1;

    TESTING("retrieve type of object reference by an object/region reference")

    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJ_REF_GET_TYPE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < OBJ_REF_GET_TYPE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % 8 + 1);

    if ((space_id = H5Screate_simple(OBJ_REF_GET_TYPE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((ref_dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    /* Create the dataset and datatype which will be referenced */
    if ((ref_dset_id = H5Dcreate2(group_id, OBJ_REF_GET_TYPE_TEST_DSET_NAME, ref_dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset for referencing\n");
        goto error;
    }

    if ((ref_dtype_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(group_id, OBJ_REF_GET_TYPE_TEST_TYPE_NAME, ref_dtype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype for referencing\n");
        goto error;
    }

    {
        /* TODO: Temporary workaround for datatypes */
        if (H5Tclose(ref_dtype_id) < 0)
            TEST_ERROR

        if ((ref_dtype_id = H5Topen2(group_id, OBJ_REF_GET_TYPE_TEST_TYPE_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open datatype for referencing\n");
            goto error;
        }
    }


    /* Create and check the group reference */
    if (H5Rcreate(&ref_array[0], file_id, "/", H5R_OBJECT, -1) < 0) {
        H5_FAILED();
        printf("    couldn't create group object reference\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving the type of the referenced object for this reference\n");
#endif

    if (H5Rget_obj_type2(file_id, H5R_OBJECT, &ref_array[0], &obj_type) < 0) {
        H5_FAILED();
        printf("    couldn't get object reference's object type\n");
        goto error;
    }

    if (H5O_TYPE_GROUP != obj_type) {
        H5_FAILED();
        printf("    referenced object was not a group\n");
        goto error;
    }

    /* Create and check the datatype reference */
    if (H5Rcreate(&ref_array[1], group_id, OBJ_REF_GET_TYPE_TEST_TYPE_NAME, H5R_OBJECT, -1) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype object reference\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving the type of the referenced object for this reference\n");
#endif

    if (H5Rget_obj_type2(file_id, H5R_OBJECT, &ref_array[1], &obj_type) < 0) {
        H5_FAILED();
        printf("    couldn't get object reference's object type\n");
        goto error;
    }

    if (H5O_TYPE_NAMED_DATATYPE != obj_type) {
        H5_FAILED();
        printf("    referenced object was not a datatype\n");
        goto error;
    }

    /* Create and check the dataset reference */
    if (H5Rcreate(&ref_array[2], group_id, OBJ_REF_GET_TYPE_TEST_DSET_NAME, H5R_OBJECT, -1) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset object reference\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Retrieving the type of the referenced object for this reference\n");
#endif

    if (H5Rget_obj_type2(file_id, H5R_OBJECT, &ref_array[2], &obj_type) < 0) {
        H5_FAILED();
        printf("    couldn't get object reference's object type\n");
        goto error;
    }

    if (H5O_TYPE_DATASET != obj_type) {
        H5_FAILED();
        printf("    referenced object was not a dataset\n");
        goto error;
    }

    /* TODO: Support for region references in this test */


    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dtype_id) < 0)
        TEST_ERROR
    if (H5Dclose(ref_dset_id) < 0)
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
        H5Tclose(ref_dset_dtype);
        H5Tclose(ref_dtype_id);
        H5Dclose(ref_dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_get_ref_name(void)
{
    TESTING("get ref. name")

    /* H5Rget_name */

    SKIPPED();

    return 0;
}

static int
test_get_region(void)
{
    TESTING("get region for region reference")

    /* H5Rget_region */

    SKIPPED();

    return 0;
}

static int
test_write_dataset_w_obj_refs(void)
{
    vol_test_obj_ref_t *ref_array = NULL;
    hsize_t             dims[OBJ_REF_DATASET_WRITE_TEST_SPACE_RANK];
    size_t              i, ref_array_size = 0;
    hid_t               file_id = -1, fapl_id = -1;
    hid_t               container_group = -1, group_id = -1;
    hid_t               dset_id = -1, ref_dset_id = -1;
    hid_t               ref_dtype_id = -1;
    hid_t               ref_dset_dtype = -1;
    hid_t               space_id = -1;

    TESTING("write to a dataset w/ object reference type")

    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJ_REF_DATASET_WRITE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < OBJ_REF_DATASET_WRITE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % 8 + 1);

    if ((space_id = H5Screate_simple(OBJ_REF_DATASET_WRITE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((ref_dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    /* Create the dataset and datatype which will be referenced */
    if ((ref_dset_id = H5Dcreate2(group_id, OBJ_REF_DATASET_WRITE_TEST_REF_DSET_NAME, ref_dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset for referencing\n");
        goto error;
    }

    if ((ref_dtype_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(group_id, OBJ_REF_DATASET_WRITE_TEST_REF_TYPE_NAME, ref_dtype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype for referencing\n");
        goto error;
    }

    {
        /* TODO: Temporary workaround for datatypes */
        if (H5Tclose(ref_dtype_id) < 0)
            TEST_ERROR

        if ((ref_dtype_id = H5Topen2(group_id, OBJ_REF_DATASET_WRITE_TEST_REF_TYPE_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open datatype for referencing\n");
            goto error;
        }
    }


    if ((dset_id = H5Dcreate2(group_id, OBJ_REF_DATASET_WRITE_TEST_DSET_NAME, H5T_STD_REF_OBJ,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, ref_array_size = 1; i < OBJ_REF_DATASET_WRITE_TEST_SPACE_RANK; i++)
        ref_array_size *= dims[i];

    if (NULL == (ref_array = (vol_test_obj_ref_t *) malloc(ref_array_size * sizeof(*ref_array))))
        TEST_ERROR

    for (i = 0; i < dims[0]; i++) {
        /* Create a reference to either a group, datatype or dataset */
        switch (rand() % 3) {
            case 0:
                if (H5Rcreate(&ref_array[i], file_id, "/", H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            case 1:
                if (H5Rcreate(&ref_array[i], group_id, OBJ_REF_DATASET_WRITE_TEST_REF_TYPE_NAME, H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            case 2:
                if (H5Rcreate(&ref_array[i], group_id, OBJ_REF_DATASET_WRITE_TEST_REF_DSET_NAME, H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            default:
                TEST_ERROR
        }
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing to dataset with buffer of object references\n");
#endif

    if (H5Dwrite(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_array) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (ref_array) {
        free(ref_array);
        ref_array = NULL;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dtype_id) < 0)
        TEST_ERROR
    if (H5Dclose(ref_dset_id) < 0)
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
        if (ref_array) free(ref_array);
        H5Sclose(space_id);
        H5Tclose(ref_dset_dtype);
        H5Tclose(ref_dtype_id);
        H5Dclose(ref_dset_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_read_dataset_w_obj_refs(void)
{
    vol_test_obj_ref_t *ref_array = NULL;
    hsize_t             dims[OBJ_REF_DATASET_READ_TEST_SPACE_RANK];
    size_t              i, ref_array_size = 0;
    hid_t               file_id = -1, fapl_id = -1;
    hid_t               container_group = -1, group_id = -1;
    hid_t               dset_id = -1, ref_dset_id = -1;
    hid_t               ref_dtype_id = -1;
    hid_t               ref_dset_dtype = -1;
    hid_t               space_id = -1;

    TESTING("read from a dataset w/ object reference type")

    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJ_REF_DATASET_READ_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group\n");
        goto error;
    }

    for (i = 0; i < OBJ_REF_DATASET_READ_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % 8 + 1);

    if ((space_id = H5Screate_simple(OBJ_REF_DATASET_READ_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((ref_dset_dtype = generate_random_datatype(H5T_NO_CLASS)) < 0)
        TEST_ERROR

    /* Create the dataset and datatype which will be referenced */
    if ((ref_dset_id = H5Dcreate2(group_id, OBJ_REF_DATASET_READ_TEST_REF_DSET_NAME, ref_dset_dtype,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset for referencing\n");
        goto error;
    }

    if ((ref_dtype_id = generate_random_datatype(H5T_NO_CLASS)) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype\n");
        goto error;
    }

    if (H5Tcommit2(group_id, OBJ_REF_DATASET_READ_TEST_REF_TYPE_NAME, ref_dtype_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT) < 0) {
        H5_FAILED();
        printf("    couldn't create datatype for referencing\n");
        goto error;
    }

    {
        /* TODO: Temporary workaround for datatypes */
        if (H5Tclose(ref_dtype_id) < 0)
            TEST_ERROR

        if ((ref_dtype_id = H5Topen2(group_id, OBJ_REF_DATASET_READ_TEST_REF_TYPE_NAME, H5P_DEFAULT)) < 0) {
            H5_FAILED();
            printf("    couldn't open datatype for referencing\n");
            goto error;
        }
    }


    if ((dset_id = H5Dcreate2(group_id, OBJ_REF_DATASET_READ_TEST_DSET_NAME, H5T_STD_REF_OBJ,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, ref_array_size = 1; i < OBJ_REF_DATASET_READ_TEST_SPACE_RANK; i++)
        ref_array_size *= dims[i];

    if (NULL == (ref_array = (vol_test_obj_ref_t *) malloc(ref_array_size * sizeof(*ref_array))))
        TEST_ERROR

    for (i = 0; i < dims[0]; i++) {
        /* Create a reference to either a group, datatype or dataset */
        switch (rand() % 3) {
            case 0:
                if (H5Rcreate(&ref_array[i], file_id, "/", H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            case 1:
                if (H5Rcreate(&ref_array[i], group_id, OBJ_REF_DATASET_READ_TEST_REF_TYPE_NAME, H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            case 2:
                if (H5Rcreate(&ref_array[i], group_id, OBJ_REF_DATASET_READ_TEST_REF_DSET_NAME, H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            default:
                TEST_ERROR
        }
    }

    if (H5Dwrite(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_array) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    /* Now read from the dataset */
    if (H5Dclose(dset_id) < 0)
        TEST_ERROR

    if ((dset_id = H5Dopen2(group_id, OBJ_REF_DATASET_READ_TEST_DSET_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open dataset\n");
        goto error;
    }

#ifdef VOL_TEST_DEBUG
    puts("Reading from dataset with object reference type\n");
#endif

    memset(ref_array, 0, ref_array_size * sizeof(*ref_array));

    if (H5Dread(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_array) < 0) {
        H5_FAILED();
        printf("    couldn't read from dataset\n");
        goto error;
    }

    for (i = 0; i < dims[0]; i++) {
        /* Check the reference type */
        if (H5R_OBJECT != ref_array[i].ref_type) {
            H5_FAILED();
            printf("    ref type was not H5R_OBJECT\n");
            goto error;
        }

        /* Check the object type referenced */
        if (   H5I_FILE != ref_array[i].ref_obj_type
            && H5I_GROUP != ref_array[i].ref_obj_type
            && H5I_DATATYPE != ref_array[i].ref_obj_type
            && H5I_DATASET != ref_array[i].ref_obj_type
           ) {
            H5_FAILED();
            printf("    ref object type mismatch\n");
            goto error;
        }

        /* Check the URI of the referenced object according to
         * the server spec where each URI is prefixed as
         * 'X-', where X is a character denoting the type
         * of object */
        if (   (ref_array[i].ref_obj_URI[1] != '-')
            || (ref_array[i].ref_obj_URI[0] != 'g'
            &&  ref_array[i].ref_obj_URI[0] != 't'
            &&  ref_array[i].ref_obj_URI[0] != 'd')
           ) {
            H5_FAILED();
            printf("    ref URI mismatch\n");
            goto error;
        }
    }

    if (ref_array) {
        free(ref_array);
        ref_array = NULL;
    }

    if (H5Sclose(space_id) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dset_dtype) < 0)
        TEST_ERROR
    if (H5Tclose(ref_dtype_id) < 0)
        TEST_ERROR
    if (H5Dclose(ref_dset_id) < 0)
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
        if (ref_array) free(ref_array);
        H5Sclose(space_id);
        H5Tclose(ref_dset_dtype);
        H5Tclose(ref_dtype_id);
        H5Dclose(ref_dset_id);
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}

static int
test_write_dataset_w_obj_refs_empty_data(void)
{
    vol_test_obj_ref_t *ref_array = NULL;
    hsize_t             dims[OBJ_REF_DATASET_EMPTY_WRITE_TEST_SPACE_RANK];
    size_t              i, ref_array_size = 0;
    hid_t               file_id = -1, fapl_id = -1;
    hid_t               container_group = -1, group_id = -1;
    hid_t               dset_id = -1;
    hid_t               space_id = -1;

    TESTING("write to a dataset w/ object reference type and some empty data")

    if ((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
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

    if ((container_group = H5Gopen2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't open container group\n");
        goto error;
    }

    if ((group_id = H5Gcreate2(container_group, OBJ_REF_DATASET_EMPTY_WRITE_TEST_SUBGROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create container sub-group");
        goto error;
    }

    for (i = 0; i < OBJ_REF_DATASET_EMPTY_WRITE_TEST_SPACE_RANK; i++)
        dims[i] = (hsize_t) (rand() % 8 + 1);

    if ((space_id = H5Screate_simple(OBJ_REF_DATASET_EMPTY_WRITE_TEST_SPACE_RANK, dims, NULL)) < 0)
        TEST_ERROR

    if ((dset_id = H5Dcreate2(group_id, OBJ_REF_DATASET_EMPTY_WRITE_TEST_DSET_NAME, H5T_STD_REF_OBJ,
            space_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        H5_FAILED();
        printf("    couldn't create dataset\n");
        goto error;
    }

    for (i = 0, ref_array_size = 1; i < OBJ_REF_DATASET_EMPTY_WRITE_TEST_SPACE_RANK; i++)
        ref_array_size *= dims[i];

    if (NULL == (ref_array = (vol_test_obj_ref_t *) calloc(1, ref_array_size * sizeof(*ref_array))))
        TEST_ERROR

    for (i = 0; i < dims[0]; i++) {
        switch (rand() % 2) {
            case 0:
                if (H5Rcreate(&ref_array[i], file_id, "/", H5R_OBJECT, -1) < 0) {
                    H5_FAILED();
                    printf("    couldn't create reference\n");
                    goto error;
                }

                break;

            case 1:
                break;

            default:
                TEST_ERROR
        }
    }

#ifdef VOL_TEST_DEBUG
    puts("Writing to dataset with buffer of empty object references\n");
#endif

    if (H5Dwrite(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, ref_array) < 0) {
        H5_FAILED();
        printf("    couldn't write to dataset\n");
        goto error;
    }

    if (ref_array) {
        free(ref_array);
        ref_array = NULL;
    }

    if (H5Sclose(space_id) < 0)
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
        H5Dclose(dset_id);
        H5Gclose(group_id);
        H5Gclose(container_group);
        H5Pclose(fapl_id);
        H5Fclose(file_id);
    } H5E_END_TRY;

    return 1;
}
#endif

static int
test_unused_object_API_calls(void)
{
    const char *comment = "comment";
    herr_t      err_ret = -1;
    hid_t       file_id = -1, fapl_id = -1;

    TESTING("unused object API calls")

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
    puts("Testing that all of the unused object API calls don't cause application issues\n");
#endif

    H5E_BEGIN_TRY {
        err_ret = H5Oset_comment(file_id, comment);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
        goto error;
    }

    H5E_BEGIN_TRY {
        err_ret = H5Oset_comment_by_name(file_id, "/", comment, H5P_DEFAULT);
    } H5E_END_TRY;

    if (err_ret >= 0) {
        H5_FAILED();
        printf("    unsupported API succeeded\n");
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
 * H5Ovisit callback to simply iterate through all of the objects in a given
 * group.
 */
static herr_t
object_visit_callback(hid_t o_id, const char *name, const H5O_info_t *object_info, void *op_data)
{
    return 0;
}

int
vol_object_test(void)
{
    size_t i;
    int    nerrors = 0;

    for (i = 0; i < ARRAY_LENGTH(object_tests); i++) {
        nerrors += (*object_tests[i])() ? 1 : 0;
    }

done:
    return nerrors;
}
