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

#define S3_TEST_BUCKET_URL "https://s3.us-east-2.amazonaws.com/hdf5ros3"
#define S3_TEST_REGION "us-east-2"
#define S3_TEST_ACCESS_ID "AKIAIMC3D3XLYXLN5COA"
#define S3_TEST_ACCESS_KEY "ugs5aVVnLFCErO/8uW14iWE3K5AgXMpsMlWneO/+"
#define S3_TEST_MAX_URL_SIZE 256

#define S3_TEST_RESOURCE_TEXT_RESTRICTED "t8.shakespeare.txt"
#define S3_TEST_RESOURCE_TEXT_PUBLIC "Poe_Raven.txt"
#define S3_TEST_RESOURCE_H5_PUBLIC "GMODO-SVM01.h5"
#define S3_TEST_RESOURCE_MISSING "missing.csv"

static char url_text_restricted[S3_TEST_MAX_URL_SIZE];
static char url_text_public[S3_TEST_MAX_URL_SIZE];
static char url_h5_public[S3_TEST_MAX_URL_SIZE];
static char url_missing[S3_TEST_MAX_URL_SIZE];

H5FD_ros3_fapl_t restricted_access_fa = {
            H5FD__CURR_ROS3_FAPL_T_VERSION, /* fapl version      */
            TRUE,                           /* authenticate      */
            S3_TEST_REGION,                 /* aws region        */
            S3_TEST_ACCESS_ID,              /* access key id     */
            S3_TEST_ACCESS_KEY };           /* secret access key */

H5FD_ros3_fapl_t anonymous_fa = {
            H5FD__CURR_ROS3_FAPL_T_VERSION,
            FALSE, "", "", "" };


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
        const char       *msg;
        herr_t            expected;
        H5FD_ros3_fapl_t  config;
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
        {   "non-authenticating config allows empties.\n",
            SUCCEED,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION, /* version      */
                FALSE,                          /* authenticate */
                "",                             /* aws_region   */
                "",                             /* secret_id    */
                "",                             /* secret_key   */
            },
        },
        {   "authenticating config asks for populated strings.\n",
            FAIL,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "",
                "",
                "",
            },
        },
        {   "populated strings; key is the empty string?\n",
            SUCCEED,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "region",
                "me",
                "",
            },
        },
        {   "id cannot be empty.\n",
            FAIL,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "",
                "me",
                "",
            },
        },
        {   "region cannot be empty.\n",
            FAIL,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "where",
                "",
                "",
            },
        },
        {   "all strings populated.\n",
            SUCCEED,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                TRUE,
                "where",
                "who",
                "thisIsA GREAT seeeecrit",
            },
        },
        {   "incorrect version should fail\n",
            FAIL,
            {   12345,
                FALSE,
                "",
                "",
                "",
            },
        },
        {   "non-authenticating config cares not for (de)population"
            "of strings.\n",
            SUCCEED,
            {   H5FD__CURR_ROS3_FAPL_T_VERSION,
                FALSE,
                "someregion",
                "someid",
                "somekey",
            },
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

            JSVERIFY( H5FD__CURR_ROS3_FAPL_T_VERSION,
                      fa_fetch.version,
                      "invalid version number" )
            JSVERIFY( config.version,
                      fa_fetch.version,
                      "version number mismatch" )
            JSVERIFY( config.authenticate,
                      fa_fetch.authenticate,
                      "authentication flag mismatch" )
            JSVERIFY_STR( config.aws_region,
                          fa_fetch.aws_region,
                          NULL )    
            JSVERIFY_STR( config.secret_id,
                          fa_fetch.secret_id,
                          NULL )
            JSVERIFY_STR( config.secret_key,
                          fa_fetch.secret_key,
                          NULL )
        }

        /*-----------------------------
         * per-test sanitation/teardown
         *-----------------------------
         */
        FAIL_IF( FAIL == H5Pclose(fapl_id) )
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
    FAIL_IF( fapl_id < 0 )

    FAIL_IF( FAIL == H5Pset_fapl_ros3(fapl_id, &ros3_fa_0) )

    driver_id = H5Pget_driver(fapl_id);
    FAIL_IF( driver_id < 0 )

    /****************
     * Check that the VFD feature flags are correct
     * SPEC MAY CHANGE 
     ******************/

    FAIL_IF( H5FDdriver_query(driver_id, &driver_flags) < 0 )

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

    H5FD_t  *fd         = NULL;
    hbool_t  curl_ready = FALSE;
    hid_t    fapl_id    = -1;



    TESTING("ROS3 VFD-level open");

    FAIL_IF( CURLE_OK != curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    /*********
     * TESTS *
     *********/

    VFD_OPEN_VERIFY_NULL(
            "default _property list_ is not allowed",
            url_text_public,
            H5F_ACC_RDONLY, H5P_DEFAULT, MAXADDR )

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 ) /* sanity-check */

    VFD_OPEN_VERIFY_NULL(
            "generic file access property list is not allowed",
            url_text_public,
            H5F_ACC_RDONLY, H5P_DEFAULT, MAXADDR )

    /* sanity check while setting fapl
     */
    FAIL_IF( FAIL == H5Pset_fapl_ros3(fapl_id, &anonymous_fa) )

    /* filename must be valid
     */
    VFD_OPEN_VERIFY_NULL(
            "filename cannot be null",
            NULL, H5F_ACC_RDONLY, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL(
            "filename cannot be empty", 
            "", H5F_ACC_RDONLY, fapl_id, MAXADDR )

    /* File must exist at given URL/URI */
    VFD_OPEN_VERIFY_NULL(
            "file must exist",
            url_missing,
            H5F_ACC_RDWR, fapl_id, MAXADDR )

    /* only supported flag is "Read-Only"
     */
    VFD_OPEN_VERIFY_NULL(
            "read-write flag not supported",
            url_text_public,
            H5F_ACC_RDWR, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL(
            "truncate flag not supported",
            url_text_public,
            H5F_ACC_TRUNC, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL(
            "create flag not supported",
            url_text_public,
            H5F_ACC_CREAT, fapl_id, MAXADDR )
    VFD_OPEN_VERIFY_NULL(
            "EXCL flag not supported",
            url_text_public,
            H5F_ACC_EXCL, fapl_id, MAXADDR )

    /* maxaddr limitations
     */
    VFD_OPEN_VERIFY_NULL(
            "MAXADDR cannot be 0 (caught in `H5FD_open()`)",
            url_text_public,
            H5F_ACC_RDONLY, fapl_id, 0 )

    /* finally, show that a file can be opened 
     */
    fd = H5FDopen(
            url_text_public, 
            H5F_ACC_RDONLY, 
            fapl_id, 
            MAXADDR);
    FAIL_IF( NULL == fd ) 

    /************
     * TEARDOWN *
     ************/

    FAIL_IF( FAIL == H5FDclose(fd) )
    fd = NULL;

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
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

    H5FD_t  *fd_shakespeare  = NULL;
    hbool_t  curl_ready      = FALSE;
    hid_t    fapl_id         = -1;



    TESTING("ROS3 eof/eoa gets and sets");

    /*********
     * SETUP *
     *********/
    
    FAIL_IF( CURLE_OK != curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    FAIL_IF( FAIL == H5Pset_fapl_ros3(fapl_id, &restricted_access_fa) )

    fd_shakespeare = H5FDopen(
             url_text_restricted,
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

    FAIL_IF( FAIL == H5FDclose(fd_shakespeare) )

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fd_shakespeare)     (void)H5FDclose(fd_shakespeare);
    if (TRUE == curl_ready) curl_global_cleanup();
    if (fapl_id >= 0) { 
        H5E_BEGIN_TRY {
            (void)H5Pclose(fapl_id);
        } H5E_END_TRY;
    }

    return 1;

} /* test_eof_eoa */


/*-----------------------------------------------------------------------------
 *
 * Function: test_H5FDread_without_eoa_set_fails()
 * 
 * Purpose:
 * 
 *     Demonstrate a not-obvious constraint by the library, preventing
 *     file read before EoA is set
 *
 * Programmer: Jacob Smith
 *             2018-01-26
 *
 *-----------------------------------------------------------------------------
 */
static int
test_H5FDread_without_eoa_set_fails(void)
{
    char              buffer[256];
    unsigned int      i                = 0;
    H5FD_t           *file_shakespeare = NULL;
    hid_t             fapl_id          = -1;
    hid_t             dxpl_id          = -1;
    H5FD_dxpl_type_t  dxpl_type_raw    = H5FD_RAWDATA_DXPL;
    H5P_genplist_t   *dxpl_plist       = NULL;



    TESTING("ROS3 VFD read-eoa temporal coupling library limitation ");

    /*********
     * SETUP *
     *********/

    /* create ROS3 fapl 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 )
    FAIL_IF( FAIL == H5Pset_fapl_ros3(fapl_id, &restricted_access_fa) )

    /* create suitable dxpl
     */
    dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    FAIL_IF( dxpl_id < 0 )

    dxpl_plist = (H5P_genplist_t *)H5I_object(dxpl_id);
    FAIL_IF( NULL == dxpl_plist )
    FAIL_IF( FAIL == H5P_set(dxpl_plist, H5FD_DXPL_TYPE_NAME, &dxpl_type_raw) )

    /* Verify that dxpl_id reflects type setting.
     * i.e., set was successful.
     */
    {
        H5P_genplist_t   *test_plist     = NULL;
        H5FD_dxpl_type_t  test_dxpl_type = H5FD_METADATA_DXPL;

        test_plist = (H5P_genplist_t *)H5I_object(dxpl_id);
        FAIL_IF( test_plist == NULL )

        FAIL_IF( FAIL ==
                 H5P_get(test_plist,
                         H5FD_DXPL_TYPE_NAME,
                         &test_dxpl_type) )

        JSVERIFY( H5FD_RAWDATA_DXPL,
                  test_dxpl_type,
                  "sanity check: dxpl types must match" )

    }

    file_shakespeare = H5FDopen(
            url_text_restricted,
            H5F_ACC_RDONLY,
            fapl_id,
            MAXADDR);
    FAIL_IF( NULL == file_shakespeare )

    JSVERIFY( 0, H5FDget_eoa(file_shakespeare, H5FD_MEM_DEFAULT),
              "EoA should remain unset by H5FDopen" )

    for (i = 0; i < 256; i++)
        buffer[i] = 0; /* zero buffer contents */

    /********
     * TEST *
     ********/

    H5E_BEGIN_TRY { /* mute stack trace on expected failure */
        JSVERIFY( FAIL,
                  H5FDread(file_shakespeare,
                       H5FD_MEM_DRAW,
                       dxpl_id,
                       1200699,
                       102,
                       buffer),
                  "cannot read before eoa is set" )
    } H5E_END_TRY;
    JSVERIFY_STR( "", buffer, "buffer should remain untouched" )

    /************
     * TEARDOWN *
     ************/

    FAIL_IF( FAIL == H5FDclose(file_shakespeare) )
    file_shakespeare = NULL;

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
    fapl_id = -1;

    FAIL_IF( FAIL == H5Pclose(dxpl_id) )
    dxpl_id = -1;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (file_shakespeare)   { (void)H5FDclose(file_shakespeare); }
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

} /* test_H5FDread_without_eoa_set_fails */



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
    struct testcase {
        const char *message;  /* purpose of test case */
        haddr_t     eoa_set;  /* set file EOA to this prior to read */
        size_t      addr;     /* offset of read in file */
        size_t      len;      /* length of read in file */
        herr_t      success;  /* expected return value of read function */
        const char *expected; /* expected contents of buffer; failure ignores */
    };

    /************************
     * test-local variables *
     ************************/
    struct testcase cases[] = {
        {   "successful range-get",
            6464,
            5691,
            31,
            SUCCEED,
            "Quoth the Raven “Nevermore.”",
        },
        {   "read past EOA fails (EOA < EOF < addr)",
            3000,
            4000,
            100,
            FAIL,
            NULL,
        },
        {   "read overlapping EOA fails (EOA < addr < EOF < (addr+len))",
            3000,
            8000,
            100,
            FAIL,
            NULL,
        },
        {   "read past EOA/EOF fails ((EOA==EOF) < addr)",
            6464,
            7000,
            100,
            FAIL,
            NULL,
        },
        {   "read overlapping EOA/EOF fails (addr < (EOA==EOF) < (addr+len))",
            6464,
            6400,
            100,
            FAIL,
            NULL,
        },
        {   "read between EOF and EOA fails (EOF < addr < (addr+len) < EOA)",
            8000,
            7000,
            100,
            FAIL,
            NULL,
        },
    };
    unsigned          testcase_count   = 6;
    unsigned          test_i           = 0;
    struct testcase   test;
    herr_t            open_return      = FAIL;
    char              buffer[S3_TEST_MAX_URL_SIZE];
    unsigned int      i                = 0;
    H5FD_t           *file_raven       = NULL;
    hid_t             fapl_id          = -1;
    hid_t             dxpl_id          = -1;
    H5FD_dxpl_type_t  dxpl_type_raw    = H5FD_RAWDATA_DXPL;
    H5P_genplist_t   *dxpl_plist       = NULL;



    TESTING("ROS3 VFD read/range-gets");

    /*********
     * SETUP *
     *********/

    /* create ROS3 fapl 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 )
    FAIL_IF( FAIL == H5Pset_fapl_ros3(fapl_id, &restricted_access_fa) )

    /* create suitable dxpl
     */
    dxpl_id = H5Pcreate(H5P_DATASET_XFER);
    FAIL_IF( dxpl_id < 0 )

    dxpl_plist = (H5P_genplist_t *)H5I_object(dxpl_id);
    FAIL_IF( NULL == dxpl_plist )
    FAIL_IF( FAIL == H5P_set(dxpl_plist, H5FD_DXPL_TYPE_NAME, &dxpl_type_raw) )

    /* Verify that dxpl_id reflects type setting.
     * i.e., set was successful.
     */
    {
        H5P_genplist_t   *test_plist     = NULL;
        H5FD_dxpl_type_t  test_dxpl_type = H5FD_METADATA_DXPL;

        test_plist = (H5P_genplist_t *)H5I_object(dxpl_id); 
        FAIL_IF( test_plist == NULL )

        FAIL_IF( FAIL ==
                 H5P_get(test_plist, H5FD_DXPL_TYPE_NAME, &test_dxpl_type) )

        FAIL_IF( H5FD_RAWDATA_DXPL != test_dxpl_type )
    }

    /* open file 
     */
    file_raven = H5FDopen( /* will open with "authenticating" fapl */
            url_text_public,
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF); /* Demonstrate success with "automatic" value */
    FAIL_IF( NULL == file_raven )

    FAIL_IF( 6464 != H5FDget_eof(file_raven, H5FD_MEM_DEFAULT) )

    /*********
     * TESTS *
     *********/

    for (test_i = 0; test_i < testcase_count; test_i++) {

        /* -------------- *
         * per-test setup *
         * -------------- */

        test        = cases[test_i];
        open_return = FAIL;

        FAIL_IF( S3_TEST_MAX_URL_SIZE < test.len ) /* buffer too small! */

        FAIL_IF( FAIL == 
                 H5FD_set_eoa( file_raven, H5FD_MEM_DEFAULT, test.eoa_set) )

        for (i = 0; i < S3_TEST_MAX_URL_SIZE; i++) /* zero buffer contents */
            buffer[i] = 0;

        /* ------------ *
         * conduct test *
         * ------------ */

        H5E_BEGIN_TRY {
            open_return = H5FDread(
                    file_raven,
                    H5FD_MEM_DRAW,
                    dxpl_id,
                    test.addr,
                    test.len,
                    buffer);
        } H5E_END_TRY;

        JSVERIFY( test.success,
                  open_return,
                  test.message )
        if (open_return == SUCCEED)
            JSVERIFY_STR( test.expected, buffer, NULL )

    } /* for each testcase */

    /************
     * TEARDOWN *
     ************/

    FAIL_IF( FAIL == H5FDclose(file_raven) )
    file_raven = NULL;

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
    fapl_id = -1;

    FAIL_IF( FAIL == H5Pclose(dxpl_id) )
    dxpl_id = -1;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (file_raven) 
        (void)H5FDclose(file_raven);
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



    TESTING("ROS3 VFD always-fail and no-op routines");

    /*********
     * SETUP *
     *********/

    FAIL_IF( CURLE_OK != curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    /* create ROS3 fapl 
     */
    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( fapl_id < 0 )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &anonymous_fa), NULL )

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
            url_text_public,
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

    FAIL_IF( FAIL == H5FDclose(file) )
    file = NULL;

    FAIL_IF( FAIL == H5Pclose(dxpl_id) )
    dxpl_id = -1;

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
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



    TESTING("ROS3 cmp (comparison)");

    /*********
     * SETUP *
     *********/

    FAIL_IF( CURLE_OK != curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &restricted_access_fa), NULL )

    fd_raven = H5FDopen(
            url_text_public,
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == fd_raven )

    fd_shakes = H5FDopen(
            url_text_restricted,
            H5F_ACC_RDONLY,
            fapl_id,
            HADDR_UNDEF);
    FAIL_IF( NULL == fd_shakes )

    fd_raven_2 = H5FDopen(
            url_text_public,
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

    FAIL_IF( FAIL == H5FDclose(fd_raven) )
    fd_raven = NULL;
    FAIL_IF( FAIL == H5FDclose(fd_shakes) )
    fd_shakes = NULL;
    FAIL_IF( FAIL == H5FDclose(fd_raven_2) )
    fd_raven_2 = NULL;
    FAIL_IF( FAIL == H5Pclose(fapl_id) )
    fapl_id = -1;

    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * CLEANUP *
     ***********/

    if (fd_raven   != NULL)  (void)H5FDclose(fd_raven);
    if (fd_raven_2 != NULL)  (void)H5FDclose(fd_raven_2);
    if (fd_shakes  != NULL)  (void)H5FDclose(fd_shakes);
    if (TRUE == curl_ready)  curl_global_cleanup();
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



    TESTING("S3 file access through HD5F library (H5F API)");

    /*********
     * SETUP *
     *********/

    FAIL_IF( CURLE_OK != curl_global_init(CURL_GLOBAL_DEFAULT) )
    curl_ready = TRUE;

    fapl_id = H5Pcreate(H5P_FILE_ACCESS);
    FAIL_IF( 0 > fapl_id )
    JSVERIFY( SUCCEED, H5Pset_fapl_ros3(fapl_id, &restricted_access_fa), NULL )

    /*********
     * TESTS *
     *********/

    /* Read-Write access is not allowed with this file driver.
     */
    H5E_BEGIN_TRY {
        FAIL_IF( 0 <= H5Fopen(
                      url_h5_public,
                      H5F_ACC_RDWR,
                      fapl_id) )
    } H5E_END_TRY;

    /* H5Fcreate() is not allowed with this file driver.
     */
    H5E_BEGIN_TRY {
        FAIL_IF( 0 <= H5Fcreate(
                      url_missing,
                      H5F_ACC_RDONLY,
                      H5P_DEFAULT,
                      fapl_id) )
    } H5E_END_TRY;
    
    /* Successful open.
     */
    file = H5Fopen(
            url_h5_public,
            H5F_ACC_RDONLY,
            fapl_id);
    FAIL_IF( file < 0 )

    /************
     * TEARDOWN *
     ************/

    FAIL_IF( FAIL == H5Fclose(file) )
    file = -1;

    FAIL_IF( FAIL == H5Pclose(fapl_id) )
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

    /************************
     * initialize test urls *
     ************************/

    if (S3_TEST_MAX_URL_SIZE < snprintf(
            url_text_restricted, 
            (size_t)S3_TEST_MAX_URL_SIZE,
            "%s/%s", 
            (const char *)S3_TEST_BUCKET_URL, 
            (const char *)S3_TEST_RESOURCE_TEXT_RESTRICTED))
    {
        HDprintf("* ros3 setup failed (text_restricted) ! *\n");
        return 1;
    }
    if (S3_TEST_MAX_URL_SIZE < snprintf(
            url_text_public,
            (size_t)S3_TEST_MAX_URL_SIZE,
            "%s/%s",
            (const char *)S3_TEST_BUCKET_URL,
            (const char *)S3_TEST_RESOURCE_TEXT_PUBLIC))
    {
        HDprintf("* ros3 setup failed (text_public) ! *\n");
        return 1;
    }
    if (S3_TEST_MAX_URL_SIZE < snprintf(
            url_h5_public,
            (size_t)S3_TEST_MAX_URL_SIZE,
            "%s/%s",
            (const char *)S3_TEST_BUCKET_URL,
            (const char *)S3_TEST_RESOURCE_H5_PUBLIC))
    {
        HDprintf("* ros3 setup failed (h5_public) ! *\n");
        return 1;
    }
    if (S3_TEST_MAX_URL_SIZE < snprintf(
            url_missing,
            S3_TEST_MAX_URL_SIZE,
            "%s/%s",
            (const char *)S3_TEST_BUCKET_URL,
            (const char *)S3_TEST_RESOURCE_MISSING))
    {
        HDprintf("* ros3 setup failed (missing) ! *\n");
        return 1;
    }

    /******************
     * commence tests *
     ******************/

    h5_reset();

    HDprintf("Testing ros3 VFD functionality.\n");

    nerrors += test_fapl_config_validation();
    nerrors += test_ros3_fapl();
    nerrors += test_vfd_open();
    nerrors += test_eof_eoa();
    nerrors += test_H5FDread_without_eoa_set_fails();
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


