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
 *     Verify behavior for Read-Only S3 VFD
 *     at the VFL (virtual file layer) level.
 *
 *     Demonstrates basic use cases and fapl/dxpl interaction.
 *
 * Programmer: Jacob Smith <jake.smith@hdfgroup.org>
 *             2017-10-11
 */

#include "h5test.h"

#include "H5ACprivate.h" /* for dxpl creation */
#include "H5FDprivate.h" /* Virtual File Driver utilities */
#include "H5Iprivate.h"  /* for dxpl creation */
#include "H5Pprivate.h"  /* for dxpl creation */
#include "H5FDros3.h"    /* this file driver's utilities */
#include <curl/curl.h>   /* for CURL global init/cleanup */



/*****************************************************************************
 *
 * FILE-LOCAL TESTING MACROS
 *
 * Purpose:
 *
 *     1) Upon test failure, goto-jump to single-location teardown in test 
 *        function. E.g., `error:` (consistency with HDF corpus) or
 *        `failed:` (reflects purpose).
 *            >>> using "error", in part because `H5E_BEGIN_TRY` expects it.
 *     2) Increase clarity and reduce overhead found with `TEST_ERROR`.
 *        e.g., "if(somefunction(arg, arg2) < 0) TEST_ERROR:"
 *        requires reading of entire line to know whether this if/call is
 *        part of the test setup, test operation, or a test unto itself.
 *     3) Provide testing macros with optional user-supplied failure message;
 *        if not supplied (NULL), generate comparison output in the spirit of 
 *        test-driven development. E.g., "expected 5 but was -3"
 *        User messages clarify test's purpose in code, encouraging description
 *        without relying on comments.
 *     4) Configurable expected-actual order in generated comparison strings.
 *        Some prefer `VERIFY(expected, actual)`, others 
 *        `VERIFY(actual, expected)`. Provide preprocessor ifdef switch
 *        to satifsy both parties, assuming one paradigm per test file.
 *        (One could #undef and redefine the flag through the file as desired,
 *         but _why_.)
 *
 *     Provided as courtesy, per consideration for inclusion in the library 
 *     proper.
 *
 *     Macros:
 * 
 *         JSVERIFY_EXP_ACT - ifdef flag, configures comparison order
 *         FAIL_IF()        - check condition
 *         FAIL_UNLESS()    - check _not_ condition
 *         JSVERIFY()       - long-int equality check; prints reason/comparison
 *         JSVERIFY_NOT()   - long-int inequality check; prints
 *         JSVERIFY_STR()   - string equality check; prints
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *****************************************************************************/


/*----------------------------------------------------------------------------
 *
 * ifdef flag: JSVERIFY_EXP_ACT
 * 
 * JSVERIFY macros accept arguments as (EXPECTED, ACTUAL[, reason]) 
 *   default, if this is undefined, is (ACTUAL, EXPECTED[, reason])
 *
 *----------------------------------------------------------------------------
 */
#define JSVERIFY_EXP_ACT 1L


/*----------------------------------------------------------------------------
 *
 * Macro: JSFAILED_AT()
 *
 * Purpose:
 *
 *     Preface a test failure by printing "*FAILED*" and location to stdout
 *     Similar to `H5_FAILED(); AT();` from h5test.h
 *
 *     *FAILED* at somefile.c:12 in function_name()...
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSFAILED_AT() {                                                   \
    HDprintf("*FAILED* at %s:%d in %s()...\n", __FILE__, __LINE__, FUNC); \
}


/*----------------------------------------------------------------------------
 *
 * Macro: FAIL_IF()
 *
 * Purpose:  
 *
 *     Make tests more accessible and less cluttered than
 *         `if (thing == otherthing()) TEST_ERROR` 
 *         paradigm.
 *
 *     The following lines are roughly equivalent:
 *
 *         `if (myfunc() < 0) TEST_ERROR;` (as seen elsewhere in HDF tests)
 *         `FAIL_IF(myfunc() < 0)`
 *
 *     Prints a generic "FAILED AT" line to stdout and jumps to `error`,
 *     similar to `TEST_ERROR` in h5test.h
 *
 * Programmer: Jacob Smith
 *             2017-10-23
 *
 *----------------------------------------------------------------------------
 */
#define FAIL_IF(condition) \
if (condition) {           \
    JSFAILED_AT()          \
    goto error;           \
}


/*----------------------------------------------------------------------------
 *
 * Macro: FAIL_UNLESS()
 *
 * Purpose:
 *
 *     TEST_ERROR wrapper to reduce cognitive overhead from "negative tests",
 *     e.g., "a != b".
 *     
 *     Opposite of FAIL_IF; fails if the given condition is _not_ true.
 *
 *     `FAIL_IF( 5 != my_op() )`
 *     is equivalent to
 *     `FAIL_UNLESS( 5 == my_op() )`
 *     However, `JSVERIFY(5, my_op(), "bad return")` may be even clearer.
 *         (see JSVERIFY)
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define FAIL_UNLESS(condition) \
if (!(condition)) {            \
    JSFAILED_AT()              \
    goto error;                \
}


/*----------------------------------------------------------------------------
 *
 * Macro: JSERR_LONG()
 *
 * Purpose:
 *
 *     Print an failure message for long-int arguments.
 *     ERROR-AT printed first.
 *     If `reason` is given, it is printed on own line and newlined after
 *     else, prints "expected/actual" aligned on own lines.
 *
 *     *FAILED* at myfile.c:488 in somefunc()...
 *     forest must be made of trees.
 *
 *     or
 *
 *     *FAILED* at myfile.c:488 in somefunc()...
 *       ! Expected 425
 *       ! Actual   3
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSERR_LONG(expected, actual, reason) {           \
    JSFAILED_AT()                                        \
    if (reason!= NULL) {                                 \
        HDprintf("%s\n", (reason));                      \
    } else {                                             \
        HDprintf("  ! Expected %ld\n  ! Actual   %ld\n", \
                  (long)(expected), (long)(actual));     \
    }                                                    \
}


/*----------------------------------------------------------------------------
 *
 * Macro: JSERR_STR()
 *
 * Purpose:
 *
 *     Print an failure message for string arguments.
 *     ERROR-AT printed first.
 *     If `reason` is given, it is printed on own line and newlined after
 *     else, prints "expected/actual" aligned on own lines.
 *
 *     *FAILED*  at myfile.c:421 in myfunc()...
 *     Blue and Red strings don't match!
 *
 *     or
 *
 *     *FAILED*  at myfile.c:421 in myfunc()...
 *     !!! Expected:
 *     this is my expected
 *     string
 *     !!! Actual:
 *     not what I expected at all
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSERR_STR(expected, actual, reason) {           \
    JSFAILED_AT()                                       \
    if ((reason) != NULL) {                             \
        HDprintf("%s\n", (reason));                     \
    } else {                                            \
        HDprintf("!!! Expected:\n%s\n!!!Actual:\n%s\n", \
                 (expected), (actual));                 \
    }                                                   \
}



#ifdef JSVERIFY_EXP_ACT


/*----------------------------------------------------------------------------
 *
 * Macro: JSVERIFY()
 *
 * Purpose: 
 *
 *     Verify that two long integers are equal.
 *     If unequal, print failure message 
 *     (with `reason`, if not NULL; expected/actual if NULL)
 *     and jump to `error` at end of function
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSVERIFY(expected, actual, reason)     \
if ((long)(actual) != (long)(expected)) {      \
    JSERR_LONG((expected), (actual), (reason)) \
    goto error;                                \
} /* JSVERIFY */


/*----------------------------------------------------------------------------
 *
 * Macro: JSVERIFY_NOT()
 *
 * Purpose: 
 *
 *     Verify that two long integers are _not_ equal.
 *     If equal, print failure message 
 *     (with `reason`, if not NULL; expected/actual if NULL)
 *     and jump to `error` at end of function
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSVERIFY_NOT(expected, actual, reason) \
if ((long)(actual) == (long)(expected)) {      \
    JSERR_LONG((expected), (actual), (reason)) \
    goto error;                                \
} /* JSVERIFY_NOT */


/*----------------------------------------------------------------------------
 *
 * Macro: JSVERIFY_STR()
 *
 * Purpose: 
 *
 *     Verify that two strings are equal.
 *     If unequal, print failure message 
 *     (with `reason`, if not NULL; expected/actual if NULL)
 *     and jump to `error` at end of function
 *
 * Programmer: Jacob Smith
 *             2017-10-24
 *
 *----------------------------------------------------------------------------
 */
#define JSVERIFY_STR(expected, actual, reason) \
if (strcmp((actual), (expected)) != 0) {       \
    JSERR_STR((expected), (actual), (reason)); \
    goto error;                                \
} /* JSVERIFY_STR */


#else 
/* JSVERIFY_EXP_ACT not defined 
 *
 * Repeats macros above, but with actual/expected parameters reversed.
 */


/*----------------------------------------------------------------------------
 * Macro: JSVERIFY()
 * See: JSVERIFY documentation above.
 * Programmer: Jacob Smith
 *             2017-10-14
 *----------------------------------------------------------------------------
 */
#define JSVERIFY(actual, expected, reason)      \
if ((long)(actual) != (long)(expected)) {       \
    JSERR_LONG((expected), (actual), (reason)); \
    goto error;                                 \
} /* JSVERIFY */


/*----------------------------------------------------------------------------
 * Macro: JSVERIFY_NOT()
 * See: JSVERIFY_NOT documentation above.
 * Programmer: Jacob Smith
 *             2017-10-14
 *----------------------------------------------------------------------------
 */
#define JSVERIFY_NOT(actual, expected, reason) \
if ((long)(actual) == (long)(expected)) {      \
    JSERR_LONG((expected), (actual), (reason)) \
    goto error;                                \
} /* JSVERIFY_NOT */


/*----------------------------------------------------------------------------
 * Macro: JSVERIFY_STR()
 * See: JSVERIFY_STR documentation above.
 * Programmer: Jacob Smith
 *             2017-10-14
 *----------------------------------------------------------------------------
 */
#define JSVERIFY_STR(actual, expected, reason) \
if (strcmp((actual), (expected)) != 0) {       \
    JSERR_STR((expected), (actual), (reason)); \
    goto error;                                \
} /* JSVERIFY_STR */

#endif /* ifdef/else JSVERIFY_EXP_ACT */

/********************************
 * OTHER MACROS AND DEFINITIONS *
 ********************************/

/* copied from src/ros3.c 
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(HDoff_t)-1))-1)


/*---------------------------------------------------------------------------
 *
 * Function: test_fapl_config_validation()
 *
 * Purpose: 
 *
 *     Test data consistency of fapl configuration.
 *     Tests `H5FD_ros3_validate_config` indirectly through `H5Pset_fapl_ros3`.
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-10-23
 *
 * Changes: None.
 *
 *---------------------------------------------------------------------------
 */
static int
test_fapl_config_validation(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        H5FD_ros3_fapl_t  config;
        herr_t            expected;
        const char       *msg;
    };

    /************************
     * test-local variables *
     ************************/

    hid_t            fapl_id     = -1;   /* file access property list ID */
    H5FD_ros3_fapl_t config;
    H5FD_ros3_fapl_t fa_fetch;
    herr_t           success     = SUCCEED;
    unsigned int     i           = 0;
    unsigned int     ncases      = 8;    /* should equal number of cases */
    struct testcase *case_ptr    = NULL; /* dumb work-around for possible     */
                                         /* dynamic cases creation because    */
                                         /* of compiler warnings Wlarger-than */
    struct testcase  cases_arr[] = {
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION, /* version      */
                FALSE,                          /* authenticate */
                "",                             /* aws_region   */
                "",                             /* secret_id    */
                "",                             /* secret_key   */
            },
            SUCCEED,
            "non-authenticating config allows empties.\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "",
                "",
                "",
            },
            FAIL,
            "authenticating config asks for populated strings.\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "region",
                "me",
                "",
            },
            SUCCEED,
            "populated strings; key is the empty string?\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "",
                "me",
                "",
            },
            FAIL,
            "id cannot be empty.\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "where",
                "",
                "",
            },
            FAIL,
            "region cannot be empty.\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "where",
                "who",
                "thisIsA GREAT seeeecrit",
            },
            SUCCEED,
            "all strings populated.\n",
        },
        {   {   12345,
                FALSE,
                "",
                "",
                "",
            },
            FAIL,
            "incorrect version should fail\n",
        },
        {   {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                FALSE,
                "someregion",
                "someid",
                "somekey",
            },
            SUCCEED,
            "non-authenticating config cares not for (de)population"
            "of strings.\n",
        },
    };



    TESTING("ROS3 fapl configuration validation");



    /*********
     * TESTS *
     *********/

    for (i = 0; i < ncases; i++) {

        /*---------------
         * per-test setup 
         *---------------
         */
        case_ptr = &cases_arr[i];
        fapl_id = H5Pcreate(H5P_FILE_ACCESS);
        FAIL_IF( fapl_id < 0 ) /* sanity-check */

        /*-----------------------------------
         * Actually test.
         * Mute stack trace in failure cases.
         *-----------------------------------
         */
        H5E_BEGIN_TRY {
            /* `H5FD_ros3_validate_config(...)` is static/private 
             * to src/ros3.c and cannot (and should not?) be tested directly?
             * Instead, validate config through public api.
             */
            success = H5Pset_fapl_ros3(fapl_id, &case_ptr->config);
        } H5E_END_TRY;

        JSVERIFY( case_ptr->expected, success, case_ptr->msg )
        
        /* Make sure we can get back what we put in.
         * Only valid if the fapl configuration does not result in error.
         */
        if (success == SUCCEED) {
            config = case_ptr->config;
            JSVERIFY( SUCCEED,
                      H5Pget_fapl_ros3(fapl_id, &fa_fetch),
                      "unable to get fapl" )
            /* or,  ( H5Pget_fapl_ros3(...) < 0 )  */

            JSVERIFY( H5FD__CURR_ROS3_FAPL_T_VERSION, fa_fetch.version, NULL )
            JSVERIFY( config.version,      fa_fetch.version,      NULL )
            JSVERIFY( config.authenticate, fa_fetch.authenticate, NULL )
            JSVERIFY_STR( config.aws_region, fa_fetch.aws_region, NULL )    
            JSVERIFY_STR( config.secret_id,  fa_fetch.secret_id,  NULL )
            JSVERIFY_STR( config.secret_key, fa_fetch.secret_key, NULL )
        }

        /*-----------------------------
         * per-test sanitation/teardown
         *-----------------------------
         */
        HDassert( SUCCEED == H5Pclose(fapl_id) );
        fapl_id = -1;

    } /* for each test case */

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fapl_id < 0) {
        H5E_BEGIN_TRY {
            (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }
    return 1;

} /* test_fapl_config_validation */


/*-------------------------------------------------------------------------
 *
 * Function:    test_ros3_fapl()
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
 *              Failure:        1
 *
 * Programmer:  John Mainzer
 *              7/12/17
 *
 * Changes:     Test only fapl and flags.
 *              Jacob Smith 2017
 *
 *-------------------------------------------------------------------------
 */
static int
test_ros3_fapl(void)
{
    /************************
     * test-local variables *
     ************************/

    hid_t             fapl_id        = -1;  /* file access property list ID */
    hid_t             driver_id      = -1;  /* ID for this VFD              */
    unsigned long     driver_flags   =  0;  /* VFD feature flags            */
    H5FD_ros3_fapl_t  ros3_fa_0      = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version       */
        FALSE,                          /* authenticate  */
        "",                             /* aws_region    */
        "",                             /* secret_id     */
        "plugh",                        /* secret_key    */
    };



    TESTING("ROS3 fapl ");

    /* Set property list and file name for ROS3 driver. 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 ) /* sanity-check */

    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa_0), NULL )

    /****************
     * Check that the VFD feature flags are correct
     * SPEC MAY CHANGE 
     ******************/

    driver_id = H5Pget_driver(fapl_id);
    FAIL_IF( driver_id < 0 )

    FAIL_IF( H5FDdriver_query(driver_id, &driver_flags) < 0 )

    /* bit(s) in `driver_flags` must align with H5FD_FEAT_DATA_SIEVE
     */
    /* FAIL_IF( !(driver_flags & H5FD_FEAT_DATA_SIEVE) ) */
    JSVERIFY_NOT( 0, (driver_flags & H5FD_FEAT_DATA_SIEVE), 
                  "bit(s) in `driver_flags` must align with "
                  "H5FD_FEAT_DATA_SIEVE" )

    JSVERIFY( H5FD_FEAT_DATA_SIEVE, driver_flags,
              "H5FD_FEAT_DATA_SIEVE should be the only supported flag")

    PASSED();
    return 0;

error:
    H5E_BEGIN_TRY {
        (void)H5Pclose(fapl_id);
    } H5E_END_TRY;

    return 1;

} /* test_ros3_fapl() */


/*---------------------------------------------------------------------------
 *
 * Function: test_vfd_open()
 *
 * Purpose: 
 *
 *     Demonstrate/specify VFD-level "Open" failure cases
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             1027-11-03
 *
 *---------------------------------------------------------------------------
 */
static int
test_vfd_open(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*------------------------------------------------------------------------
     * VFD_OPEN_VERIFY_NULL()
     *
     * - Wrapper to clarify tests where H5FDopen() should fail with given 
     *   arguments: returns NULL.
     * - Uses `H5E_BEGIN_TRY` and `H5E_END_TRY` to mute stack trace from 
     *   expected error during open.
     *
     * - If `H5FDopen()` does _not_ return NULL with the given arguments,
     *   verication fails and prints FAILED message. (see `JSFAILED_AT()`)
     * - If `reason` is not NULL, it is printed after the *FAILED* output.
     *
     * Uses variable `H5FD_t *fd` for `H5FDopen()` return value.
     *
     * Jacob Smith 2017-10-27
     *------------------------------------------------------------------------
     */
#define VFD_OPEN_VERIFY_NULL(reason, fname, flags, fapl_id, maxaddr) \
{   H5E_BEGIN_TRY {                                                  \
        fd = H5FDopen((fname), (flags), (fapl_id), (maxaddr));       \
    } H5E_END_TRY;                                                   \
    if (NULL != fd) {                                                \
        JSFAILED_AT()                                                \
        if (reason != NULL) HDprintf(reason);                        \
        goto error;                                                  \
    }                                                                \
} /* VFD_OPEN_VERIFY_NULL */

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    H5FD_t           *fd         = NULL;
    hbool_t           curl_ready = FALSE;
    hid_t             fapl_id    = -1;
    H5FD_ros3_fapl_t  ros3_fa    = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version      */
        TRUE,                           /* authenticate */
        "us-east-1",                    /* aws_region   */
        "HDFGROUP0",                    /* secret_id    */
        "HDFGROUP0",                    /* secret_key   */
    };



    TESTING("ROS3 VFD-level open");

    /* required setup for s3comms underneath
     * FAIL_UNLESS instead of HDassert, because other tests might be run
     * even if this one cannot
     */
    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT))

    curl_ready = TRUE;

    /*********
     * TESTS *
     *********/

    VFD_OPEN_VERIFY_NULL( "default _property list_ is not allowed",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_RDONLY, H5P_DEFAULT, MAXADDR )

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 ) /* sanity-check */

    VFD_OPEN_VERIFY_NULL(
            "generic file access property list is not allowed",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
             H5F_ACC_RDONLY, H5P_DEFAULT, MAXADDR )

    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa),
              "(sanity check) unable to set fapl" )

    /* filename must be valid
     */
    VFD_OPEN_VERIFY_NULL( "filename cannot be null",
            NULL, H5F_ACC_RDONLY, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL( "filename cannot be empty", 
            "", H5F_ACC_RDONLY, fapl_id, MAXADDR )

    /* File must exist at given URL/URI */
    VFD_OPEN_VERIFY_NULL( "file must exist",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/nonexistent.txt",
            H5F_ACC_RDWR, fapl_id, MAXADDR )

    /* only supported flag is "Read-Only"
     */
    VFD_OPEN_VERIFY_NULL( "read-write flag not supported",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_RDWR, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL( "truncate flag not supported",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_TRUNC, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL( "create flag not supported",
            "http://minio.ad.hdfgroup.org/shakespeare/t8.shakespeare.txt",
            H5F_ACC_CREAT, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL( "EXCL flag not supported",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_EXCL, fapl_id, MAXADDR )

    /* maxaddr limitations
     */
    VFD_OPEN_VERIFY_NULL( "MAXADDR cannot be 0 (caught in `H5FD_open()`)",
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_RDONLY, fapl_id, 0 )

    /* finally, show that a file can be opened 
     */
    fd = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt", 
            H5F_ACC_RDONLY, 
            fapl_id, 
            MAXADDR);
    /* long/pointer conflation
     * JSVERIFY_NOT( NULL,  fd, "this open should succeed!" )
     */
    FAIL_IF( NULL == fd ) 

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5FDclose(fd) );
    fd = NULL;

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fd) { 
        (void)H5FDclose(fd); 
    }
    if (fapl_id >= 0) {
        H5E_BEGIN_TRY {
            (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }
    if (curl_ready == TRUE) {
        curl_global_cleanup();
    }

    return 1;

#undef VFD_OPEN_VERIFY_NULL

} /* test_vfd_open */


/*---------------------------------------------------------------------------
 *
 * Function: test_eof_eoa()
 *
 * Purpose: 
 *
 *     Demonstrate behavior of get_eof, get_eoa, and set_eoa.
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-11-08
 *
 *---------------------------------------------------------------------------
 */
static int
test_eof_eoa(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    H5FD_t           *fd_shakespeare  = NULL;
    hbool_t           curl_ready = FALSE;
    hid_t             fapl_id    = -1;
    H5FD_ros3_fapl_t  ros3_fa    = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version      */
        TRUE,                           /* authenticate */
        "us-east-1",                    /* aws_region   */
        "HDFGROUP0",                    /* secret_id    */
        "HDFGROUP0",                    /* secret_key   */
    };



    TESTING("ROS3 eof/eoa gets and sets");

    /*********
     * SETUP *
     *********/
    
    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa), NULL )

    fd_shakespeare = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
             H5F_ACC_RDONLY,
             fapl_id,
             HADDR_UNDEF);
    FAIL_IF( NULL == fd_shakespeare )

    /*********
     * TESTS *
     *********/

    /* verify as found
     */
    JSVERIFY( 5458199, H5FDget_eof(fd_shakespeare, H5FD_MEM_DEFAULT), NULL )
    JSVERIFY( H5FDget_eof(fd_shakespeare, H5FD_MEM_DEFAULT),
              H5FDget_eof(fd_shakespeare, H5FD_MEM_DRAW),
              "mismatch between DEFAULT and RAW memory types" )
    JSVERIFY( 0,
              H5FDget_eoa(fd_shakespeare, H5FD_MEM_DEFAULT), 
              "EoA should be unset by H5FDopen" )

    /* set EoA below EoF
     */
    JSVERIFY( SUCCEED, 
              H5FDset_eoa(fd_shakespeare, H5FD_MEM_DEFAULT, 44442202), 
              "unable to set EoA (lower)" )
    JSVERIFY( 5458199, 
              H5FDget_eof(fd_shakespeare, H5FD_MEM_DEFAULT), 
              "EoF changed" )
    JSVERIFY( 44442202, 
              H5FDget_eoa(fd_shakespeare, H5FD_MEM_DEFAULT), 
              "EoA unchanged" )

    /* set EoA above EoF
     */
    JSVERIFY( SUCCEED, 
              H5FDset_eoa(fd_shakespeare, H5FD_MEM_DEFAULT, 6789012), 
              "unable to set EoA (higher)" )
    JSVERIFY( 5458199, 
              H5FDget_eof(fd_shakespeare, H5FD_MEM_DEFAULT), 
              "EoF changed" )
    JSVERIFY( 6789012, 
              H5FDget_eoa(fd_shakespeare, H5FD_MEM_DEFAULT), 
              "EoA unchanged" )

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5FDclose(fd_shakespeare) );

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fd_shakespeare)     { (void)H5FDclose(fd_shakespeare); }
    if (TRUE == curl_ready) { curl_global_cleanup(); }
    if (fapl_id >= 0) { 
        H5E_BEGIN_TRY {
            (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }

    return 1;

} /* test_eof_eoa */


/*---------------------------------------------------------------------------
 *
 * Function: test_read()
 *
 * Purpose: 
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-11-06
 *
 *---------------------------------------------------------------------------
 */
static int
test_read(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    char              buffer[256];
    hbool_t           curl_ready       = FALSE;
    hbool_t           show_progress    = FALSE;
    unsigned int      i                = 0;
    H5FD_t           *file_shakespeare = NULL;
    H5FD_t           *file_raven       = NULL;
    hid_t             fapl_id          = -1;
    hid_t             dxpl_id          = -1;
    H5FD_dxpl_type_t  dxpl_type_raw    = H5FD_RAWDATA_DXPL;
    H5P_genplist_t   *dxpl_plist       = NULL;
    H5FD_ros3_fapl_t  ros3_fa          = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version       */
        TRUE,                           /* authenticate  */
        "us-east-1",                    /* aws_region    */
        "HDFGROUP0",                    /* secret_id     */
        "HDFGROUP0",                    /* secret_key    */
    };



    TESTING("ROS3 VFD read/range-gets");

    /*********
     * SETUP *
     *********/

    /* initialize CURL
     */
    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    /* create ROS3 fapl 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 )
    JSVERIFY( SUCCEED, 
              H5Pset_fapl_ros3(fapl_id, &ros3_fa),
              "problem configuring fapl" )

    /* create suitable dxpl
     */
    dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    FAIL_IF( dxpl_id < 0 )

    dxpl_plist = (H5P_genplist_t *)H5I_object(dxpl_id);
    FAIL_IF( NULL == dxpl_plist )

    JSVERIFY( SUCCEED, 
              H5P_set(dxpl_plist, H5FD_DXPL_TYPE_NAME, &dxpl_type_raw),
              "problem setting dxpl type" )

    /* Verify that dxpl_id reflects type setting.
     * i.e., set was successful.
     */
    {
        H5P_genplist_t   *test_plist     = NULL;
        H5FD_dxpl_type_t  test_dxpl_type = H5FD_METADATA_DXPL;

        test_plist = (H5P_genplist_t *)H5I_object(dxpl_id); 
        HDassert(test_plist);

        JSVERIFY( SUCCEED,
                  H5P_get(test_plist, 
                          H5FD_DXPL_TYPE_NAME, 
                          &test_dxpl_type),
                   NULL )

        JSVERIFY( H5FD_RAWDATA_DXPL, test_dxpl_type, NULL )

        if (show_progress) { 
            HDfprintf(stdout, "dxpl_type set successfully\n");
        }
    }

    /* open two separate files 
     */
    file_raven = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF); /* Demonstrate success with "automatic" value */
    FAIL_IF( NULL == file_raven )

    file_shakespeare = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            MAXADDR);
    FAIL_IF( NULL == file_shakespeare )

    for (i = 0; i < 256; i++) { buffer[i] = 0; } /* zero buffer contents */

    /*********
     * TESTS *
     *********/

    JSVERIFY( 5458199, H5FDget_eof(file_shakespeare, H5FD_MEM_DEFAULT), NULL )

    JSVERIFY( 0, H5FDget_eoa(file_shakespeare, H5FD_MEM_DEFAULT), 
              "EoA should remain unset by H5FDopen" )

    if (show_progress) {
        HDfprintf(stdout, "\n\n******* read fail (address overflow ) ******\n");
    }

    H5E_BEGIN_TRY { /* mute stack trace on expected failure */
        JSVERIFY( FAIL,
                  H5FDread(file_shakespeare, 
                       H5FD_MEM_DRAW, 
                       dxpl_id,
                       1200699,
                       102,
                       buffer),
                  "address beyond EoA (0) results in read failure/error" )
    } H5E_END_TRY;

    if (show_progress) {
        HDfprintf(stdout, "\n\n******* first read ******\n");
    }

    JSVERIFY( SUCCEED, 
              H5FDset_eoa(file_shakespeare, 
                          H5FD_MEM_DEFAULT, 
                          H5FDget_eof(file_shakespeare, H5FD_MEM_DEFAULT) ),
              "unable to set EoA" )

    JSVERIFY( SUCCEED, 
              H5FDread(file_shakespeare, 
                       H5FD_MEM_DRAW, 
                       dxpl_id,
                       1200699,
                       102,
                       buffer),
               "unable to execute read" )
    JSVERIFY_STR( "Osr. Sweet lord, if your lordship were at leisure, " \
                  "I should impart\n    a thing to you from his Majesty.", 
            buffer, 
            NULL )

    for (i = 0; i < 256; i++) { buffer[i] = 0; }

    if (show_progress) {
        HDfprintf(stdout, "\n\n******* second read ******\n");
    }

    JSVERIFY( SUCCEED, 
              H5FDset_eoa(file_raven, 
                          H5FD_MEM_DEFAULT, 
                          H5FDget_eof(file_raven, H5FD_MEM_DEFAULT) ),
              "unable to set EoA" )

    JSVERIFY( SUCCEED,
              H5FDread(file_raven,
                       H5FD_MEM_DRAW,
                       dxpl_id,
                       5691,
                       31,
                       buffer),
               "unable to execute read" )
    JSVERIFY_STR( "Quoth the Raven “Nevermore.”", buffer, NULL )

    for (i = 0; i < 256; i++) { buffer[i] = 0; }

    if (show_progress) {
        HDfprintf(stdout, "\n\n******* addr past eoa ******\n");
    }

    H5E_BEGIN_TRY { /* mute stack trace on expected failure */
        JSVERIFY( FAIL, 
                  H5FDread(file_shakespeare,
                           H5FD_MEM_DRAW,
                           dxpl_id,
                           5555555,
                           102,
                           buffer),
                   "reading with addr past eoa/eof should fail" )
    } H5E_END_TRY;

    if (show_progress) {
        HDfprintf(stdout, "\n\n******* addr+size past eoa ******\n");
    }

    H5E_BEGIN_TRY { /* mute stack trace on expected failure */
        JSVERIFY( FAIL, 
                  H5FDread(file_shakespeare,
                           H5FD_MEM_DRAW,
                           dxpl_id,
                           5458000,
                           255,
                           buffer),
                   "reading with (addr+size) past eoa/eof should fail" )
    } H5E_END_TRY;



    if (show_progress) {
        HDfprintf(stdout, "\n\n******* tests successful ******\n");
    }

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5FDclose(file_raven) );
    file_raven = NULL;

    HDassert( SUCCEED == H5FDclose(file_shakespeare) );
    file_shakespeare = NULL;

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    HDassert( SUCCEED == H5Pclose(dxpl_id) );
    dxpl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (file_raven)         { (void)H5FDclose(file_raven);       }
    if (file_shakespeare)   { (void)H5FDclose(file_shakespeare); }
    if (curl_ready == TRUE) { curl_global_cleanup();             }
    if (fapl_id >= 0) {
        H5E_BEGIN_TRY {
           (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }
    if (dxpl_id >= 0) {
        H5E_BEGIN_TRY {
           (void)H5Pclose(dxpl_id);
        } H5E_END_TRY;
    }

    return 1;

} /* test_read */


/*---------------------------------------------------------------------------
 *
 * Function: test_noops_and_autofails()
 *
 * Purpose: 
 *
 *     Demonstrate the unavailable and do-nothing routines unique to
 *     Read-Only VFD.
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-11-06
 *
 *---------------------------------------------------------------------------
 */
static int
test_noops_and_autofails(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    hbool_t           curl_ready = FALSE;
    hid_t             fapl_id    = -1;
    H5FD_t           *file       = NULL;
    hid_t             dxpl_id    = -1;
    H5P_genplist_t   *dxpl_plist = NULL;
    H5FD_dxpl_type_t  dxpl_type  = H5FD_RAWDATA_DXPL;
    const char        data[36]   = "The Force shall be with you, always";
    H5FD_ros3_fapl_t  ros3_fa    = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version       */
        TRUE,                           /* authenticate  */
        "us-east-1",                    /* aws_region    */
        "HDFGROUP0",                    /* secret_id     */
        "HDFGROUP0",                    /* secret_key    */
    };



    TESTING("ROS3 VFD always-fail and no-op routines");

    /*********
     * SETUP *
     *********/

    /* prepare CURL 
     */
    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    /* create ROS3 fapl 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa), NULL )

    /* create suitable dxpl
     */
    dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    FAIL_IF( dxpl_id < 0 )
    dxpl_plist = (H5P_genplist_t *)H5I_object(dxpl_id);
    FAIL_IF( NULL == dxpl_plist )
    JSVERIFY( SUCCEED,
              H5P_set(dxpl_plist, H5FD_DXPL_TYPE_NAME, &dxpl_type),
              "unable to set dxpl" )

    /* open file
     */
    file = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == file )

    /*********
     * TESTS *
     *********/

    /* auto-fail calls to write and truncate
     */
    H5E_BEGIN_TRY {
        JSVERIFY( FAIL, 
                  H5FDwrite(file, H5FD_MEM_DRAW, dxpl_id, 1000, 35, data),
                  "write must fail" )
    } H5E_END_TRY;

    H5E_BEGIN_TRY {
        JSVERIFY( FAIL,
                  H5FDtruncate(file, dxpl_id, FALSE),
                  "truncate must fail" )
    } H5E_END_TRY;

    H5E_BEGIN_TRY {
        JSVERIFY( FAIL,
                  H5FDtruncate(file, dxpl_id, TRUE),
                  "truncate must fail (closing)" )
    } H5E_END_TRY;

    /* no-op calls to `lock()` and `unlock()`
     */
    JSVERIFY( SUCCEED,
              H5FDlock(file, TRUE),
              "lock always succeeds; has no effect" )
    JSVERIFY( SUCCEED,
              H5FDlock(file, FALSE),
              NULL )
    JSVERIFY( SUCCEED,
              H5FDunlock(file),
              NULL )
    /* Lock/unlock with null file or similar error crashes tests.
     * HDassert in calling heirarchy, `H5FD[un]lock()` and `H5FD_[un]lock()`
     */

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5FDclose(file) );
    file = NULL;

    HDassert( SUCCEED == H5Pclose(dxpl_id) );
    dxpl_id = -1;

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fapl_id >= 0) {
        H5E_BEGIN_TRY {
           (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }
    if (dxpl_id >= 0) {
        H5E_BEGIN_TRY {
           (void)H5Pclose(dxpl_id);
        } H5E_END_TRY;
    }
    if (file)               { (void)H5FDclose(file); }
    if (curl_ready == TRUE) { curl_global_cleanup(); }

    return 1;

} /* test_noops_and_autofails*/


/*---------------------------------------------------------------------------
 *
 * Function: test_cmp()
 *
 * Purpose: 
 *
 *     Verify "file comparison" behavior.
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-11-06
 *
 *---------------------------------------------------------------------------
 */
static int
test_cmp(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    H5FD_t           *fd_raven   = NULL;
    H5FD_t           *fd_shakes  = NULL;
    H5FD_t           *fd_raven_2 = NULL;
    hbool_t           curl_ready = FALSE;
    hid_t             fapl_id    = -1;
    H5FD_ros3_fapl_t  ros3_fa    = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version      */
        TRUE,                           /* authenticate */
        "us-east-1",                    /* aws_region   */
        "HDFGROUP0",                    /* secret_id    */
        "HDFGROUP0",                    /* secret_key   */
    };



    TESTING("ROS3 cmp (comparison)");

    /*********
     * SETUP *
     *********/

    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa), NULL )

    fd_raven = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == fd_raven )

    fd_shakes = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == fd_shakes )

    fd_raven_2 = H5FDopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt",
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == fd_raven_2 )

    /*********
     * TESTS *
     *********/

    JSVERIFY(  0, H5FDcmp(fd_raven,  fd_raven_2), NULL )
    JSVERIFY( -1, H5FDcmp(fd_raven,  fd_shakes),  NULL )
    JSVERIFY(  1, H5FDcmp(fd_shakes, fd_raven_2), NULL )

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5FDclose(fd_raven) );
    HDassert( SUCCEED == H5FDclose(fd_shakes) );
    HDassert( SUCCEED == H5FDclose(fd_raven_2) ); 

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fd_raven)           { (void)H5FDclose(fd_raven);   }
    if (fd_raven_2)         { (void)H5FDclose(fd_raven_2); }
    if (fd_shakes)          { (void)H5FDclose(fd_shakes);  }
    if (TRUE == curl_ready) { curl_global_cleanup(); }
    if (fapl_id >= 0) { 
        H5E_BEGIN_TRY {
            (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }

    return 1;

} /* test_cmp */


/*---------------------------------------------------------------------------
 *
 * Function: test_H5F_integration()
 *
 * Purpose: 
 *
 *     Demonstrate S3 file-open through H5F API.
 *
 * Return:
 *
 *     PASSED : 0
 *     FAILED : 1
 *
 * Programmer: Jacob Smith
 *             2017-11-07
 *
 *---------------------------------------------------------------------------
 */
static int
test_H5F_integration(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    hid_t             file       = -1;
    hbool_t           curl_ready = FALSE;
    hid_t             fapl_id    = -1;
    H5FD_ros3_fapl_t  ros3_fa    = {
        H5FD__CURR_ROS3_FAPL_T_VERSION, /* version       */
        TRUE,                           /* authenticate  */
        "us-east-1",                    /* aws_region    */
        "HDFGROUP0",                    /* secret_id     */
        "HDFGROUP0",                    /* secret_key    */
    };



    TESTING("S3 file access through HD5F library (H5F API)");

    /*********
     * SETUP *
     *********/

    FAIL_UNLESS( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &ros3_fa), NULL )

    /*********
     * TESTS *
     *********/

    /* Read-Write access is not allowed with this file driver.
     */
    H5E_BEGIN_TRY {
        FAIL_IF( 0 <= H5Fopen(
                      "http://minio.ad.hdfgroup.org:9000/shakespeare/t.h5",
                      H5F_ACC_RDWR,
                      fapl_id) )
    } H5E_END_TRY;

    /* H5Fcreate() is not allowed with this file driver.
     */
    H5E_BEGIN_TRY {
        FAIL_IF( 0 <= H5Fcreate(
                      "http://minio.ad.hdfgroup.org:9000/shakespeare/nope.h5",
                      H5F_ACC_RDONLY,
                      H5P_DEFAULT,
                      fapl_id) )
    } H5E_END_TRY;
    
    /* Successful open.
     */
    file = H5Fopen(
            "http://minio.ad.hdfgroup.org:9000/shakespeare/t.h5",
            H5F_ACC_RDONLY,
            fapl_id);
    FAIL_IF( file < 0 )

    /************
     * TEARDOWN *
     ************/

    HDassert( SUCCEED == H5Fclose(file) );
    file = -1;

    HDassert( SUCCEED == H5Pclose(fapl_id) );
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fapl_id >= 0) {
        H5E_BEGIN_TRY {
           (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }
    if (file > 0)           { (void)H5Fclose(file);  }
    if (curl_ready == TRUE) { curl_global_cleanup(); }

    return 1;

} /* test_H5F_integration */




/*-------------------------------------------------------------------------
 *
 * Function:    main
 *
 * Purpose:     Tests the basic features of Virtual File Drivers
 *
 * Return:      Success: 0
 *              Failure: 1
 *
 * Programmer:  Jacob Smith
 *              2017-10-23
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    int nerrors = 0;

    h5_reset();

    HDprintf("Testing ros3 VFD functionality.\n");

    nerrors += test_fapl_config_validation();
    nerrors += test_ros3_fapl();
    nerrors += test_vfd_open();
    nerrors += test_eof_eoa();
    nerrors += test_read();
    nerrors += test_noops_and_autofails();
    nerrors += test_cmp();
    nerrors += test_H5F_integration();

    if (nerrors > 0) {
        HDprintf("***** %d ros3 TEST%s FAILED! *****\n",
                 nerrors, 
                 nerrors > 1 ? "S" : "");
        nerrors = 1;
    } else {
        HDprintf("All ros3 tests passed.\n");
    }
    return nerrors; /* 0 if no errors, 1 if any errors */

} /* main() */


