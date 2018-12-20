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

#ifndef VOL_GROUP_TEST_H
#define VOL_GROUP_TEST_H

#include "vol_test.h"

int vol_group_test(void);

/**********************************************
 *                                            *
 *      VOL connector Group test defines      *
 *                                            *
 **********************************************/

#define GROUP_CREATE_UNDER_ROOT_GNAME "/group_under_root"

#define GROUP_CREATE_UNDER_GROUP_REL_GNAME "group_under_group2"

#define GROUP_CREATE_INVALID_PARAMS_GROUP_NAME "/invalid_params_group"

#define GROUP_CREATE_ANONYMOUS_GROUP_NAME "anon_group"

#define OPEN_NONEXISTENT_GROUP_TEST_GNAME "/nonexistent_group"

#define GROUP_PROPERTY_LIST_TEST_GROUP_NAME1 "property_list_test_group1"
#define GROUP_PROPERTY_LIST_TEST_GROUP_NAME2 "property_list_test_group2"
#define GROUP_PROPERTY_LIST_TEST_DUMMY_VAL   100

#endif
