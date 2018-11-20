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

#ifndef VOL_OBJECT_TEST_H
#define VOL_OBJECT_TEST_H

#include "vol_test.h"

int vol_object_test(void);

/***********************************************
 *                                             *
 *      VOL connector Object test defines      *
 *                                             *
 ***********************************************/

#define GENERIC_DATASET_OPEN_TEST_SPACE_RANK 2
#define GENERIC_DATASET_OPEN_TEST_DSET_NAME  "generic_dataset_open_test"

#define GENERIC_GROUP_OPEN_TEST_GROUP_NAME "generic_group_open_test"

#define GENERIC_DATATYPE_OPEN_TEST_TYPE_NAME "generic_datatype_open_test"

#define OBJECT_EXISTS_TEST_DSET_SPACE_RANK 2
#define OBJECT_EXISTS_TEST_SUBGROUP_NAME   "h5o_exists_by_name_test"
#define OBJECT_EXISTS_TEST_DTYPE_NAME      "h5o_exists_by_name_dtype"
#define OBJECT_EXISTS_TEST_DSET_NAME       "h5o_exists_by_name_dset"

#define OBJECT_COPY_TEST_SUBGROUP_NAME "object_copy_test"
#define OBJECT_COPY_TEST_SPACE_RANK    2
#define OBJECT_COPY_TEST_DSET_DTYPE    H5T_NATIVE_INT
#define OBJECT_COPY_TEST_DSET_NAME     "dset"
#define OBJECT_COPY_TEST_DSET_NAME2    "dset_copy"

#define H5O_CLOSE_TEST_SPACE_RANK 2
#define H5O_CLOSE_TEST_DSET_NAME  "h5o_close_test_dset"
#define H5O_CLOSE_TEST_TYPE_NAME  "h5o_close_test_type"

#define OBJ_REF_GET_TYPE_TEST_SUBGROUP_NAME "obj_ref_get_obj_type_test"
#define OBJ_REF_GET_TYPE_TEST_DSET_NAME "ref_dset"
#define OBJ_REF_GET_TYPE_TEST_TYPE_NAME "ref_dtype"
#define OBJ_REF_GET_TYPE_TEST_SPACE_RANK 2

#define OBJ_REF_DATASET_WRITE_TEST_SUBGROUP_NAME  "obj_ref_write_test"
#define OBJ_REF_DATASET_WRITE_TEST_REF_DSET_NAME  "ref_dset"
#define OBJ_REF_DATASET_WRITE_TEST_REF_TYPE_NAME  "ref_dtype"
#define OBJ_REF_DATASET_WRITE_TEST_SPACE_RANK     1
#define OBJ_REF_DATASET_WRITE_TEST_DSET_NAME      "obj_ref_dset"

#define OBJ_REF_DATASET_READ_TEST_SUBGROUP_NAME  "obj_ref_read_test"
#define OBJ_REF_DATASET_READ_TEST_REF_DSET_NAME  "ref_dset"
#define OBJ_REF_DATASET_READ_TEST_REF_TYPE_NAME  "ref_dtype"
#define OBJ_REF_DATASET_READ_TEST_SPACE_RANK     1
#define OBJ_REF_DATASET_READ_TEST_DSET_NAME      "obj_ref_dset"

#define OBJ_REF_DATASET_EMPTY_WRITE_TEST_SUBGROUP_NAME  "obj_ref_empty_write_test"
#define OBJ_REF_DATASET_EMPTY_WRITE_TEST_SPACE_RANK     1
#define OBJ_REF_DATASET_EMPTY_WRITE_TEST_DSET_NAME      "obj_ref_dset"

#endif
