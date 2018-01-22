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
 * Purpose:    Unit tests for the S3 Communications (s3comms) module.
 *
 * Programmer: Jacob Smith <jake.smith@hdfgroup.org>
 *             2017-10-11
 */
 

#include "h5test.h"
#include "H5FDs3comms.h"
#include "H5MMprivate.h" /* memory management */

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
 *        Provided as courtesy, per consideration for inclusion in the library 
 *        proper.
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
    goto error;            \
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
/* VERIFY rountines with paramter order (<expected>, <actual> [, <msg> ])
 */


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

#if 0 /* UNUSED */

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
#endif /* JSVERIFY_NOT unused */


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

#if 0 /* UNUSED */

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
#endif /* JSVERIFY_NOT unused */


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



#define S3_TEST_BUCKET_URL "https://s3.us-east-2.amazonaws.com/hdf5ros3"
#define S3_TEST_REGION "us-east-2"
#define S3_TEST_ACCESS_ID "AKIAIMC3D3XLYXLN5COA"
#define S3_TEST_ACCESS_KEY "ugs5aVVnLFCErO/8uW14iWE3K5AgXMpsMlWneO/+"

#define S3_TEST_RESOURCE_TEXT_RESTRICTED "t8.shakespeare.txt"
#define S3_TEST_RESOURCE_TEXT_PUBLIC "Poe_Raven.txt"
#define S3_TEST_RESOURCE_H5_PUBLIC "GMODO-SVM01.h5"
#define S3_TEST_RESOURCE_MISSING "missing.csv"

#define S3_TEST_RUN_TIMEOUT 0 /* run tests that might hand */
#define S3_TEST_MAX_URL_SIZE 256 /* char array size */




/*---------------------------------------------------------------------------
 *
 * Function: test_macro_format_credential()
 *
 * Purpose:
 *
 *     Demonstrate that the macro `S3COMMS_FORMAT_CREDENTIAL`
 *     performs as expected.
 *
 * Programmer: Jacob Smith
 *             2017-09-19
 *
 *----------------------------------------------------------------------------
 */
static herr_t
test_macro_format_credential(void)
{
    /************************
     * test-local variables *
     ************************/

    char       dest[256];
    const char access[]   = "AKIAIOSFODNN7EXAMPLE";
    const char date[]     = "20130524";
    const char region[]   = "us-east-1";
    const char service[]  = "s3";
    const char expected[] = 
            "AKIAIOSFODNN7EXAMPLE/20130524/us-east-1/s3/aws4_request";



    TESTING("test_macro_format_credential");

    FAIL_IF( S3COMMS_MAX_CREDENTIAL_SIZE < 
             S3COMMS_FORMAT_CREDENTIAL(dest, access, date, region, service) )

    JSVERIFY_STR( expected, dest, NULL )

    PASSED();
    return 0;

error:
    return -1;
    
} /* test_macro_format_credential */


/*---------------------------------------------------------------------------
 *
 * Function: test_aws_canonical_request()
 *
 * Purpose:
 *
 *     Demonstrate the construction of a Canoncial Request (and Signed Headers)
 *
 *     Elided / not yet implemented:
 *         Query strings
 *         request "body"
 *
 * Programmer: Jacob Smith
 *             2017-10-04
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_aws_canonical_request(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct header {
        const char *name;
        const char *value;
    };

    struct testcase {
        const char    *exp_request;
        const char    *exp_headers;
        const char    *verb;
        const char    *resource;
        unsigned int   listsize;
        struct header  list[5];
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase  cases[]   = {
        {   "GET\n/some/path.file\n\nhost:somebucket.someserver.somedomain\nrange:bytes=150-244\n\nhost;range\ne3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "host;range",
            "GET",
            "/some/path.file",
            2,
            {   {"Range", "bytes=150-244"},
                {"Host", "somebucket.someserver.somedomain"},
            },
        },
        {   "HEAD\n/bucketpath/myfile.dat\n\nhost:place.domain\nx-amz-content-sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\nx-amz-date:19411207T150803Z\n\nhost;x-amz-content-sha256;x-amz-date\ne3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "host;x-amz-content-sha256;x-amz-date",
            "HEAD",
            "/bucketpath/myfile.dat",
            3,
            {   {"x-amz-content-sha256", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
                {"host", "place.domain"},
                {"x-amz-date", "19411207T150803Z"},
            }
        },
        {   "PUT\n/\n\n\n\ne3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            "",
            "PUT",
            "/",
            0,
            {{"",""},}, /* unused; satisfies compiler */
        },
    }; /* struct testcase cases[] */
    struct testcase *C         = NULL;
    char             cr_dest[512];     /* canonical request */
    hrb_t           *hrb       = NULL; /* http request buffer object */
    unsigned int     i         = 0;    /* looping/indexing */
    unsigned int     j         = 0;    /* looping/indexing */
    hrb_node_t      *node      = NULL; /* http headers list pointer */
    unsigned int     n_cases   = 3;
    char             sh_dest[64];       /* signed headers */



    TESTING("test_aws_canonical_request");

    for (i = 0; i < n_cases; i++) {
        /* pre-test bookkeeping
         */
        C = &cases[i];
        for (j = 0; j < 256; j++) { cr_dest[j] = 0; } /* zero request buffer */
        for (j = 0; j <  64; j++) { sh_dest[j] = 0; } /* zero headers buffer */

        /* create HTTP request object with given verb, resource/path
         */
        hrb = H5FD_s3comms_hrb_init_request(C->verb, 
                                            C->resource, 
                                            "HTTP/1.1");
        HDassert(hrb->body == NULL);

        /* Create headers list from test case input
         */
        for (j = 0; j < C->listsize; j++) {
            HDassert( SUCCEED ==
                      H5FD_s3comms_hrb_node_set( 
                              &node,
                              C->list[j].name,
                              C->list[j].value));
        }

        hrb->first_header = node;

        /* test
         */
        JSVERIFY( SUCCEED,
                  H5FD_s3comms_aws_canonical_request(cr_dest, sh_dest, hrb),
                  "unable to compose canonical request" )
        JSVERIFY_STR( C->exp_headers, sh_dest, NULL )
        JSVERIFY_STR( C->exp_request, cr_dest, NULL )

        /* tear-down
         */
        while (node != NULL) 
            HDassert(SUCCEED ==
                     H5FD_s3comms_hrb_node_set(&node, node->name, NULL));
        HDassert(NULL == node);
        HDassert( SUCCEED == H5FD_s3comms_hrb_destroy(&hrb));
        HDassert(NULL == hrb);

    } /* for each test case */

    /***************
     * ERROR CASES *
     ***************/

     /*  malformed hrb and/or node-list
      */
    JSVERIFY( FAIL, H5FD_s3comms_aws_canonical_request(cr_dest, sh_dest, NULL),
              "http request object cannot be null" )

    hrb = H5FD_s3comms_hrb_init_request("GET", "/", "HTTP/1.1");
    JSVERIFY( FAIL, H5FD_s3comms_aws_canonical_request(NULL, sh_dest, hrb),
              "canonical request destination cannot be NULL" )

    JSVERIFY( FAIL, H5FD_s3comms_aws_canonical_request(cr_dest, NULL, hrb),
              "signed headers destination cannot be null" )

    HDassert( SUCCEED == H5FD_s3comms_hrb_destroy(&hrb) );
    HDassert( NULL == hrb );

    PASSED();
    return 0;

error:

    if (node != NULL) { 
        while (node != NULL) 
            HDassert( SUCCEED ==
                      H5FD_s3comms_hrb_node_set(&node, node->name, NULL) );
    }
    if (hrb != NULL) 
        (void)H5FD_s3comms_hrb_destroy(&hrb);

    return -1;

} /* test_aws_canonical_request */


/*---------------------------------------------------------------------------
 *
 * Function: test_bytes_to_hex
 *
 * Purpose:
 *
 *     Define and verify behavior of  `H5FD_s3comms_bytes_to_hex()`.
 *
 * Return:
 *
 *     Success:  0
 *     Failure: -1
 *
 * Programmer: Jacob Smith
 *             2017-09-14
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_bytes_to_hex(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char          exp[17]; /* in size * 2 + 1 for null terminator */
        const unsigned char in[8];
        size_t              size;
        hbool_t             lower;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "52F3000C9A",
            {82,243,0,12,154},
            5,
            FALSE,
        },
        {   "009a0cf3005200", /* lowercase alphas */
            {0,154,12,243,0,82,0},
            7,
            TRUE,
        },
        {   "",
            {17,63,26,56},
            0,
            FALSE, /* irrelevant */
        },
    };
    int  i       = 0;
    int  n_cases = 3;
    char out[17];
    int  out_off = 0;



    TESTING("bytes-to-hex");

    for (i = 0; i < n_cases; i++) {
        for (out_off = 0; out_off < 17; out_off++) {
            out[out_off] = 0;
        }

        JSVERIFY( SUCCEED,
                  H5FD_s3comms_bytes_to_hex(out,
                                            cases[i].in,
                                            cases[i].size,
                                            cases[i].lower),
                  NULL )

        JSVERIFY_STR(cases[i].exp, out, NULL)
    }

    /* dest cannot be null 
     */
    JSVERIFY( FAIL, 
              H5FD_s3comms_bytes_to_hex(
                      NULL, 
                      (const unsigned char *)"nada", 
                      5, 
                      FALSE),
               "destination cannot be null" )

    PASSED();
    return 0;

error:
    return -1;

} /* test_bytes_to_hex */


/*---------------------------------------------------------------------------
 *
 * Function: test_hrb_init_request()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_hrb_init_request()`
 *
 * Programmer: Jacob Smith
 *             2017-09-20
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_hrb_init_request(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char  msg[64];
        const char *verb;
        const char *resource;
        const char *exp_res;
        const char *version;
        hbool_t     ret_null;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "get HTTP request just as we provided",
            "GET",
            "/path/to/some/file",
            "/path/to/some/file",
            "HTTP/1.1",
            FALSE,
        },
        {   "null verb substitues to GET",
            NULL,
            "/MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            "HTTP/1.1",
            FALSE,
        },
        {   "demonstrate non-GET verb",
            "HEAD",
            "/MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            "HTTP/1.1",
            FALSE,
        },
        {   "slash prepented to resource path, if necessary",
            NULL,
            "MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            NULL,
            FALSE,
        },
        {   "null resource path causes problem",
            "GET",
            NULL,
            NULL,
            NULL,
            TRUE,
        },
    };
    struct testcase *C      = NULL;
    unsigned int     i      = 0;
    unsigned int     ncases = 5;
    hrb_t           *req    = NULL;



    TESTING("hrb_init_request");

    for (i = 0; i < ncases; i++) {
        C = &cases[i];
        req = H5FD_s3comms_hrb_init_request(C->verb,
                                            C->resource,
                                            C->version);
        if (cases[i].ret_null == TRUE) {
            FAIL_IF( req != NULL );
        } else {
            FAIL_IF( req == NULL );
            JSVERIFY( S3COMMS_HRB_MAGIC, req->magic, NULL )
            if (C->verb == NULL) {
                JSVERIFY_STR( "GET", req->verb, NULL )
            } else {
                JSVERIFY_STR( req->verb, C->verb, NULL )
            }
            JSVERIFY_STR( "HTTP/1.1",  req->version,  NULL )
            JSVERIFY_STR( C->exp_res,  req->resource, NULL )
            FAIL_IF( req->first_header != NULL );
            FAIL_IF( req->body         != NULL );
            JSVERIFY( 0, req->body_len, NULL )
            JSVERIFY( SUCCEED, H5FD_s3comms_hrb_destroy(&req), 
                      "unable to destroy hrb_t" )
            FAIL_IF( NULL != req ); /* should annull pointer as well as free */
        }

    } /* for each testcase */

    PASSED();
    return 0;

error:
    (void)H5FD_s3comms_hrb_destroy(&req);

    return -1;

} /* test_hrb_init_request */


/*---------------------------------------------------------------------------
 *
 * Function: test_hrb_node_set()
 *
 * Purpose: 
 *
 *     Test operations on hrb_node_t structure
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_hrb_node_set(void)
{
    /************************
     * test-local variables *
     ************************/

    hrb_node_t *list = NULL;



    TESTING("test_hrb_node_t");

    HDassert( list == NULL );
    JSVERIFY( FAIL,
              H5FD_s3comms_hrb_node_set(&list, "Host", NULL),
              "removed a node from a null list")

    HDassert( list == NULL );
    JSVERIFY( FAIL,
              H5FD_s3comms_hrb_node_set(&list, NULL, "somevalue"),
              "created a list with NULL field name")

    HDassert( list == NULL );

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "Host", "mybucket.s3.com") ,
              "unable to create a node")
    FAIL_IF( list == NULL );
    JSVERIFY( S3COMMS_HRB_NODE_MAGIC, list->magic, NULL )
    JSVERIFY_STR( "Host",                  list->name,      NULL )
    JSVERIFY_STR( "host",                  list->lowername, NULL )
    JSVERIFY_STR( "mybucket.s3.com",       list->value,     NULL )
    JSVERIFY_STR( "Host: mybucket.s3.com", list->cat,       NULL )
    FAIL_IF( list->next != NULL)

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "x-amz-date", "20170921"),
              "unable to append node" )
    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "Range", "bytes=50-100"),
              "unable to insert node" );
    JSVERIFY_STR( "host",       list->lowername,             NULL )
    JSVERIFY_STR( "range",      list->next->lowername,       NULL )
    JSVERIFY_STR( "x-amz-date", list->next->next->lowername, NULL )
    FAIL_IF( list->next->next->next != NULL )
    JSVERIFY_STR( "bytes=50-100", list->next->value, NULL )

    JSVERIFY( SUCCEED, 
              H5FD_s3comms_hrb_node_set(&list, "Access", "always"),
              "unable to prepend node" )
    FAIL_IF( list == NULL );
    JSVERIFY_STR( "Access",     list->name,                   NULL )
    JSVERIFY_STR( "Host",       list->next->name,             NULL )
    JSVERIFY_STR( "Range",      list->next->next->name,       NULL )
    JSVERIFY_STR( "x-amz-date", list->next->next->next->name, NULL )
    FAIL_IF( list->next->next->next->next != NULL )
    JSVERIFY_STR( "bytes=50-100", list->next->next->value, NULL )

    JSVERIFY_STR( "x-amz-date: 20170921", list->next->next->next->cat, NULL)
    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "x-amz-date", "19411207"),
              "unable to modify x-amz-date value" )
    JSVERIFY_STR( "x-amz-date: 19411207", list->next->next->next->cat, NULL)

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "hoST", "none"),
              "unable to modify Host -> hoST" )
    JSVERIFY_STR( "Access: always", list->cat, NULL )
    JSVERIFY_STR( "hoST: none", list->next->cat, NULL )
    JSVERIFY_STR( "Range: bytes=50-100", list->next->next->cat, NULL )

    JSVERIFY( FAIL,
              H5FD_s3comms_hrb_node_set(&list, NULL, "somevalue"),
              "set NULL element in list" )

    /* AT THIS TIME:
     *
     * list order: [access, host, range, x-amz-date]
     *
     * now remove nodes and observe changes
     */

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "Access", NULL),
              "unable to remove first node in list (Access)" )
    JSVERIFY_STR( "hoST", list->name, NULL )
    JSVERIFY_STR( "Range", list->next->name, NULL )
    JSVERIFY_STR( "x-amz-date", list->next->next->name, NULL )
    FAIL_IF( list->next->next->next != NULL )

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "rAnGe", NULL),
              "unable to remove inner node with case-differing name" )
    JSVERIFY_STR( "hoST", list->name, NULL )
    JSVERIFY_STR( "x-amz-date", list->next->name, NULL )
    FAIL_IF( list->next->next != NULL )

    JSVERIFY( FAIL,
              H5FD_s3comms_hrb_node_set(&list, "Range", NULL),
              "no error reported when trying to remove absent node" )
    JSVERIFY_STR( "hoST", list->name, NULL )
    JSVERIFY_STR( "x-amz-date", list->next->name, NULL )
    FAIL_IF( list->next->next != NULL )

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "x-amz-date", NULL),
              "unable to remove node at end of list" )
    JSVERIFY_STR( "hoST", list->name, NULL )
    FAIL_IF( list->next != NULL )

    JSVERIFY( SUCCEED,
              H5FD_s3comms_hrb_node_set(&list, "host", NULL),
              "unable to empty list" )
    FAIL_IF( list != NULL )

    PASSED();
    return 0;

error:
    while (list != NULL) 
        HDassert(SUCCEED == H5FD_s3comms_hrb_node_set(&list, list->name, NULL));

    return -1;

} /* test_hrb_node_t */


/*---------------------------------------------------------------------------
 *
 * Function: test_HMAC_SHA256()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_HMAC_SHA256()`
 *
 * Programmer: Jacob Smith
 *             2017-09-19
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_HMAC_SHA256(void)
{

    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        herr_t               ret; /* SUCCEED/FAIL expected from call */
        const unsigned char  key[SHA256_DIGEST_LENGTH];
        size_t               key_len;
        const char          *msg;
        size_t               msg_len;
        const char          *exp; /* not used if ret == FAIL */
        size_t               dest_size; /* if 0, `dest` is not malloc'd */
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   SUCCEED,
            {   0xdb, 0xb8, 0x93, 0xac, 0xc0, 0x10, 0x96, 0x49, 
                0x18, 0xf1, 0xfd, 0x43, 0x3a, 0xdd, 0x87, 0xc7, 
                0x0e, 0x8b, 0x0d, 0xb6, 0xbe, 0x30, 0xc1, 0xfb, 
                0xea, 0xfe, 0xfa, 0x5e, 0xc6, 0xba, 0x83, 0x78,
            },
            SHA256_DIGEST_LENGTH,
            "AWS4-HMAC-SHA256\n20130524T000000Z\n20130524/us-east-1/s3/aws4_request\n7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972",
            strlen("AWS4-HMAC-SHA256\n20130524T000000Z\n20130524/us-east-1/s3/aws4_request\n7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972"),
            "f0e8bdb87c964420e857bd35b5d6ed310bd44f0170aba48dd91039c6036bdb41",
            SHA256_DIGEST_LENGTH * 2 + 1, /* +1 for null terminator */
        },
        {   SUCCEED,
            {'J','e','f','e'},
            4,
            "what do ya want for nothing?",
            28,
            "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
            SHA256_DIGEST_LENGTH * 2 + 1,
        },
        {    FAIL,
             "DOESN'T MATTER",
             14,
             "ALSO IRRELEVANT",
             15,
             NULL,
             0, /* dest -> null, resulting in immediate error */
        },
    };
    char *dest    = NULL;
    int   i       = 0;
    int   n_cases = 3;



    TESTING("HMAC_SHA256");

    for (i = 0; i < n_cases; i++) {
        if (cases[i].dest_size == 0) {
           dest = NULL;
        } else {
           dest = (char *)malloc(sizeof(char) * cases[i].dest_size);
           HDassert(dest != NULL);
        }

        JSVERIFY( cases[i].ret,
                  H5FD_s3comms_HMAC_SHA256(
                          cases[i].key,
                          cases[i].key_len,
                          cases[i].msg,
                          cases[i].msg_len,
                          dest),
                  cases[i].msg );
        if (cases[i].ret == SUCCEED) {
#ifdef VERBOSE 
            if (0 != 
                strncmp(cases[i].exp, 
                        dest, 
                        strlen(cases[i].exp)))
            {
                /* print out how wrong things are, and then fail
                 */
                dest = (char *)realloc(dest, cases[i].dest_size + 1);
                HDassert(dest != NULL);
                dest[cases[i].dest_size] = 0;
                HDfprintf(stdout, 
                          "ERROR:\n!!! \"%s\"\n != \"%s\"\n", 
                          cases[i].exp, 
                          dest);
                TEST_ERROR;
            }
#else
            /* simple pass/fail test
             */
            JSVERIFY( 0, 
                      strncmp(cases[i].exp, dest, strlen(cases[i].exp)), 
                      NULL);
#endif
        }
        free(dest);
    }

    PASSED();
    return 0;

error:
    free(dest);
    return -1;

} /* test_HMAC_SHA256 */


/*----------------------------------------------------------------------------
 *
 * Function: test_nlowercase()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_nlowercase()`
 *
 * Programmer: Jacob Smith
 *             2017-19-18
 *
 *----------------------------------------------------------------------------
 */
static herr_t
test_nlowercase(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char *in;
        size_t      len;
        const char *exp;
    };

    /************************
     * test-local variables *
     ************************/

    /* any character after in exp on or after exp[len] is undefined.
     * in this test, kept as the null character for simplicity.
     */
    struct testcase cases[] = {
        {   "HALlEluJAh",
            6,
            "hallel",
        },
        {   "all\0 lower",
            10,
            "all\0 lower",
        },
        {   "to meeeeeee",
            0,
            "",
        },
    };
    char *dest    = NULL;
    int   i       = 0;
    int   n_cases = 3;



    TESTING("nlowercase");

    for (i = 0; i < n_cases; i++) {
        dest = (char *)malloc(sizeof(char) * 16);

        JSVERIFY( SUCCEED, 
                  H5FD_s3comms_nlowercase(dest,
                                          cases[i].in,
                                          cases[i].len),
                  cases[i].in )
        if (cases[i].len > 0) {
            JSVERIFY( 0, strncmp(dest, cases[i].exp, cases[i].len), NULL )
        }
        free(dest);
    }

    JSVERIFY( FAIL,
              H5FD_s3comms_nlowercase(NULL, 
                                      cases[i].in, 
                                      cases[i].len),
              "null distination should fail" )

    PASSED();
    return 0;

error:
    free(dest);
    return -1;

} /* test_nlowercase */


/*---------------------------------------------------------------------------
 *
 * Function: test_parse_url()
 *
 * Programmer: Jacob Smith
 *             2017-11-??
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_parse_url(void)
{
    /*********************
     * test-local macros *
     *********************/

    /*************************
     * test-local structures *
     *************************/

    typedef struct {
        const char *scheme;
        const char *host;
        const char *port;
        const char *path;
        const char *query;
    } const_purl_t;

    struct testcase {
        const char   *url;
        herr_t        exp_ret; /* expected return;              */
                               /* if FAIL, `expected` is unused */
        const_purl_t  expected;
        const char   *msg;
    };

    /************************
     * test-local variables *
     ************************/

    parsed_url_t   *purl    = NULL;
    unsigned int    i       = 0;
    unsigned int    ncases  = 15;
    struct testcase cases[] = {
        {   NULL,
            FAIL,
            { NULL, NULL, NULL, NULL, NULL },
            "null url",
        },
        {   "",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL },
            "empty url",
        },
        {   "ftp://[1000:4000:0002:2010]",
            SUCCEED,
            {   "ftp",
                "[1000:4000:0002:2010]",
                NULL,
                NULL,
                NULL,
            },
            "IPv6 ftp and empty path (root)",
        },
        {   "ftp://[1000:4000:0002:2010]:2040",
            SUCCEED,
            {   "ftp",
                "[1000:4000:0002:2010]",
                "2040",
                NULL,
                NULL,
            },
            "root IPv6 ftp with port",
        },
        {   "http://some.domain.org:9000/path/to/resource.txt",
            SUCCEED,
            {   "http",
                "some.domain.org",
                "9000",
                "path/to/resource.txt",
                NULL,
            },
            "without query",
        },
        {   "https://domain.me:00/file.txt?some_params unchecked",
            SUCCEED,
            {   "https",
                "domain.me",
                "00",
                "file.txt",
                "some_params unchecked",
            },
            "with query",
        },
        {   "ftp://domain.com/",
            SUCCEED,
            {   "ftp",
                "domain.com",
                NULL,
                NULL,
                NULL,
            },
            "explicit root w/out port",
        },
        {   "ftp://domain.com:1234/",
            SUCCEED,
            {   "ftp",
                "domain.com",
                "1234",
                NULL,
                NULL,
            },
            "explicit root with port",
        },
        {   "ftp://domain.com:1234/file?",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL, },
            "empty query is invalid",
        },
        {   "ftp://:1234/file",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL, },
            "no host",
        },
        {   "h&r block",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL, },
            "no scheme (bad URL)",
        },
        {   "http://domain.com?a=b&d=b",
            SUCCEED,
            {   "http", 
                "domain.com", 
                NULL, 
                NULL, 
                "a=b&d=b", 
            },
            "QUERY with implict PATH",
        },
        {   "http://[5]/path?a=b&d=b",
            SUCCEED,
            {   "http", 
                "[5]", 
                NULL, 
                "path", 
                "a=b&d=b", 
            },
            "IPv6 extraction is really dumb",
        },
        {   "http://[1234:5678:0910:1112]:port/path",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL, },
            "non-decimal PORT (port)",
        },
        {   "http://mydomain.com:01a3/path",
            FAIL,
            { NULL, NULL, NULL, NULL, NULL, },
            "non-decimal PORT (01a3)",
        },
    };



    TESTING("url-parsing functionality");

    /*********
     * TESTS *
     *********/

    for (i = 0; i < ncases; i++) {
        HDassert( purl == NULL );

        JSVERIFY( cases[i].exp_ret,
                  H5FD_s3comms_parse_url(cases[i].url, &purl),
                  cases[i].msg )

        if (cases[i].exp_ret == FAIL) {
            /* on FAIL, `purl` should be untouched--remains NULL */
            FAIL_UNLESS( purl == NULL )
        } else {
            /* on SUCCEED, `purl` should be set */
            FAIL_IF( purl == NULL )

            if (cases[i].expected.scheme != NULL) {
                FAIL_IF( NULL == purl->scheme )
                JSVERIFY_STR( cases[i].expected.scheme, 
                              purl->scheme, 
                              cases[i].msg )
            } else {
                FAIL_UNLESS( NULL == purl->scheme )
            }

            if (cases[i].expected.host != NULL) {
                FAIL_IF( NULL == purl->host )
                JSVERIFY_STR( cases[i].expected.host, 
                              purl->host, 
                              cases[i].msg )
            } else {
                FAIL_UNLESS( NULL == purl->host )
            }

            if (cases[i].expected.port != NULL) {
                FAIL_IF( NULL == purl->port )
                JSVERIFY_STR( cases[i].expected.port, 
                              purl->port, 
                              cases[i].msg )
            } else {
                FAIL_UNLESS( NULL == purl->port )
            }

            if (cases[i].expected.path != NULL) {
                FAIL_IF( NULL == purl->path )
                JSVERIFY_STR( cases[i].expected.path, 
                              purl->path, 
                              cases[i].msg )
            } else {
                FAIL_UNLESS( NULL == purl->path )
            }

            if (cases[i].expected.query != NULL) {
                FAIL_IF( NULL == purl->query )
                JSVERIFY_STR( cases[i].expected.query, 
                              purl->query, 
                              cases[i].msg )
            } else {
                FAIL_UNLESS( NULL == purl->query )
            }
        } /* if parse-url return SUCCEED/FAIL */

        /* per-test cleanup
         * well-behaved, even if `purl` is NULL 
         */
        HDassert( SUCCEED == H5FD_s3comms_free_purl(purl) );
        purl = NULL;

    } /* for each testcase */

    PASSED();
    return 0;

error:
    /***********
     * cleanup *
     ***********/
    (void)H5FD_s3comms_free_purl(purl);

    return -1;

} /* test_parse_url */


/*---------------------------------------------------------------------------
 *
 * Function: test_percent_encode_char()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_percent_encode_char()`
 *
 * Return:
 *
 *     Success:  0
 *     Failure: -1
 *
 * Programmer: Jacob Smith
 *             2017-09-14
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_percent_encode_char(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char  c;
        const char *exp;
        size_t      exp_len;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {'$', "%24", 3}, /* u+0024 dollar sign */
        {' ', "%20", 3}, /* u+0020 space */
        {'^', "%5E", 3}, /* u+0094 carat */
        {'/', "%2F", 3}, /* u+002f solidus (forward slash) */
        /* {??, "%C5%8C", 6},*/ /* u+014c Latin Capital Letter O with Macron */
        /* Not included because it is multibyte "wide" character that poses  */
        /* issues both in the underlying function and in being written in    */
        /* this file.                                                        */
        /* {'¢', "%C2%A2", 6}, */ /* u+00a2 cent sign */
        /* above works, but complains about wide character overflow      */
        /* Elide for now, until it is determined (a) unnecessary or      */
        /* (b) requiring signature change to accommodate wide characters */
        {'\0', "%00", 3}, /* u+0000 null */
    };
    char   dest[13];
    size_t dest_len = 0;
    int    i        = 0;
    int    n_cases  = 5;



    TESTING("percent encode characters");

    for (i = 0; i < n_cases; i++) {
        JSVERIFY( SUCCEED, 
                  H5FD_s3comms_percent_encode_char(
                          dest, 
                          (const unsigned char)cases[i].c, 
                          &dest_len),
                  NULL )
        JSVERIFY(cases[i].exp_len, dest_len, NULL ) 
        JSVERIFY(0, strncmp(dest, cases[i].exp, dest_len), NULL ) 
        JSVERIFY_STR( cases[i].exp, dest, NULL )
    }

    JSVERIFY( FAIL, 
              H5FD_s3comms_percent_encode_char(
                      NULL, 
                      (const unsigned char)'^', 
                      &dest_len),
              NULL )

    PASSED();
    return 0;

error:
    return -1;

} /* test_percent_encode_char */



/*---------------------------------------------------------------------------
 *---------------------------------------------------------------------------
 */
static herr_t
test_s3r_open(void)
{

    /************************
     * test-local variables *
     ************************/

    char           url_missing[S3_TEST_MAX_URL_SIZE];
    char           url_raven[S3_TEST_MAX_URL_SIZE];
    char           url_raven_badport[S3_TEST_MAX_URL_SIZE];
    char           url_shakespeare[S3_TEST_MAX_URL_SIZE];
    unsigned char  signing_key[SHA256_DIGEST_LENGTH];
    struct tm     *now          = NULL;
    char           iso8601now[ISO8601_SIZE];
    s3r_t         *handle       = NULL;
    hbool_t        curl_ready   = FALSE;
    parsed_url_t  *purl         = NULL;



    TESTING("s3r_open");

    /******************
     * PRE-TEST SETUP *
     ******************/

    FAIL_IF( S3_TEST_MAX_URL_SIZE < 
             snprintf(url_shakespeare, 
                      S3_TEST_MAX_URL_SIZE,
                      "%s/%s", 
                      S3_TEST_BUCKET_URL, 
                      S3_TEST_RESOURCE_TEXT_RESTRICTED) );

    FAIL_IF( S3_TEST_MAX_URL_SIZE <
             snprintf(url_missing,
                      S3_TEST_MAX_URL_SIZE,
                      "%s/%s", 
                      S3_TEST_BUCKET_URL, 
                      "missing.csv") );

    FAIL_IF( S3_TEST_MAX_URL_SIZE <
             snprintf(url_raven,
                      S3_TEST_MAX_URL_SIZE,
                      "%s/%s",
                      S3_TEST_BUCKET_URL, 
                      "Poe_Raven.txt") );

    /* Set given bucket url with invalid/inactive port number for badport.
     * Note, this sort of micro-management of parsed_url_t is not advised
     */
    FAIL_IF( FAIL == H5FD_s3comms_parse_url(S3_TEST_BUCKET_URL, &purl) )
    if (purl->port == NULL) {
        purl->port = H5MM_malloc(sizeof(char) * 5);
        FAIL_IF( purl->port == NULL );
        FAIL_IF( 5 < snprintf(purl->port, 5, "9000") )
    } else if (strcmp(purl->port, "9000") != 0) {
        FAIL_IF( 5 < snprintf(purl->port, 5, "9000") )
    } else {
        FAIL_IF( 5 < snprintf(purl->port, 5, "1234") )
    }
    FAIL_IF( S3_TEST_MAX_URL_SIZE < 
             snprintf(url_raven_badport, 
                      S3_TEST_MAX_URL_SIZE,
                      "%s://%s:%s/%s",
                      purl->scheme,
                      purl->host,
                      purl->port,
                      "Poe_Raven.txt") );
//TODO: nix all HDasserts

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ready = TRUE;

    now = gmnow();
    HDassert(now != NULL);
    HDassert(ISO8601NOW(iso8601now, now) == (ISO8601_SIZE - 1)); 

    /* It is desired to have means available to verify that signing_key
     * was set successfully and to an expected value.
     */
    HDassert( SUCCEED ==
              H5FD_s3comms_signing_key(
                      signing_key, 
                      (const char *)S3_TEST_ACCESS_KEY,
                      (const char *)S3_TEST_REGION,
                      (const char *)iso8601now) );

    /*************************
     * OPEN NONEXISTENT FILE *
     *************************/

    /* attempt anonymously
     */
    handle = H5FD_s3comms_s3r_open(url_missing, NULL, NULL, NULL); 
    FAIL_IF( handle != NULL );

    /* attempt with authentication
     */
    handle = H5FD_s3comms_s3r_open(
             url_missing,
             (const char *)S3_TEST_REGION,
             (const char *)S3_TEST_ACCESS_ID,
             (const unsigned char *)signing_key);
    FAIL_IF( handle != NULL );

    /**************************
     * INACTIVE PORT  ON HOST *
     **************************/

#if S3_TEST_RUN_TIMEOUT
printf("Opening on inactive port may hang for a minute; waiting for timeout\n");
    handle = H5FD_s3comms_s3r_open(url_raven_badport, NULL, NULL, NULL);
    FAIL_IF( handle != NULL );
#endif

    /*******************************
     * INVALID AUTHENTICATION INFO *
     *******************************/

    /* anonymous access on restricted file
     */
    handle = H5FD_s3comms_s3r_open(url_shakespeare, NULL, NULL, NULL);
    FAIL_IF( handle != NULL );

    /* passed in a bad ID 
     */
    handle = H5FD_s3comms_s3r_open(
             url_shakespeare,
             (const char *)S3_TEST_REGION,
             "I_MADE_UP_MY_ID",
             (const unsigned char *)signing_key);
    FAIL_IF( handle != NULL );

    /* using an invalid signing key
     */
    handle = H5FD_s3comms_s3r_open(
             url_shakespeare,
             (const char *)S3_TEST_REGION,
             (const char *)S3_TEST_ACCESS_ID,
             (const unsigned char *)EMPTY_SHA256);
    FAIL_IF( handle != NULL );

    /*******************************
     * SUCCESSFUL OPEN (AND CLOSE) *
     *******************************/

    /* anonymous 
     */
    handle = H5FD_s3comms_s3r_open(url_raven, NULL, NULL, NULL);
    FAIL_IF( handle == NULL );
    JSVERIFY( 6464, handle->filesize, NULL )
    JSVERIFY( SUCCEED, 
              H5FD_s3comms_s3r_close(handle),
              "unable to close file" )
    handle = NULL;
    
    /* authenticating
     */
    handle = H5FD_s3comms_s3r_open(
                     url_shakespeare,
                     (const char *)S3_TEST_REGION,
                     (const char *)S3_TEST_ACCESS_ID,
                     (const unsigned char *)signing_key);
    FAIL_IF( handle == NULL );
    JSVERIFY( 5458199, handle->filesize, NULL )
    JSVERIFY( SUCCEED, 
              H5FD_s3comms_s3r_close(handle),
              "unable to close file" )
    handle = NULL;

    /* using authentication on anonymously-accessible file?
     */
    handle = H5FD_s3comms_s3r_open(
             url_raven,
             (const char *)S3_TEST_REGION,
             (const char *)S3_TEST_ACCESS_ID,
             (const unsigned char *)signing_key);
    FAIL_IF( handle == NULL );
    JSVERIFY( 6464, handle->filesize, NULL )
    JSVERIFY( SUCCEED, 
              H5FD_s3comms_s3r_close(handle),
              "unable to close file" )
    handle = NULL;
    
    /* authenticating
     */
    handle = H5FD_s3comms_s3r_open(
                     url_shakespeare,
                     (const char *)S3_TEST_REGION,
                     (const char *)S3_TEST_ACCESS_ID,
                     (const unsigned char *)signing_key);
    FAIL_IF( handle == NULL );
    JSVERIFY( 5458199, handle->filesize, NULL )
    JSVERIFY( SUCCEED, 
              H5FD_s3comms_s3r_close(handle),
              "unable to close file" )
    handle = NULL;



    curl_global_cleanup();
    curl_ready = FALSE;

    FAIL_IF( FAIL == H5FD_s3comms_free_purl(purl) )
    purl = NULL;

    PASSED();
    return 0;
error:
    /***********
     * cleanup *
     ***********/

    if (handle != NULL)
        H5FD_s3comms_s3r_close(handle); 
    if (purl != NULL)
        H5FD_s3comms_free_purl(purl);
    if (curl_ready == TRUE)
        curl_global_cleanup();

    return -1;
} /* test_s3r_open */


/*---------------------------------------------------------------------------
 *
 * Function: test_s3r_read()
 *
 * Purpose:
 *
 *     Specify and demonstrate the use and life cycle of an S3 Request handle
 *     `s3r_t`, through its related functions.
 *
 *     H5FD_s3comms_s3r_open
 *     H5FD_s3comms_s3r_getsize << called by open() _only_
 *     H5FD_s3comms_s3r_read    << called by getsize(), multiple times working
 *     H5FD_s3comms_s3r_close  
 *
 *     Shows most basic curl interation.
 *
 * Programmer: Jacob Smith
 *             2017-10-06
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_s3r_read(void)
{
#define S3COMMS_TEST_BUFFER_SIZE 256
    /************************
     * test-local variables *
     ************************/

    char           url_raven[S3_TEST_MAX_URL_SIZE];
    char           buffer[S3COMMS_TEST_BUFFER_SIZE];
    char           buffer2[S3COMMS_TEST_BUFFER_SIZE];
    s3r_t         *handle     = NULL;
    hbool_t        curl_ready = FALSE;
    unsigned int   i          = 0;



    TESTING("test_s3r_read");

    /*
     * initial setup
     */

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ready = TRUE;
    FAIL_IF( S3_TEST_MAX_URL_SIZE < 
             snprintf(url_raven, 
                      S3_TEST_MAX_URL_SIZE,
                      "%s/%s", 
                      S3_TEST_BUCKET_URL, 
                      "Poe_Raven.txt") );

    for (i = 0; i < S3COMMS_TEST_BUFFER_SIZE; i++) 
        buffer[i] = '\0';

    /* open file
     */
    handle = H5FD_s3comms_s3r_open(url_raven, NULL, NULL, NULL);
    FAIL_IF( handle == NULL )
    FAIL_IF( handle->filesize != 6464 )

    for (i = 0; i < S3COMMS_TEST_BUFFER_SIZE; i++) 
        buffer[i] = '\0';

    /********************** 
     * read start of file *
     **********************/

    JSVERIFY( SUCCEED,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)0,
                      (size_t)117,
                      buffer),
              NULL )
    JSVERIFY( 0,
              strncmp(buffer,
                      "Once upon a midnight dreary, while I pondered, weak and weary,\nOver many a quaint and curious volume of forgotten lore",
                      117),
              buffer )

    for (i = 0; i < S3COMMS_TEST_BUFFER_SIZE; i++) 
        buffer[i] = '\0';

    /************************
     * read arbitrary range *
     ************************/

    JSVERIFY( SUCCEED,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)2540,
                      (size_t)53,
                      buffer),
              NULL )
    JSVERIFY( 0,
              strncmp(buffer,
                      "the grave and stern decorum of the countenance it wore",
                      53),
              buffer )

    for (i = 0; i < S3COMMS_TEST_BUFFER_SIZE; i++) 
        buffer[i] = '\0';

    /***************
     * read to EoF *
     ***************/

    JSVERIFY( SUCCEED,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)6370,
                      (size_t)0,
                      buffer),
              NULL )
    JSVERIFY( 0,
              strncmp(buffer,
                      "And my soul from out that shadow that lies floating on the floor\nShall be lifted—nevermore!\n",
                      64),
              buffer )

    for (i = 0; i < S3COMMS_TEST_BUFFER_SIZE; i++) 
        buffer[i] = '\0';

    /*****************
     * read past eof *
     *****************/

    JSVERIFY( FAIL,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)6400,
                      (size_t)100, /* 6400+100 > 6464 */
                      buffer),
              NULL )
     JSVERIFY( 0, strcmp("", buffer), NULL )

    /************************
     * read starts past eof *
     ************************/

    JSVERIFY( FAIL,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)1200699, /* 1200699 > 6464 */
                      (size_t)100,
                      buffer),
              NULL )
     JSVERIFY( 0, strcmp("", buffer), NULL )

    /**********************
     * read starts on eof *
     **********************/

    JSVERIFY( FAIL,
              H5FD_s3comms_s3r_read(
                      handle,
                      (haddr_t)6464,
                      (size_t)0,
                      buffer),
              NULL )
     JSVERIFY( 0, strcmp("", buffer), NULL )

    /*************
     * TEAR DOWN *
     *************/

    JSVERIFY( SUCCEED,
              H5FD_s3comms_s3r_close(handle),
              "unable to close file" )
    handle = NULL;

    HDassert( curl_ready == TRUE );
    curl_global_cleanup();
    curl_ready = FALSE;

    PASSED();
    return 0;

error:
    /***********
     * cleanup *
     ***********/

    if (handle != NULL)
        H5FD_s3comms_s3r_close(handle); 

    if (curl_ready == TRUE)
        curl_global_cleanup();

    return -1;

#undef S3COMMS_TEST_BUFFER_SIZE

} /* test_s3r_read */


/*---------------------------------------------------------------------------
 *
 * Function: test_signing_key()
 *
 * Purpose:
 * 
 *     Define and verify behavior of `H5FD_s3comms_signing_key()`
 *
 *     More test cases would be a very good idea.
 *
 * Programmer: Jacob Smith
 *             2017-09-18
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_signing_key(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char    *region;
        const char    *secret_key;
        const char    *when;
        unsigned char  exp[SHA256_DIGEST_LENGTH];
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "us-east-1",
            "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY",
            "20130524T000000Z",
            {   0xdb, 0xb8, 0x93, 0xac, 0xc0, 0x10, 0x96, 0x49, 
                0x18, 0xf1, 0xfd, 0x43, 0x3a, 0xdd, 0x87, 0xc7, 
                0x0e, 0x8b, 0x0d, 0xb6, 0xbe, 0x30, 0xc1, 0xfb, 
                0xea, 0xfe, 0xfa, 0x5e, 0xc6, 0xba, 0x83, 0x78,
            },
        },
    };
    int            i      = 0;
    unsigned char *key    = NULL;
    int            ncases = 1;



    TESTING("signing_key");

    for (i = 0; i < ncases; i++) {
        key = (unsigned char *)malloc(sizeof(unsigned char) * \
                                      SHA256_DIGEST_LENGTH);
    HDassert(key != NULL);

        JSVERIFY( SUCCEED,
                  H5FD_s3comms_signing_key(
                          key,
                          cases[i].secret_key,
                          cases[i].region,
                          cases[i].when),
                  NULL )

        JSVERIFY( 0,
                  strncmp((const char *)cases[i].exp, 
                          (const char *)key,
                          SHA256_DIGEST_LENGTH),
                   cases[i].exp )

        free(key);
        key = NULL;
    }


    /***************
     * ERROR CASES *
     ***************/

    key = (unsigned char *)malloc(sizeof(unsigned char) * \
                                  SHA256_DIGEST_LENGTH);
    HDassert(key != NULL);

    JSVERIFY( FAIL,
              H5FD_s3comms_signing_key(
                      NULL,
                      cases[0].secret_key,
                      cases[0].region,
                      cases[0].when),
              "destination cannot be NULL" )

    JSVERIFY( FAIL,
              H5FD_s3comms_signing_key(
                      key,
                      NULL,
                      cases[0].region,
                      cases[0].when),
              "secret key cannot be NULL" )

    JSVERIFY( FAIL,
              H5FD_s3comms_signing_key(
                      key,
                      cases[0].secret_key,
                      NULL,
                      cases[0].when),
              "aws region cannot be NULL" )

    JSVERIFY( FAIL,
              H5FD_s3comms_signing_key(
                      key,
                      cases[0].secret_key,
                      cases[0].region,
                      NULL),
              "time string cannot be NULL" )

    free(key);
    key = NULL;

    PASSED();
    return 0;

error:
    if (key != NULL) {
        free(key);
    }

    return -1;

} /* test_signing_key */


/*---------------------------------------------------------------------------
 *
 * Function: test_tostringtosign()
 *
 * Purpose:
 *
 *     Verify that we can get the "string to sign" from a Canonical Request and
 *     related information.
 *
 *     Demonstrate failure cases.
 *
 * Return:    
 *
 *     Success:  0
 *     Failure: -1
 *
 * Programmer: Jacob Smith
 *             2017-09-13
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_tostringtosign(void)
{

    /************************
     * test-local variables *
     ************************/

    const char canonreq[]   = "GET\n/test.txt\n\nhost:examplebucket.s3.amazonaws.com\nrange:bytes=0-9\nx-amz-content-sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\nx-amz-date:20130524T000000Z\n\nhost;range;x-amz-content-sha256;x-amz-date\ne3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    const char iso8601now[] = "20130524T000000Z";
    const char region[]     = "us-east-1";
    char s2s[512];



    TESTING("s3comms tostringtosign");

    JSVERIFY( SUCCEED,
              H5FD_s3comms_tostringtosign(s2s, canonreq, iso8601now, region),
              "unable to create string to sign" )

    JSVERIFY_STR( "AWS4-HMAC-SHA256\n20130524T000000Z\n20130524/us-east-1/s3/aws4_request\n7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972", 
                 s2s, NULL ) 

    JSVERIFY( FAIL,
              H5FD_s3comms_tostringtosign(s2s, NULL, iso8601now, region),
              "canonical request string cannot be NULL" )

    JSVERIFY( FAIL,
              H5FD_s3comms_tostringtosign(s2s, canonreq, NULL, region),
              "time string cannot be NULL" )

    JSVERIFY( FAIL,
              H5FD_s3comms_tostringtosign(s2s, canonreq, iso8601now, NULL),
              "aws region cannot be NULL" )

    PASSED();
    return 0;

error :
    return -1;

} /* test_tostringtosign */


/*----------------------------------------------------------------------------
 *
 * Function: test_trim()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_trim()`.
 *
 * Programmer: Jacob Smith
 *             2017-09-14
 *
 *----------------------------------------------------------------------------
 */
static herr_t
test_trim(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char *in;
        size_t      in_len;
        const char *exp;
        size_t      exp_len;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "block string",
            12,
            "block string",
            12,
        },
        {   " \n\r  \t",
            6,
            "",
            0,
        },
        {   " \twhite b4",
            10,
            "white b4",
            8,
        },
        {   "white after\r\n  ",
            15,
            "white after",
            11,
        },
        {   " on\nends\t",
            9,
            "on\nends",
            7,
        },
    };
    char    dest[32];
    size_t  dest_len = 0;
    int     i        = 0;
    int     n_cases  = 5;
    char   *str      = NULL;



    TESTING("s3comms trim");

    for (i = 0; i < n_cases; i++) {
        HDassert(str == NULL);
        str = (char *)malloc(sizeof(char) * cases[i].in_len);
        HDassert(str != NULL);
        strncpy(str, cases[i].in, cases[i].in_len);

        JSVERIFY( SUCCEED,
                  H5FD_s3comms_trim(dest, str, cases[i].in_len, &dest_len),
                  NULL )
        JSVERIFY( cases[i].exp_len, dest_len, cases[i].in )
        if (dest_len > 0) {
           JSVERIFY( 0, strncmp(cases[i].exp, dest, dest_len),
                     cases[i].exp )
        }
        free(str);
        str = NULL;
    }

    JSVERIFY( SUCCEED, H5FD_s3comms_trim(dest, NULL, 3, &dest_len),
              "should not fail when trimming a null string" );
    JSVERIFY( 0, dest_len, "trimming NULL string writes 0 characters" )
    
    HDassert(str == NULL);
    str = (char *)malloc(sizeof(char *) * 11);
    HDassert(str != NULL);
    memcpy(str, "some text ", 11); /* string with null terminator */
    JSVERIFY( FAIL, H5FD_s3comms_trim(NULL, str, 10, &dest_len),
              "destination for trim cannot be NULL" );
    free(str);
    str = NULL;

    PASSED();
    return 0;

error:
    if (str != NULL) 
        free(str);
    return -1;

} /* test_trim */


/*----------------------------------------------------------------------------
 *
 * Function: test_uriencode()
 *
 * Purpose:
 *
 *     Define and verify behavior of `H5FD_s3comms_uriencode()`.
 *
 * Programmer: Jacob Smith
 *             2017-09-14
 *
 *----------------------------------------------------------------------------
 */
static herr_t
test_uriencode(void)
{
    /*************************
     * test-local structures *
     *************************/

    struct testcase {
        const char *str;
        size_t      s_len;
        hbool_t     encode_slash;
        const char *expected;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "/path/to/resource.jpg",
            21,
            FALSE,
            "/path/to/resource.jpg",
        },
        {   "/path/to/resource.jpg",
            21,
            TRUE,
            "%2Fpath%2Fto%2Fresource.jpg",
        },
        {   "string got_spaa  ces",
            20,
            TRUE,
            "string%20got_spaa%20%20ces",
        },
        {   "sp ac~es/and-sl ash.encoded",
            27,
            TRUE,
            "sp%20ac~es%2Fand-sl%20ash.encoded",
        },
        {   "sp ac~es/and-sl ash.unencoded",
            29,
            FALSE,
            "sp%20ac~es/and-sl%20ash.unencoded",
        },
        {   "/path/to/resource.txt",
            0,
            FALSE,
            "",
            
        }
    };
    char   *dest         = NULL; 
    size_t  dest_written = 0;
    int     i            = 0;
    int     ncases       = 6;
    size_t  str_len      = 0;



    TESTING("s3comms uriencode")

    for (i = 0; i < ncases; i++) {
        str_len = cases[i].s_len;
        dest = (char *)malloc(sizeof(char) * str_len * 3 + 1);
        HDassert(dest != NULL);

        JSVERIFY( SUCCEED,
                  H5FD_s3comms_uriencode(dest, 
                                         cases[i].str, 
                                         str_len,
                                         cases[i].encode_slash,
                                         &dest_written),
                  NULL );

        JSVERIFY( strlen(cases[i].expected), dest_written, NULL )
        JSVERIFY( 0, strncmp(dest, cases[i].expected, dest_written),
                  cases[i].expected );

        free(dest);
        dest = NULL;
    }

    /***************
     * ERROR CASES *
     ***************/

    dest = (char *)malloc(sizeof(char) * 15);
    HDassert(dest != NULL);

    JSVERIFY( FAIL,
              H5FD_s3comms_uriencode(NULL, "word$", 5, false, &dest_written),
              "destination cannot be NULL" );
    JSVERIFY( FAIL,
              H5FD_s3comms_uriencode(dest, NULL, 5, false, &dest_written),
              "source string cannot be NULL" );

    free(dest);
    dest = NULL;

    PASSED();
    return 0;

error:
    if (dest != NULL) {
        free(dest);
    }
    return -1;

} /* test_uriencode */




/*-------------------------------------------------------------------------
 * Function: main()
 *
 * Purpose:     
 *
 *     Run unit tests for S3 Communications (s3comms).
 *
 * Return:      
 *
 *     Success: 0
 *     Failure: 1
 *
 * Programmer:  Jacob Smith
 *              2017-10-12
 *
 *-------------------------------------------------------------------------
 */
int
main(void)
{
    int nerrors = 0;

    h5_reset();

    HDprintf("Testing S3Communications functionality.\n");

    /* tests ordered rougly by dependence */
    nerrors += test_macro_format_credential() < 0 ? 1 : 0;
    nerrors += test_trim()                    < 0 ? 1 : 0;
    nerrors += test_nlowercase()              < 0 ? 1 : 0;
    nerrors += test_uriencode()               < 0 ? 1 : 0;
    nerrors += test_percent_encode_char()     < 0 ? 1 : 0;
    nerrors += test_bytes_to_hex()            < 0 ? 1 : 0;
    nerrors += test_HMAC_SHA256()             < 0 ? 1 : 0;
    nerrors += test_signing_key()             < 0 ? 1 : 0;
    nerrors += test_hrb_node_set()            < 0 ? 1 : 0;
    nerrors += test_hrb_init_request()        < 0 ? 1 : 0;
    nerrors += test_parse_url()               < 0 ? 1 : 0;
    nerrors += test_aws_canonical_request()   < 0 ? 1 : 0;
    nerrors += test_tostringtosign()          < 0 ? 1 : 0;
    nerrors += test_s3r_open()                < 0 ? 1 : 0;
    nerrors += test_s3r_read()                < 0 ? 1 : 0;

    if(nerrors) {
        HDprintf("***** %d S3comms TEST%s FAILED! *****\n",
                 nerrors,
                 nerrors > 1 ? "S" : "");
        return 1;
    } /* end if */

    HDprintf("All S3comms tests passed.\n");

    return 0;
} /* end main() */

