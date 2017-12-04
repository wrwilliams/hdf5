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



#ifndef __js_test__

#define __js_test__ 1L

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
 *         JSVERIY_STR()    - string equality check; prints
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


#endif /* __js_test__ */



#if 0
/* ##########################################################################
 *
 * TEST PROTOTYPE. from top: Y9}
 *
 * ##########################################################################
 */

/*---------------------------------------------------------------------------
 *
 * Function: funcname()
 *
 * Programmer: Jacob Smith
 *             yyyy-MM-DD
 *
 *---------------------------------------------------------------------------
 */
static herr_t
funcname(void)
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

    TESTING("funcname");

    /*********
     * TESTS *
     *********/

    PASSED();
    return 0;

error:
    /***********
     * cleanup *
     ***********/

    return -1;

} /* funcname */

#endif


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

        /* prepare objects
         */
        hrb = H5FD_s3comms_hrb_init_request(C->verb, 
                                            C->resource, 
                                            "HTTP/1.1");
        HDassert(hrb->body == NULL);

        /* add headers to list 
         */
        for (j = 0; j < C->listsize; j++) {
            node = H5FD_s3comms_hrb_node_set(node, 
                                             C->list[j].name,
                                             C->list[j].value);
        }

        /* get first node (sorted)
         */
        node = H5FD_s3comms_hrb_node_first(node, HRB_NODE_ORD_SORTED);

        hrb->first_header = node;

        /* test
         */
        FAIL_IF( FAIL == 
                 H5FD_s3comms_aws_canonical_request(cr_dest, sh_dest, hrb) );
        JSVERIFY_STR( C->exp_headers, sh_dest, NULL )
        JSVERIFY_STR( C->exp_request, cr_dest, NULL )

        /* tear-down
         */
        H5FD_s3comms_hrb_node_destroy(node);
        node = NULL;
        H5FD_s3comms_hrb_destroy(hrb);
        hrb = NULL;        

    } /* for each test case */

    /***************
     * ERROR CASES *
     ***************/

     /*  malformed hrb and/or node-list
      */
    FAIL_IF( FAIL != 
             H5FD_s3comms_aws_canonical_request(cr_dest, sh_dest, NULL) );

    hrb = H5FD_s3comms_hrb_init_request("GET", "/", "HTTP/1.1");
    FAIL_IF( FAIL != 
             H5FD_s3comms_aws_canonical_request(NULL, sh_dest, hrb) );

    FAIL_IF( FAIL != 
             H5FD_s3comms_aws_canonical_request(cr_dest, NULL, hrb) );

    H5FD_s3comms_hrb_destroy(hrb);
    hrb = NULL;

    PASSED();
    return 0;

error:

    if (node != NULL) { H5FD_s3comms_hrb_node_destroy(node); }
    if (hrb  != NULL) { H5FD_s3comms_hrb_destroy(hrb); }

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

        FAIL_IF( FAIL == 
                 H5FD_s3comms_bytes_to_hex(out,
                                           cases[i].in,
                                           cases[i].size,
                                           cases[i].lower) );

        JSVERIFY_STR(cases[i].exp, out, NULL)
    }

    /* dest cannot be null 
     */
    FAIL_IF( FAIL != 
             H5FD_s3comms_bytes_to_hex(NULL, 
                                       (const unsigned char *)"nada", 
                                       5, 
                                       FALSE) );

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
        {   "GET",
            "/path/to/some/file",
            "/path/to/some/file",
            "HTTP/1.1",
            FALSE,
        },
        {   NULL,
            "/MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            "HTTP/1.1",
            FALSE,
        },
        {   "HEAD",
            "/MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            "HTTP/1.1",
            FALSE,
        },
        {   NULL,
            "MYPATH/MYFILE.tiff",
            "/MYPATH/MYFILE.tiff",
            NULL,
            FALSE,
        },
        {   "GET",
            NULL, /* problem path! */
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
            FAIL_IF( req->magic != S3COMMS_HRB_MAGIC );
            if (C->verb != NULL) {
                JSVERIFY_STR( req->verb, C->verb, NULL )
            }
            JSVERIFY_STR( "HTTP/1.1",  req->version,  NULL )
            JSVERIFY_STR( C->exp_res,  req->resource, NULL )
            FAIL_IF( req->first_header != NULL );
            FAIL_IF( req->body         != NULL );
            FAIL_IF( req->body_len     != 0 );
            JSVERIFY( SUCCEED, H5FD_s3comms_hrb_destroy(req), 
                      "unable to destroy hrb_t" )
        }

    } /* for each testcase */

    PASSED();
    return 0;

error:
    H5FD_s3comms_hrb_destroy(req);

    return -1;

} /* test_hrb_init_request */


/*---------------------------------------------------------------------------
 *
 * Function: test_hrb_node_t()
 *
 * Purpose: 
 *
 *     Test operations on hrb_node_t structure
 *
 *     Specifies :
 *         `H5FD_s3comms_hrb_node_set()`
 *         `H5FD_s3comms_hrb_node_first()`
 *         `H5FD_s3comms_hrb_node_next()`
 *         `H5FD_s3comms_hrb_node_destroy()`
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 *---------------------------------------------------------------------------
 */
static herr_t
test_hrb_node_t(void)
{
    /*********************
     * test-local macros *
     *********************/

#define THFT_STR_LEN 256 /* 0x100 */

#define THFT_NAME      1
#define THFT_VALUE     2
#define THFT_CAT       3
#define THFT_LOWERNAME 4

/*
 * THFT_CAT_CHECK := [T]est [H]rb [F]ield] [T]ype con[CAT]enation [CHECK]
 *
 * with inputs: 
 *     `expected` - a given concatenated output 
 *     `ord_enum` - sorting order enum and
 *     `selector` - component selector definition
 *
 * use function-local variables:
 *     `str`  - string buffer 
 *     `i`    - indexing vairable
 *     `list` - pointer to any `hrb_node_t` node in list to check
 *     `node` - working variable `hrb_node_t` node pointer
 *
 * zero and build a comparison string buffer 
 * check against the expected
 * error if mismatch with expected.
 */
#define THFT_CAT_CHECK(expected, ord_enum, selector)             \
{   for (i = 0; i < THFT_STR_LEN; i++) {                         \
        str[i] = 0; /* zero destination string (THFT_STR_LEN) */ \
    }                                                            \
    /* working node at start of list, per ord_enum */            \
    /* node = H5FD_s3comms_hrb_node_first(list, ord_enum); */    \
    node = list;                                                 \
    if (ord_enum == HRB_NODE_ORD_GIVEN) {                        \
        while (node->prev != NULL) node = node->prev;            \
    } else {                                                     \
        while (node->prev_lower != NULL) node = node->prev_lower; \
    }                                                            \
    while (node != NULL) { /* stract which strings to str */     \
        switch(selector) {                                       \
            case THFT_NAME :                                     \
                strcat(str, node->name);                         \
                break;                                           \
            case THFT_VALUE :                                    \
                strcat(str, node->value);                        \
                break;                                           \
            case THFT_CAT :                                      \
                strcat(str, node->cat);                          \
                break;                                           \
            case THFT_LOWERNAME :                                \
                strcat(str, node->lowername);                    \
                break;                                           \
            default :                                            \
                break;                                           \
        }                                                        \
        /* advance to next node, according to order enum */      \
/*         node = H5FD_s3comms_hrb_node_next(node, (ord_enum)); */ \
        node = ((ord_enum) == HRB_NODE_ORD_GIVEN)                \
             ? node->next                                        \
             : node->next_lower;                                 \
    }                                                            \
    FAIL_IF( node != NULL);                                      \
    /* comparison string completed; compare */                   \
    JSVERIFY_STR( expected, str, NULL )                          \
}

    /************************
     * test-local variables *
     ************************/

    size_t      i    = 0;          /* working variable for macro */
    hrb_node_t *list = NULL;       /* list node we are working with */
    hrb_node_t *node = NULL;       /* working variable for macro */
    char        str[THFT_STR_LEN]; /* working buffer to test against */



    TESTING("test_hrb_node_t");

    /* cannot "unset" a field from an uninstantiated hrb_node_t */
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_set(NULL, "Host", NULL) );
/*  JSVFY(H5FD_s3comms_hrb_node_set(NULL, "Host", NULL), NULL, 
 *        "cannot 'unset' a field from an uninstantiated hrb_node_t.\n");
 *  ^ lacks ability to compare pointers. preserve for now.
 */

    /* NULL field name has no effect */
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_set(NULL, NULL, "somevalue") );
/* JSVFY(H5FD_s3comms_hrb_node_set(NULL, NULL, "somevalue"), NULL,
 *       "Cannot create list with NULL name.\n);"
 */

    /* looking for 'next' on an uninstantiated hrb_node_t returns NULL */
/*  JSVFY(list, NULL, NULL);
 *  JSVFY(H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_GIVEN), NULL, 
 *        "'next' on uninstantiated list (NULL) is always NULL.\n");
 *  JSVFY(H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_SORTED), NULL, NULL);
 */
    FAIL_IF( list != NULL);
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_GIVEN) );
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_SORTED) );

    /* insert one element
     */
    list = H5FD_s3comms_hrb_node_set(NULL, "Host", "mybucket.s3.com");
/*  JSVFY(list, NULL, NULL);
 *  JSVFY(list->magic, S3COMMS_HRB_NODE_MAGIC, NULL);
 */
    FAIL_IF( list == NULL );
    FAIL_IF( list->magic != S3COMMS_HRB_NODE_MAGIC );
    THFT_CAT_CHECK("Host", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK("Host", HRB_NODE_ORD_SORTED, THFT_NAME);
    THFT_CAT_CHECK("mybucket.s3.com", HRB_NODE_ORD_SORTED, THFT_VALUE);
    THFT_CAT_CHECK("Host: mybucket.s3.com", HRB_NODE_ORD_GIVEN, THFT_CAT);
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_GIVEN) );
    FAIL_IF( NULL != H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_SORTED) );

    /* insert two more elements, one sorted "between" 
     */
    list = H5FD_s3comms_hrb_node_set(list, "x-amz-date", "20170921");
    list = H5FD_s3comms_hrb_node_set(list, "Range", "bytes=50-100");
    FAIL_IF( list == NULL );
    FAIL_IF( list->magic != S3COMMS_HRB_NODE_MAGIC );
    THFT_CAT_CHECK("Hostx-amz-dateRange", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK("HostRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    THFT_CAT_CHECK("hostrangex-amz-date", HRB_NODE_ORD_SORTED, THFT_LOWERNAME);

    THFT_CAT_CHECK(                                \
            "mybucket.s3.combytes=50-10020170921", \
            HRB_NODE_ORD_SORTED,                   \
            THFT_VALUE);
    THFT_CAT_CHECK(                                                         \
            "Host: mybucket.s3.comRange: bytes=50-100x-amz-date: 20170921", \
            HRB_NODE_ORD_SORTED,                                            \
            THFT_CAT);
    THFT_CAT_CHECK(                                                         \
            "Host: mybucket.s3.comx-amz-date: 20170921Range: bytes=50-100", \
            HRB_NODE_ORD_GIVEN,                                             \
            THFT_CAT);

    /* add entry "less than" first node
     */
    list = H5FD_s3comms_hrb_node_set(list, "Access", "always");
    FAIL_IF( list == NULL );
    THFT_CAT_CHECK("Hostx-amz-dateRangeAccess", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK("AccessHostRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    /* demonstrate `H5FD_s3comms_hrb_node_first()`
     */
    node = H5FD_s3comms_hrb_node_first(list, HRB_NODE_ORD_SORTED);
    JSVERIFY_STR( "Access: always", node->cat, NULL )
    node = NULL;

    /* modify entry
     */
    list = H5FD_s3comms_hrb_node_set(list, "x-amz-date", "19411207");
    FAIL_IF( list == NULL );
    THFT_CAT_CHECK("Hostx-amz-dateRangeAccess", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK( \
 "Access: alwaysHost: mybucket.s3.comRange: bytes=50-100x-amz-date: 19411207",\
            HRB_NODE_ORD_SORTED, \
            THFT_CAT);

    /* add at end again
     */
    list = H5FD_s3comms_hrb_node_set(list, "x-forbidden", "True");
    FAIL_IF( list == NULL );
    THFT_CAT_CHECK("Hostx-amz-dateRangeAccessx-forbidden", \
                   HRB_NODE_ORD_GIVEN,                     \
                   THFT_NAME);
    THFT_CAT_CHECK("AccessHostRangex-amz-datex-forbidden", \
                   HRB_NODE_ORD_SORTED,                    \
                   THFT_NAME);
    THFT_CAT_CHECK("accesshostrangex-amz-datex-forbidden", \
                   HRB_NODE_ORD_SORTED,                    \
                   THFT_LOWERNAME);
    THFT_CAT_CHECK("alwaysmybucket.s3.combytes=50-10019411207True", \
                   HRB_NODE_ORD_SORTED,                             \
                   THFT_VALUE);

    /* modify and case-change entry
     */
    list = H5FD_s3comms_hrb_node_set(list, "hoST", "none");
    THFT_CAT_CHECK("hoSTx-amz-dateRangeAccessx-forbidden", \
                   HRB_NODE_ORD_GIVEN,                     \
                   THFT_NAME);
    THFT_CAT_CHECK("AccesshoSTRangex-amz-datex-forbidden", \
                   HRB_NODE_ORD_SORTED,                    \
                   THFT_NAME);
    THFT_CAT_CHECK("accesshostrangex-amz-datex-forbidden", \
                   HRB_NODE_ORD_SORTED,                    \
                   THFT_LOWERNAME);
    THFT_CAT_CHECK("alwaysnonebytes=50-10019411207True", \
                   HRB_NODE_ORD_SORTED,                  \
                   THFT_VALUE);

    /* AT THIS TIME:
     *
     * given  order: host, x-amz-date, range, access, x-forbidden
     * sorted order: access, host, range, x-amz-date, x-forbidden
     *
     * `list` points to 'host' node
     *
     * now, remove nodes and observe changes
     */

    /* remove last node of both lists
     */
    list = H5FD_s3comms_hrb_node_set(list, "x-forbidden", NULL);
    THFT_CAT_CHECK("hoSTx-amz-dateRangeAccess", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("AccesshoSTRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);

    /* remove first node of sorted 
     */
    list = H5FD_s3comms_hrb_node_set(list, "ACCESS", NULL);
    JSVERIFY_STR("hoST", list->name, NULL)
    THFT_CAT_CHECK("hoSTRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    THFT_CAT_CHECK("hoSTx-amz-dateRange", HRB_NODE_ORD_GIVEN,  THFT_NAME);

    /* remove first node of both; changes `list` pointer 
     * to first non null of previous sorted or next sorted 
     *     (in this case, next)
     */
    list = H5FD_s3comms_hrb_node_set(list, "Host", NULL);
    THFT_CAT_CHECK("x-amz-dateRange", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("Rangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    JSVERIFY_STR("Range", list->name, NULL )

    /* re-add Host element, and remove sorted Range
     */
    list = H5FD_s3comms_hrb_node_set(list, "Host", "nah");
    THFT_CAT_CHECK("x-amz-dateRangeHost", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("HostRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    JSVERIFY_STR( "Range", list->name, NULL )
    list = H5FD_s3comms_hrb_node_set(list, "Range", NULL);
    THFT_CAT_CHECK("x-amz-dateHost", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("Hostx-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    JSVERIFY_STR( "Host", list->name, NULL )

    /* remove Host again; on opposite ends of each list
     */ 
    list = H5FD_s3comms_hrb_node_set(list, "Host", NULL);
    THFT_CAT_CHECK("x-amz-date", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("x-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    JSVERIFY_STR( "x-amz-date", list->name, NULL )

    /* removing absent element has no effect
     */
    list = H5FD_s3comms_hrb_node_set(list, "Host", NULL);
    THFT_CAT_CHECK("x-amz-date", HRB_NODE_ORD_GIVEN,  THFT_NAME);
    THFT_CAT_CHECK("x-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    JSVERIFY_STR( "x-amz-date", list->name, NULL )

    /* removing last element returns NULL, but list node lingers
     */
    FAIL_UNLESS( NULL == H5FD_s3comms_hrb_node_set(list, "x-amz-date", NULL) );
    FAIL_IF( list == NULL ); /* not null, but has been freed */
    list = NULL;

    HDassert( node == NULL );

    /***********
     * DESTROY *
     ***********/
    
    /*  build up a list and demonstrate `H5FD_s3comms_hrb_node_destroy()`
     */

    list = H5FD_s3comms_hrb_node_set(NULL, "Host", "something");
    list = H5FD_s3comms_hrb_node_set(list, "Access", "None");
    list = H5FD_s3comms_hrb_node_set(list, "x-amz-date", "20171010T210844Z");
    list = H5FD_s3comms_hrb_node_set(list, "Range", "bytes=1024-");

    /* verify list */
    THFT_CAT_CHECK("HostAccessx-amz-dateRange", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK("AccessHostRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);
    HDassert( node == NULL );

    /* change pointer; demonstrate can destroy from anywhere in list */
    list = H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_GIVEN);
    list = H5FD_s3comms_hrb_node_next(list, HRB_NODE_ORD_GIVEN);
    JSVERIFY_STR( "x-amz-date", list->name, NULL);

    /* re-verify list */
    THFT_CAT_CHECK("HostAccessx-amz-dateRange", HRB_NODE_ORD_GIVEN, THFT_NAME);
    THFT_CAT_CHECK("AccessHostRangex-amz-date", HRB_NODE_ORD_SORTED, THFT_NAME);

    /* destroy eats everything but programmer must reset pointer */
    JSVERIFY( SUCCEED, H5FD_s3comms_hrb_node_destroy(list), 
              "unable to destroy");
    FAIL_IF( list == NULL ); /* not-null, but _has_ been freed */
    list = NULL;
    HDassert( node == NULL );

    /**********
     * PASSED *
     **********/

    PASSED();
    return 0;

error:
    /* this is how to dispose of a list in one go
     */
    if (list != NULL) {
        HDassert( SUCCEED == H5FD_s3comms_hrb_node_destroy(list) );
    }
    HDassert( node == NULL );

    return -1;

#undef THFT_STR_LEN
#undef THFT_CAT
#undef THFT_LOWERNAME
#undef THFT_NAME
#undef THFT_VALUE
#undef THFT_CAT_CHECK

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
            SHA256_DIGEST_LENGTH * 2,
        },
        {   SUCCEED,
            {'J','e','f','e'},
            4,
            "what do ya want for nothing?",
            28,
            "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843",
            SHA256_DIGEST_LENGTH * 2,
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
/* if (VERBOSE_HI) {...} */
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
        const char *exp;
        const char *in;
        size_t      len;
    };

    /************************
     * test-local variables *
     ************************/

    struct testcase cases[] = {
        {   "hallel",
            "HALlEluJAh",
            6,
        },
        {   "all\0 lower",
            "all\0 lower",
            10,
        },
        {   "nothing really matters",
            "to meeeeeee",
            0,
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
              NULL )

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
        {   "http://minio.ad.hdfgroup.org:9000/shakespeare/Poe_Raven.txt",
            SUCCEED,
            {   "http",
                "minio.ad.hdfgroup.org",
                "9000",
                "shakespeare/Poe_Raven.txt",
                NULL,
            },
            "hdf minio w/out query",
        },
        {   "http://hdfgroup.org:00/Poe_Raven.txt?some_params unchecked",
            SUCCEED,
            {   "http",
                "hdfgroup.org",
                "00",
                "Poe_Raven.txt",
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

        FAIL_IF( cases[i].exp_ret !=
                 H5FD_s3comms_parse_url(cases[i].url, &purl) )

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

    struct testcase cases[6] = {
        {'$', "%24", 3}, /* u+0024 dollar sign */
        {' ', "%20", 3}, /* u+0020 space */
        {'^', "%5E", 3}, /* u+0094 carat */
        {'/', "%2F", 3}, /* u+002f solidus (forward slash) */
/*        {??, "%C5%8C", 6},*/ 
/* u+014c Latin Capital Letter O with Macron
 * Not included because it is multibyte "wide" character that poses issues
 * both in the underlying function and in being written in this file.        
 */
        {'¢', "%C2%A2", 6}, /* u+00a2 cent sign */
        {'\0', "%00", 3}, /* u+0000 null */
    };

    char   dest[13];
    size_t dest_len = 0;
    int    i        = 0;
    int    n_cases  = 6;



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
 *
 * Function: test_s3r_ops()
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
test_s3r_ops(void)
{
    /*********************
     * test-local macros *
     *********************/
#define MY_BUFFER_SIZE 0x100 /* 256 */

    /*************************
     * test-local structures *
     *************************/

    /************************
     * test-local variables *
     ************************/

    const char     region[]     = "us-east-1";
    const char     secret_id[]  = "HDFGROUP0";
    const char     secret_key[] = "HDFGROUP0";
    char           buffer[MY_BUFFER_SIZE];
    char           buffer2[MY_BUFFER_SIZE];
    unsigned char  signing_key[SHA256_DIGEST_LENGTH];
    struct tm     *now          = NULL;
    char           iso8601now[ISO8601_SIZE];
    s3r_t         *handle       = NULL;
    unsigned int   curl_ready   = 0;



    TESTING("test_s3r_ops");

    /***************
     * BASIC SETUP *
     ***************/

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ready++;

    now = gmnow();
    HDassert(now != NULL);
    HDassert(ISO8601NOW(iso8601now, now) == (ISO8601_SIZE - 1)); 

    /* It is desired to have means available to verify that signing_key
     * was set successfully and to an expected value.
     * This means is not yet in hand.
     */
    HDassert(SUCCEED ==
             H5FD_s3comms_signing_key(signing_key, 
                                      (const char *)secret_key,
                                      (const char *)region,
                                      (const char *)iso8601now) );

    /**************
     * READ RANGE *
     **************/

    handle = H5FD_s3comms_s3r_open(
             "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
             region,
             secret_id,
             (const unsigned char *)signing_key);

    FAIL_IF( handle == NULL );
    FAIL_IF( FAIL ==
             H5FD_s3comms_s3r_read(handle,
                                   (haddr_t)1200699,
                                   (size_t)103,
                                   buffer) );
    FAIL_IF( 0 != strncmp(buffer,
                   "Osr. Sweet lord, if your lordship were at leisure, "  \
                   "I should impart\n    a thing to you from his Majesty.",
                   103) );

    /**********************
     * DEMONSTRATE RE-USE *
     **********************/

    FAIL_IF( FAIL ==
             H5FD_s3comms_s3r_read(handle,
                                   (haddr_t)3544662,
                                   (size_t)44,
                                   buffer2) );
    FAIL_IF( 0 != 
             strncmp(buffer2,
                     "Our sport shall be to take what they mistake",
                     44) );

    /* stop using this handle now!
     */
    FAIL_IF ( FAIL == 
              H5FD_s3comms_s3r_close(handle) );
    handle = NULL;

    /***********************
     * OPEN AN ABSENT FILE *
     ***********************/

    handle = H5FD_s3comms_s3r_open(
             "http://minio.ad.hdfgroup.org:9000/shakespeare/missing.csv",
             region,
             secret_id,
             (const unsigned char *)signing_key);

    FAIL_IF( handle != NULL );

    /**************************
     * INACTIVE PORT  ON HOST *
     **************************/

    FAIL_IF(NULL != H5FD_s3comms_s3r_open(
             "http://minio.ad.hdfgroup.org:80/shakespeare/t8.shakespeare.txt",
             region,
             secret_id,
             (const unsigned char *)signing_key) )

    /*******************************
     * INVALID AUTHENTICATION INFO *
     *******************************/

    /* passed in a bad ID 
     */
    handle = H5FD_s3comms_s3r_open(
             "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
             region,
             "I_MADE_UP_MY_ID",
             (const unsigned char *)signing_key);

    FAIL_IF( handle != NULL );

    /* using an invalid signing key
     */
    handle = H5FD_s3comms_s3r_open(
             "http://minio.ad.hdfgroup.org:9000/shakespeare/t8.shakespeare.txt",
             region,
             secret_id,
             (const unsigned char *)EMPTY_SHA256);

    FAIL_IF( handle != NULL );

    /*************
     * TEAR DOWN *
     *************/
    if (curl_ready != 0 ) { 
        curl_global_cleanup();
        curl_ready = 0;
    }

    PASSED();
    return 0;

error:
    /***********
     * cleanup *
     ***********/
    if (handle != NULL) { 
        H5FD_s3comms_s3r_close(handle); 
        handle = NULL; 
    }

    if (curl_ready != 0 ) { 
        curl_global_cleanup();
        curl_ready = 0;
    }

#undef MY_BUFFER_SIZE

    return -1;

} /* test_s3r_ops*/


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

        FAIL_IF( FAIL ==
                 H5FD_s3comms_signing_key(
                         key,
                         cases[i].secret_key,
                         cases[i].region,
                         cases[i].when) ); 

        FAIL_IF( 0 !=
                 strncmp((const char *)cases[i].exp, 
                         (const char *)key,
                         SHA256_DIGEST_LENGTH) );

        free(key);
        key = NULL;
    }


    /***************
     * ERROR CASES *
     ***************/

    key = (unsigned char *)malloc(sizeof(unsigned char) * \
                                  SHA256_DIGEST_LENGTH);
    HDassert(key != NULL);

    FAIL_IF( FAIL != 
             H5FD_s3comms_signing_key(
                     NULL,
                     cases[0].secret_key,
                     cases[0].region,
                     cases[0].when) );

    FAIL_IF( FAIL != 
             H5FD_s3comms_signing_key(
                     key,
                     NULL,
                     cases[0].region,
                     cases[0].when) );


    FAIL_IF( FAIL != 
             H5FD_s3comms_signing_key(
                     key,
                     cases[0].secret_key,
                     NULL,
                     cases[0].when) );

    FAIL_IF( FAIL != 
             H5FD_s3comms_signing_key(
                     key,
                     cases[0].secret_key,
                     cases[0].region,
                     NULL) );

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

    FAIL_IF( FAIL == 
             H5FD_s3comms_tostringtosign(s2s, canonreq, iso8601now, region) );

    JSVERIFY_STR( "AWS4-HMAC-SHA256\n20130524T000000Z\n20130524/us-east-1/s3/aws4_request\n7344ae5b7ee6c3e7e6b0fe0640412a37625d1fbfff95c48bbb2dc43964946972", 
                 s2s, NULL ) 

    FAIL_IF( FAIL != 
             H5FD_s3comms_tostringtosign(s2s, NULL, iso8601now, region) );

    FAIL_IF( FAIL != 
             H5FD_s3comms_tostringtosign(s2s, canonreq, NULL, region) );

    FAIL_IF( FAIL != 
             H5FD_s3comms_tostringtosign(s2s, canonreq, iso8601now, NULL) );

#if 0
/* not yet supported */
/* not recognized as ISO8601 time format */
    if (FAIL != H5FD_s3comms_tostringtosign(dest, canonreq, "Tue, 21 Dec 2017", region)) 
        TEST_ERROR;
#endif

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
        str = (char *)malloc(sizeof(char) * cases[i].in_len);
        HDassert(str != NULL);
        strncpy(str, cases[i].in, cases[i].in_len);

        FAIL_IF( FAIL == 
                 H5FD_s3comms_trim(dest, str, cases[i].in_len, &dest_len) );
        FAIL_IF( dest_len != cases[i].exp_len );
        if (dest_len > 0) {
           FAIL_IF( strncmp(cases[i].exp, dest, dest_len) != 0 );
        }
    }

    FAIL_IF( FAIL == H5FD_s3comms_trim(dest, NULL, 3, &dest_len) );
    FAIL_IF( 0 != dest_len );
    
    str = (char *)malloc(sizeof(char *) * 11);
    HDassert(str != NULL);
    memcpy(str, "some text ", 11); /* string with null terminator */
    FAIL_IF( FAIL != H5FD_s3comms_trim(NULL, str, 10, &dest_len) );
    free(str);

    PASSED();
    return 0;

error:
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

        FAIL_IF( FAIL ==
                 H5FD_s3comms_uriencode(dest, 
                                        cases[i].str, 
                                        str_len,
                                        cases[i].encode_slash,
                                        &dest_written) );

        FAIL_IF( dest_written != strlen(cases[i].expected) );

        FAIL_IF( strncmp(dest, cases[i].expected, dest_written) != 0 );

        free(dest);
        dest = NULL;
    }

    /***************
     * ERROR CASES *
     ***************/

    dest = (char *)malloc(sizeof(char) * 15);
    HDassert(dest != NULL);

    /* dest cannot be null
     */
    FAIL_IF( FAIL != 
             H5FD_s3comms_uriencode(NULL, "word$", 5, false, &dest_written) );

    /* source string cannot be null
     */
    FAIL_IF( FAIL != 
             H5FD_s3comms_uriencode(dest, NULL, 5, false, &dest_written) );

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
    nerrors += test_hrb_node_t()              < 0 ? 1 : 0;
    nerrors += test_hrb_init_request()        < 0 ? 1 : 0;
    nerrors += test_parse_url()               < 0 ? 1 : 0;
    nerrors += test_aws_canonical_request()   < 0 ? 1 : 0;
    nerrors += test_tostringtosign()          < 0 ? 1 : 0;
    nerrors += test_s3r_ops()                 < 0 ? 1 : 0;

    if(nerrors) {
        HDprintf("***** %d S3comms TEST%s FAILED! *****\n",
                 nerrors,
                 nerrors > 1 ? "S" : "");
        return 1;
    } /* end if */

    HDprintf("All S3comms tests passed.\n");

    return 0;
} /* end main() */

