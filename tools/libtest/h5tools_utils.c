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
 * Purpose: unit-test functionality of the routines in `tools/lib/h5tools_utils`
 *
 * Jacob Smith 2017-11-10
 */

#include "H5private.h"
#include "h5tools_utils.h"
/* #include "h5test.h" */ /* linking failure */

#ifndef _H5TEST_

#define AT() fprintf(stdout, "   at %s:%d in %s()...\n",        \
                     __FILE__, __LINE__, FUNC);

#define FAILED(msg) {                 \
    fprintf(stdout, "*FAILED*"); AT() \
    if (msg == NULL) {                \
        fprintf(stdout,"(NULL)\n");   \
    } else {                          \
        fprintf(stdout, "%s\n", msg); \
    }                                 \
    fflush(stdout);                   \
}
#endif




/*----------------------------------------------------------------------------
 *
 * Function: test_parse_tuple()
 *
 * Purpose: 
 *
 *     Provide unit tests and specification for the `parse_tuple()` function.
 *
 * Return:
 *
 *     0   Tests passed.
 *     1   Tests failed.
 *
 * Programmer: Jacob Smith
 *             2017-10-11
 *
 * Changes: None.
 *
 *----------------------------------------------------------------------------
 */
static unsigned
test_parse_tuple(void)
{

    /*************************
     * TEST-LOCAL STRUCTURES *
     *************************/

    struct testcase {
        const char *test_msg;     /* info about test case */
        const char *in_str;       /* input string */
        int         sep;          /* separator "character" */
        herr_t      exp_ret;      /* expected SUCCEED / FAIL */
        unsigned    exp_nelems;   /* expected number of elements */
                                  /* (no more than 7!)           */
        const char *exp_elems[7]; /* list of elements (no more than 7!) */
    };



    /******************
     * TEST VARIABLES *
     ******************/

    struct testcase cases[] = {
        {   "bad start",
            "words(before)",
            ';',
            FAIL,
            0,
            {NULL},
        },
        {   "tuple not closed",
            "(not ok",
            ',',
            FAIL,
            0,
            {NULL},
        },
        {   "empty tuple",
            "()",
            '-',
            SUCCEED,
            1,
            {""},
        },
        {   "no separator",
            "(stuff keeps on going)",
            ',',
            SUCCEED,
            1,
            {"stuff keeps on going"},
        },
        {   "4-ple, escaped seperator",
            "(elem0,elem1,el\\,em2,elem3)", /* "el\,em" */
            ',',
            SUCCEED,
            4,
            {"elem0", "elem1", "el,em2", "elem3"},
        },
        {   "5-ple, escaped escaped separator",
            "(elem0,elem1,el\\\\,em2,elem3)",
            ',',
            SUCCEED,
            5,
            {"elem0", "elem1", "el\\", "em2", "elem3"},
        },
        {   "escaped non-comma separator",
            "(5-2-7-2\\-6-2)",
            '-',
            SUCCEED,
            5,
            {"5","2","7","2-6","2"},
        },
        {   "embedded close-paren",
            "(be;fo)re)",
            ';',
            SUCCEED,
            2,
            {"be", "fo)re"},
        },
        {   "embedded non-escaping backslash",
            "(be;fo\\re)",
            ';',
            SUCCEED,
            2,
            {"be", "fo\\re"},
        },
        {   "double close-paren at end",
            "(be;fore))",
            ';',
            SUCCEED,
            2,
            {"be", "fore)"},
        },
        {   "empty elements",
            "(;a1;;a4;)",
            ';',
            SUCCEED,
            5,
            {"", "a1", "", "a4", ""},
        },
    };
    struct testcase   tc;
    unsigned          n_tests       = 11;
    unsigned          i             = 0;
    unsigned          count         = 0;
    unsigned          elem_i        = 0;
    char            **parsed        = NULL;
    char             *cpy           = NULL;
    herr_t            success       = TRUE;
    hbool_t           show_progress = FALSE;



#ifdef _H5TEST_
    TESTING("arbitrary-count tuple parsing");
#else
    fprintf(stdout, "Testing %-62s", "arbitrary-count tuple parsing");
    fflush(stdout);
#endif



    /* TESTS */

    for (i = 0; i < n_tests; i++) {

        HDassert(parsed == NULL);
        HDassert(cpy == NULL);

        tc = cases[i];

        if (show_progress == TRUE) {
            printf("testing %s...\n", tc.test_msg);
        }



        success = parse_tuple(tc.in_str, tc.sep,
                              &cpy, &count, &parsed);
        /* JSVERIFY( tc.exp_ret, success, NULL ) */
        if (success != tc.exp_ret) { 
            FAILED(tc.test_msg)
            goto failed; 
        }
        /* JSVERIFY( tc.exp_nelems, count, NULL ) */
        if (count != tc.exp_nelems) { 
            FAILED(tc.test_msg)
            printf("    expected %d\n    actual   %d\n", tc.exp_nelems, count);
            fflush(stdout);
            goto failed; 
        }

        if (success == SUCCEED) {
            if (parsed == NULL) { 
                FAILED(tc.test_msg)
                printf("    No parsed pointer\n");
                goto failed; 
            }
            for (elem_i = 0; elem_i < count; elem_i++) {
#if 0
                fprintf(stdout,
                        "    expected %s\n    actual   %s\n",
                         tc.exp_elems[elem_i], parsed[elem_i]);
#endif
                if ((parsed[elem_i] == NULL) ||
                    (0 != strcmp(tc.exp_elems[elem_i], parsed[elem_i])))
                { 
                    FAILED(tc.test_msg)
                    fprintf(stdout, 
                            "    expected %s\n    actual   %s\n",
                            tc.exp_elems[elem_i], parsed[elem_i]);
                    fflush(stdout);
                    goto failed; 
                }
            }
            HDassert(parsed != NULL);
            HDassert(cpy    != NULL);
            free(parsed);
            parsed = NULL;
            free(cpy);
            cpy = NULL;
        } else if (parsed != NULL) { /* FAIL -> null parsed */
            FAILED(tc.test_msg)
            printf("    parsed pointer was not null\n");
            fflush(stdout);
        }

    } /* for each testcase */



#ifdef _H5TEST_
    PASSED();
#else
    fprintf(stdout, " PASSED\n"); fflush(stdout);
#endif
    return 0;

failed:
    /***********
     * CLEANUP *
     ***********/

    if (parsed != NULL) free(parsed);
    if (cpy    != NULL) free(cpy);

    return 1;

} /* test_parse_tuple */




/*----------------------------------------------------------------------------
 *
 * Function:   main()
 *
 * Purpose:    Run all test functions.
 *
 * Return:     0 iff all test pass
 *             1 iff any failures
 *
 * Programmer: Jacob Smith
 *             2017-11-10
 *
 * Changes:    None.
 *
 *----------------------------------------------------------------------------
 */
int
main(void)
{
    unsigned nerrors = 0;

#if 0
    h5reset(); /* h5test? */
#endif
    HDfprintf(stdout, "Testing h5tools_utils corpus.\n");

    nerrors += test_parse_tuple();

    if (nerrors > 0) {
        HDfprintf(stdout, "***** %d h5tools_utils TEST%s FAILED! *****\n",
                 nerrors,
                 nerrors > 1 ? "S" : "");
        nerrors = 1;
    } else {
        HDfprintf(stdout, "All h5tools_utils tests passed\n");
    }

    return (int)nerrors;

} /* main */


