/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
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
 * Purpose:     
 *
 *     Tests something, we think?
 *
 * Programmer: Jacob Smith <jake.smith@hdfgroup.org>
 *             2017-10-11
 *
 */

#include "h5test.h"

#if 0

#define KB              1024U
#define FAMILY_NUMBER   4
#define FAMILY_SIZE     (1*KB)
#define FAMILY_SIZE2    (5*KB)
#define MULTI_SIZE      128

#define CORE_INCREMENT  (4*KB)
#define CORE_PAGE_SIZE  (1024*KB)
#define CORE_DSET_NAME  "core dset"
#define CORE_DSET_DIM1  1024
#define CORE_DSET_DIM2  32

#define DSET1_NAME   "dset1"
#define DSET1_DIM1   1024
#define DSET1_DIM2   32
#define DSET3_NAME   "dset3"

/* Macros for Direct VFD */
#ifdef H5_HAVE_DIRECT
#define MBOUNDARY    512
#define FBSIZE       (4*KB)
#define CBSIZE       (8*KB)
#define THRESHOLD    1
#define DSET2_NAME   "dset2"
#define DSET2_DIM    4
#endif /* H5_HAVE_DIRECT */

#endif

const char *FILENAME[] = {
    "sec2_file",         /*0*/
    "core_file",         /*1*/
    "family_file",       /*2*/
    "new_family_v16_",   /*3*/
    "multi_file",        /*4*/
    "direct_file",       /*5*/
    "log_file",          /*6*/
    "stdio_file",        /*7*/
    "windows_file",      /*8*/
    "new_multi_file_v16",/*9*/
    "ro_s3_file6",       /*10*/
    NULL
};

#if 0
#define LOG_FILENAME "log_ros3_out.log"
#endif




/*-------------------------------------------------------------------------
 *
 * Function:    test_ros3
 *
 * Purpose:     Tests the file handle interface for the ROS3 driver
 *
 *              As the ROS3 driver is 1) read only, 2) requires access
 *              to an S3 server (minio for now), this test is quite
 *              different from the other tests.
 *
 *              For now, test only fapl & flags.  Extend as the 
 *              work on the VFD continues.
 *
 * Return:      Success:        0
 *              Failure:        -1
 *
 * Programmer:  John Mainzer
 *              7/12/17
 *
 *-------------------------------------------------------------------------
 */
static herr_t
test_ros3(void)
{
    hid_t       fid = -1;                   /* file ID                      */
    hid_t       fapl_id = -1;               /* file access property list ID */
    hid_t       fapl_id_out = -1;           /* from H5Fget_access_plist     */
    hid_t       driver_id = -1;             /* ID for this VFD              */
    unsigned long driver_flags = 0;         /* VFD feature flags            */
    char        filename[1024];             /* filename                     */
    void        *os_file_handle = NULL;     /* OS file handle               */
    hsize_t     file_size;                  /* file size                    */
    H5FD_ros3_fapl_t test_ros3_fa;
    H5FD_ros3_fapl_t ros3_fa_0 = 
    {
        /* version      = */ H5FD__CURR_ROS3_FAPL_T_VERSION,
        /* authenticate = */ FALSE,
        /* aws_region   = */ "", 
        /* secret_id    = */ "", 
        /* secret_key   = */ "plugh",
    };

    TESTING("ROS3 file driver");

    /* Set property list and file name for ROS3 driver. */
    if((fapl_id = H5Pcreate(H5P_FILE_ACCESS)) < 0)
        TEST_ERROR;

    if(H5Pset_fapl_ros3(fapl_id, &ros3_fa_0) < 0)
        TEST_ERROR;

    /* verify that the ROS3 FAPL entry is set as expected */
    if(H5Pget_fapl_ros3(fapl_id, &test_ros3_fa) < 0)
        TEST_ERROR;

    /* need a macro to compare instances of H5FD_ros3_fapl_t */
    if((test_ros3_fa.version != ros3_fa_0.version) ||
       (test_ros3_fa.authenticate != ros3_fa_0.authenticate) ||
       (strcmp(test_ros3_fa.aws_region, ros3_fa_0.aws_region) != 0) ||
       (strcmp(test_ros3_fa.secret_id, ros3_fa_0.secret_id) != 0) ||
       (strcmp(test_ros3_fa.secret_key, ros3_fa_0.secret_key) != 0))
        TEST_ERROR;

    h5_fixname(FILENAME[10], fapl_id, filename, sizeof(filename));

    /* Check that the VFD feature flags are correct */
    if ((driver_id = H5Pget_driver(fapl_id)) < 0)
        TEST_ERROR;

    if (H5FDdriver_query(driver_id, &driver_flags) < 0)
        TEST_ERROR;

    if(!(driver_flags & H5FD_FEAT_DATA_SIEVE))              TEST_ERROR

    /* Check for extra flags not accounted for above */
    if(driver_flags != (H5FD_FEAT_DATA_SIEVE))
        TEST_ERROR

    /* can't create analogs of the following tests until the 
     * ROS3 driver is up and running in a minimal fashion.
     * Comment them out until we get to them.
     */
#if 0 
    if((fid = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, fapl_id)) < 0)
        TEST_ERROR;

    /* Retrieve the access property list... */
    if((fapl_id_out = H5Fget_access_plist(fid)) < 0)
        TEST_ERROR;

    /* Check that the driver is correct */
    if(H5FD_ROS3 != H5Pget_driver(fapl_id_out))
        TEST_ERROR;

    /* ...and close the property list */
    if(H5Pclose(fapl_id_out) < 0)
        TEST_ERROR;

    /* Check that we can get an operating-system-specific handle from
     * the library.
     */
    if(H5Fget_vfd_handle(fid, H5P_DEFAULT, &os_file_handle) < 0)
        TEST_ERROR;
    if(os_file_handle == NULL)
        FAIL_PUTS_ERROR("NULL os-specific vfd/file handle was returned from H5Fget_vfd_handle");


    /* There is no garantee the size of metadata in file is constant.
     * Just try to check if it's reasonable.
     * 
     * Currently it should be around 2 KB.
     */
    if(H5Fget_filesize(fid, &file_size) < 0)
        TEST_ERROR;
    if(file_size < 1 * KB || file_size > 4 * KB)
        FAIL_PUTS_ERROR("suspicious file size obtained from H5Fget_filesize");

    /* Close and delete the file */
    if(H5Fclose(fid) < 0)
        TEST_ERROR;
    h5_delete_test_file(FILENAME[0], fapl_id);

    /* Close the fapl */
    if(H5Pclose(fapl_id) < 0)
        TEST_ERROR;
#endif

    PASSED();
    return 0;

error:
    H5E_BEGIN_TRY {
        H5Pclose(fapl_id);
        H5Pclose(fapl_id_out);
        H5Fclose(fid);
    } H5E_END_TRY;
    return -1;
} /* end test_ros3() */



/*-------------------------------------------------------------------------
 * Function:    main
 *
 * Purpose:     Tests the basic features of Virtual File Drivers
 *
 * Return:      Success:        0
 *              Failure:        1
 *
 * Programmer:  Raymond Lu
 *              Tuesday, Sept 24, 2002
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    int nerrors = 0;

    h5_reset();

    HDprintf("Testing S3Communications and ros3 VFD functionality.\n");

    nerrors += test_ros3()                    < 0 ? 1 : 0;

    if(nerrors) {
        HDprintf("***** %d S3comms + ros3 TEST%s FAILED! *****\n",
            nerrors, nerrors > 1 ? "S" : "");
        return 1;
    } /* end if */

    HDprintf("All S3comms and ros3 tests passed.\n");

    return 0;
} /* end main() */

