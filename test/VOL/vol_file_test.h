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

#ifndef VOL_FILE_TEST_H
#define VOL_FILE_TEST_H

#include "vol_test.h"

int vol_file_test(void);

/*********************************************
 *                                           *
 *      VOL connector File test defines      *
 *                                           *
 *********************************************/

#define FILE_INTENT_TEST_DATASETNAME "/test_dset"
#define FILE_INTENT_TEST_DSET_RANK   2
#define FILE_INTENT_TEST_FILENAME    "intent_test_file"

#define NONEXISTENT_FILENAME "nonexistent_file"

#define FILE_PROPERTY_LIST_TEST_FNAME1 "property_list_test_file1"
#define FILE_PROPERTY_LIST_TEST_FNAME2 "property_list_test_file2"

#endif
