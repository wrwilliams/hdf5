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

/*
 * A generic test suite meant to test a specified VOL connector or set of
 * VOL connectors.
 *
 * The first file-related test will create a file that contains a group for
 * each of the different class of tests.
 */

#include "vol_test.h"

#ifdef H5_HAVE_PARALLEL
#include "vol_test_parallel.h"
#endif

static size_t
parse_vol_connector_list()
{
    size_t number_of_connectors = 0;

    return number_of_connectors;
}

int main(int argc, char **argv)
{
    size_t i;
    size_t num_connectors = parse_vol_connector_list();
    int    nerrors = 0;

    h5_reset();

    for (i = 0; i < num_connectors; i++) {
        printf("Running VOL tests with VOL connector '%s'\n\n");

        nerrors += vol_file_test();
        nerrors += vol_group_test();
        nerrors += vol_dataset_test();
        nerrors += vol_datatype_test();
        nerrors += vol_attribute_test();
        nerrors += vol_link_test();
        nerrors += vol_object_test();
        nerrors += vol_misc_test();

#ifdef H5_HAVE_PARALLEL
        if (MAINPROCESS)
#endif
            puts("All VOL tests passed\n");
    } /* end for */

done:

    ALARM_OFF;

    H5close();

#ifdef H5_HAVE_PARALLEL
    MPI_Finalize();
#endif

    exit((nerrors ? EXIT_FAILURE : EXIT_SUCCESS));
}
