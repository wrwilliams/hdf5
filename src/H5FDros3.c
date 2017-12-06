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
 * Programmer: Jacob Smith <jake.smith@hdfgroup.org>
 *             2017-10-13
 *
 * Purpose: 
 *
 *     Provide read-only access to files hosted on Amazon's S3 service.
 *     Uses "s3comms" utility layer to interface with the S3 REST API.
 */

/* This source code file is part of the H5FD driver module */
#include "H5FDdrvr_module.h"

#include "H5private.h"      /* Generic Functions        */
#include "H5Eprivate.h"     /* Error handling           */
#include "H5FDprivate.h"    /* File drivers             */
#include "H5FDros3.h"       /* ros3 file driver         */
#include "H5FLprivate.h"    /* Free Lists               */
#include "H5Iprivate.h"     /* IDs                      */
#include "H5MMprivate.h"    /* Memory management        */
#include "H5FDs3comms.h"    /* S3 Communications        */

/* toggle function call prints: 1 turns on
 */
#define ROS3_DEBUG 0

/* The driver identification number, initialized at runtime */
static hid_t H5FD_ROS3_g = 0;

/***************************************************************************
 *
 * Structure: H5FD_ros3_t
 *
 * Purpose:
 *
 *     H5FD_ros3_t is a structure used to store all information needed to 
 *     maintain R/O access to a single HDF5 file that has been stored as a
 *     S3 object.  This structure is created when such a file is "opened" and 
 *     discarded when it is "closed".
 *
 *     Presents an S3 object as a file to the HDF5 library.
 *
 *
 *
 * `pub` (H5FD_t)
 *
 *     Instance of H5FD_t which contains all fields common to all VFDs.
 *     It must be the first item in this structure, since at higher levels,
 *     this structure will be treated as an instance of H5FD_t.
 *
 * `fa` (H5FD_ros3_fapl_t)
 *
 *     Instance of H5FD_ros3_fapl_t containing the S3 configuration data 
 *     needed to "open" the HDF5 file.
 *
 * `eoa` (haddr_t)
 *
 *     End of addressed space in file. After open, it should always
 *     equal the file size.
 *
 * `s3r_handle` (s3r_t *)
 *     
 *     Instance of S3 Request handle associated with the target resource.
 *     Responsible for communicating with remote host and presenting file 
 *     contents as indistinguishable from a file on the local filesystem.
 *
 *
 *
 * Programmer: Jacob Smith
 *
 ***************************************************************************/
typedef struct H5FD_ros3_t {
    H5FD_t            pub;
    H5FD_ros3_fapl_t  fa;
    haddr_t           eoa;
    s3r_t            *s3r_handle;
} H5FD_ros3_t;

/*
 * These macros check for overflow of various quantities.  These macros
 * assume that HDoff_t is signed and haddr_t and size_t are unsigned.
 *
 * ADDR_OVERFLOW:   Checks whether a file address of type `haddr_t'
 *                  is too large to be represented by the second argument
 *                  of the file seek function.
 *
 */
#define MAXADDR (((haddr_t)1<<(8*sizeof(HDoff_t)-1))-1)
#define ADDR_OVERFLOW(A)    (HADDR_UNDEF==(A) || ((A) & ~(haddr_t)MAXADDR))

/* Prototypes */
static herr_t  H5FD_ros3_term(void);
static void   *H5FD_ros3_fapl_get(H5FD_t *_file);
static void   *H5FD_ros3_fapl_copy(const void *_old_fa);
static herr_t  H5FD_ros3_fapl_free(void *_fa);
static H5FD_t *H5FD_ros3_open(const char *name, unsigned flags, hid_t fapl_id,
                              haddr_t maxaddr);
static herr_t  H5FD_ros3_close(H5FD_t *_file);
static int     H5FD_ros3_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t  H5FD_ros3_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_ros3_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t  H5FD_ros3_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_ros3_get_eof(const H5FD_t *_file, H5FD_mem_t type);
static herr_t  H5FD_ros3_get_handle(H5FD_t *_file, hid_t fapl, 
                                    void** file_handle);
static herr_t  H5FD_ros3_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, 
                               haddr_t addr, size_t size, void *buf);
static herr_t  H5FD_ros3_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, 
                               haddr_t addr, size_t size, const void *buf);
static herr_t  H5FD_ros3_truncate(H5FD_t *_file, hid_t dxpl_id, 
                                  hbool_t closing);
static herr_t  H5FD_ros3_lock(H5FD_t *_file, hbool_t rw);
static herr_t  H5FD_ros3_unlock(H5FD_t *_file);
static herr_t  H5FD_ros3_validate_config(const H5FD_ros3_fapl_t * fa);

static const H5FD_class_t H5FD_ros3_g = {
    "ros3",                     /* name                 */
    MAXADDR,                    /* maxaddr              */
    H5F_CLOSE_WEAK,             /* fc_degree            */
    H5FD_ros3_term,             /* terminate            */
    NULL,                       /* sb_size              */
    NULL,                       /* sb_encode            */
    NULL,                       /* sb_decode            */
    sizeof(H5FD_ros3_fapl_t),   /* fapl_size            */
    H5FD_ros3_fapl_get,         /* fapl_get             */
    H5FD_ros3_fapl_copy,        /* fapl_copy            */
    H5FD_ros3_fapl_free,        /* fapl_free            */
    0,                          /* dxpl_size            */
    NULL,                       /* dxpl_copy            */
    NULL,                       /* dxpl_free            */
    H5FD_ros3_open,             /* open                 */
    H5FD_ros3_close,            /* close                */
    H5FD_ros3_cmp,              /* cmp                  */
    H5FD_ros3_query,            /* query                */
    NULL,                       /* get_type_map         */
    NULL,                       /* alloc                */
    NULL,                       /* free                 */
    H5FD_ros3_get_eoa,          /* get_eoa              */
    H5FD_ros3_set_eoa,          /* set_eoa              */
    H5FD_ros3_get_eof,          /* get_eof              */
    H5FD_ros3_get_handle,       /* get_handle           */
    H5FD_ros3_read,             /* read                 */
    H5FD_ros3_write,            /* write                */
    NULL,                       /* flush                */
    H5FD_ros3_truncate,         /* truncate             */
    H5FD_ros3_lock,             /* lock                 */
    H5FD_ros3_unlock,           /* unlock               */
    H5FD_FLMAP_DICHOTOMY        /* fl_map               */
};

/* Declare a free list to manage the H5FD_ros3_t struct */
H5FL_DEFINE_STATIC(H5FD_ros3_t);


/*-------------------------------------------------------------------------
 * Function:    H5FD__init_package
 *
 * Purpose:     Initializes any interface-specific data or routines.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Changes:     Rename as appropriate for ros3 vfd.
 *              Jacob Smith 2017
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD__init_package(void)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_STATIC

    if (H5FD_ros3_init() < 0) {
        HGOTO_ERROR(H5E_VFL, H5E_CANTINIT, FAIL, 
                    "unable to initialize ros3 VFD")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD__init_package() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_init
 *
 * Purpose:     Initialize this driver by registering the driver with the
 *              library.
 *
 * Return:      Success:    The driver ID for the ros3 driver.
 *              Failure:    Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Changes:     Rename as appropriate for ros3 vfd.
 *              Jacob Smith 2017
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5FD_ros3_init(void)
{
    hid_t ret_value = H5I_INVALID_HID; /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_init() called.\n");
#endif

    if (H5I_VFL != H5I_get_type(H5FD_ROS3_g))
        H5FD_ROS3_g = H5FD_register(&H5FD_ros3_g, sizeof(H5FD_class_t), FALSE);

    /* Set return value */
    ret_value = H5FD_ROS3_g;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5FD_ros3_init() */


/*---------------------------------------------------------------------------
 * Function:    H5FD_ros3_term
 *
 * Purpose:     Shut down the VFD
 *
 * Returns:     SUCCEED (Can't fail)
 *
 * Programmer:  Quincey Koziol
 *              Friday, Jan 30, 2004
 *
 * Changes:     Rename as appropriate for ros3 vfd.
 *              Jacob Smith 2017
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_term(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_term() called.\n");
#endif

    /* Reset VFL ID */
    H5FD_ROS3_g = 0;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5FD_ros3_term() */


/*-------------------------------------------------------------------------
 * Function:    H5Pset_fapl_ros3
 *
 * Purpose:     Modify the file access property list to use the H5FD_ROS3
 *              driver defined in this source file.  All driver specfic 
 *              properties are passed in as a pointer to a suitably 
 *              initialized instance of H5FD_ros3_fapl_t
 *
 * Return:      SUCCEED/FAIL
 *
 * Programmer:  John Mainzer
 *              9/10/17
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pset_fapl_ros3(hid_t             fapl_id, 
                 H5FD_ros3_fapl_t *fa)
{
    H5P_genplist_t *plist     = NULL; /* Property list pointer */
    herr_t          ret_value = FAIL;



    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", fapl_id, fa);

    HDassert(fa); /* fa cannot be null? */

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5Pset_fapl_ros3() called.\n");
#endif

    plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS);
    if (plist == NULL) { 
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, \
                    "not a file access property list")
    }

    if (FAIL == H5FD_ros3_validate_config(fa))
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid ros3 config")

    ret_value = H5P_set_driver(plist, H5FD_ROS3, (void *)fa);

done:

    FUNC_LEAVE_API(ret_value)

} /* H5Pset_fapl_ros3() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_validate_config()
 *
 * Purpose:     Test to see if the supplied instance of H5FD_ros3_fapl_t
 *              contains internally consistant data.  Return SUCCEED if so,
 *              and FAIL otherwise.
 *
 *              Note the difference between internally consistant and 
 *              correct.  As we will have to try to access the target 
 *              object to determine whether the supplied data is correct,
 *              we will settle for internal consistancy at this point
 *
 * Return:      SUCCEED if instance of H5FD_ros3_fapl_t contains internally 
 *              consistant data, FAIL otherwise.
 *
 * Programmer:  Jacob Smith
 *              9/10/17
 *
 * Changes:     Add checks for authenticate flag requring populated
 *              `aws_region` and `secret_id` strings.
 *                  -- Jacob Smith 2017-11-01
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_validate_config(const H5FD_ros3_fapl_t * fa)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

    HDassert(fa);

    if ( fa->version != H5FD__CURR_ROS3_FAPL_T_VERSION ) {
         HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                     "Unknown H5FD_ros3_fapl_t version");
    }

    /* if set to authenticate, region and id cannot be empty strings
     */
    if (fa->authenticate == TRUE) {
        if ((fa->aws_region[0] == 0) ||
            (fa->secret_id[0]  == 0))
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "Inconsistent authentication information");
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_validate_config() */


/*-------------------------------------------------------------------------
 * Function:    H5Pget_fapl_ros3
 *
 * Purpose:     Returns information about the ros3 file access property
 *              list though the function arguments.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  John Mainzer
 *              9/10/17
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Pget_fapl_ros3(hid_t             fapl_id, 
                 H5FD_ros3_fapl_t *fa_out)
{
    const H5FD_ros3_fapl_t *fa;
    H5P_genplist_t         *plist     = NULL;    /* Property list pointer */
    herr_t                  ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*x", fapl_id, fa_out);

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5Pget_fapl_ros3() called.\n");
#endif

    plist = H5P_object_verify(fapl_id, H5P_FILE_ACCESS);
    if (plist == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file access list")
    }

    if (H5FD_ROS3 != H5P_peek_driver(plist)) {
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "incorrect VFL driver")
    }

    fa = (const H5FD_ros3_fapl_t *)H5P_peek_driver_info(plist);
    if (fa == NULL) {
        HGOTO_ERROR(H5E_PLIST, H5E_BADVALUE, FAIL, "bad VFL driver info")
    }

    if (fa_out == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "fa_out is NULL")

    /* Copy the ros3 fapl data out */
    HDmemcpy(fa_out, fa, sizeof(H5FD_ros3_fapl_t));

done:
    FUNC_LEAVE_API(ret_value)

} /* H5Pget_fapl_ros3() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_fapl_get
 *
 * Purpose:     Gets a file access property list which could be used to
 *              create an identical file.
 *
 * Return:      Success:        Ptr to new file access property list value.
 *
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              9/8/17
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_ros3_fapl_get(H5FD_t *_file)
{
    H5FD_ros3_t      *file      = (H5FD_ros3_t*)_file;
    H5FD_ros3_fapl_t *fa        = NULL;
    void             *ret_value = NULL;

    FUNC_ENTER_NOAPI_NOINIT

    fa = (H5FD_ros3_fapl_t *)H5MM_calloc(sizeof(H5FD_ros3_fapl_t));
    if (fa == NULL) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, 
                    "memory allocation failed")
    }

    /* Copy the fields of the structure */
    HDmemcpy(fa, &(file->fa), sizeof(H5FD_ros3_fapl_t));

    /* Set return value */
    ret_value = fa;

done:
    if (ret_value == NULL) {
        if (fa != NULL) {
            H5MM_xfree(fa); 
        }
    }
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_fapl_get() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_fapl_copy
 *
 * Purpose:     Copies the ros3-specific file access properties.
 *
 * Return:      Success:        Ptr to a new property list
 *
 *              Failure:        NULL
 *
 * Programmer:  John Mainzer
 *              9/8/17
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static void *
H5FD_ros3_fapl_copy(const void *_old_fa)
{
    const H5FD_ros3_fapl_t *old_fa    = (const H5FD_ros3_fapl_t*)_old_fa;
    H5FD_ros3_fapl_t       *new_fa    = NULL;
    void                   *ret_value = NULL;     /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

    new_fa = (H5FD_ros3_fapl_t *)H5MM_malloc(sizeof(H5FD_ros3_fapl_t));
    if (new_fa == NULL) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, 
                    "memory allocation failed");
    }

    HDmemcpy(new_fa, old_fa, sizeof(H5FD_ros3_fapl_t));
    ret_value = new_fa;

done:
    if (ret_value == NULL) {
        if (new_fa != NULL)
            H5MM_xfree(new_fa);
    }
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_fapl_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_fapl_free
 *
 * Purpose:     Frees the ros3-specific file access properties.
 *
 * Return:      SUCCEED (cannot fail)
 *
 * Programmer:  John Mainzer
 *              9/8/17
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_fapl_free(void *_fa)
{
    H5FD_ros3_fapl_t *fa = (H5FD_ros3_fapl_t*)_fa;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(fa); /* sanity check */ 

    H5MM_xfree(fa);

    FUNC_LEAVE_NOAPI(SUCCEED)

} /* H5FD_ros3_fapl_free() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_open()
 *
 * Purpose:
 *
 *     Create and/or opens a file as an HDF5 file.
 *
 *     Any flag except H5F_ACC_RDONLY will cause an error.
 * 
 *     Name (as received from `H5FD_open()`) must conform to web url:
 *         NAME   :: HTTP "://" DOMAIN [PORT] ["/" [URI] [QUERY] ]
 *         HTTP   :: "http" [ "s" ] 
 *         DOMAIN :: e.g., "mybucket.host.org"
 *         PORT   :: ":" <number> (e.g., ":9000" )
 *         URI    :: <string> (e.g., "path/to/resource.hd5" )
 *         QUERY  :: "?" <string> (e.g., "arg1=param1&arg2=param2")
 *
 * Return:
 *
 *     Success: A pointer to a new file data structure. 
 *              The public fields will be initialized by the caller, which is 
 *              always H5FD_open().
 *
 *     Failure: NULL
 *
 * Programmer: Jacob Smith
 *             2017-11-02
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static H5FD_t *
H5FD_ros3_open(const char *url, 
               unsigned    flags, 
               hid_t       fapl_id, 
               haddr_t     maxaddr)
{
    H5FD_ros3_t      *file     = NULL;
    struct tm        *now      = NULL;
    char              iso8601now[ISO8601_SIZE];
    unsigned char     signing_key[SHA256_DIGEST_LENGTH];
    s3r_t            *handle   = NULL;
    H5FD_ros3_fapl_t *plist    = NULL;
    H5FD_t          *ret_value = NULL;



    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_open() called.\n");
#endif

    /* Sanity check on file offsets */
    HDcompile_assert(sizeof(HDoff_t) >= sizeof(size_t));

    /* Check arguments */
    if(!url || !*url) /* assert name != NULL && name[0] != 0 */
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid file name")
    if(0 == maxaddr || HADDR_UNDEF == maxaddr)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "bogus maxaddr")
    if(ADDR_OVERFLOW(maxaddr))
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, NULL, "bogus maxaddr")
    if (flags != H5F_ACC_RDONLY) {
        HGOTO_ERROR(H5E_ARGS, H5E_UNSUPPORTED, NULL,
                    "only Read-Only access allowed")
    }

    plist = (H5FD_ros3_fapl_t *)malloc(sizeof(H5FD_ros3_fapl_t));
    if (plist == NULL) { 
        HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, NULL, 
                    "can't allocate space for ros3 property list copy")
    }
    if (FAIL == H5Pget_fapl_ros3(fapl_id, plist)) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "can't get property list")
    }

/* TODO: curl_global_cleanup once and only once per global init */
/* TODO: move this global init away from here... to where is open question */
/* TODO: coordinate curl global cleanup--it is not thread-safe */
    HDassert( CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT) );

    /* open file; procedure depends on whether or not the fapl instructs to
     * authenticate requests or not.
     */
    if (plist->authenticate == TRUE) {
        /* compute signing key (part of AWS/S3 REST API)
         * can be re-used by user/key for 7 days after creation.
         * find way to re-use/share
         */
        now = gmnow();
        HDassert( now != NULL );
        HDassert( ISO8601NOW(iso8601now, now) == (ISO8601_SIZE - 1) );
        HDassert( SUCCEED ==
                  H5FD_s3comms_signing_key(signing_key,
                                          (const char *)plist->secret_key,
                                          (const char *)plist->aws_region,
                                          (const char *)iso8601now) );

        handle = H5FD_s3comms_s3r_open(
                 url,
                 (const char *)plist->aws_region,
                 (const char *)plist->secret_id,
                 (const unsigned char *)signing_key);
    } else {
        handle = H5FD_s3comms_s3r_open(url, NULL, NULL, NULL);
    } /* if/else should authenticate */

    if (handle == NULL) {
        /* If we want to check CURL's say on the matter in a controlled
         * fashion, this is the place to do it, but would need to make a 
         * few minor changes to s3comms `s3r_t` and `s3r_read()`.
         */ 
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, NULL, "could not open");
    }

    /* create new file struct 
     */
    file = H5FL_CALLOC(H5FD_ros3_t);
    if (file == NULL) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, 
                    "unable to allocate file struct")
    }

    file->s3r_handle = handle;

    ret_value = (H5FD_t*)file;

done:
    if (plist) { 
        free(plist); 
    }
    if (NULL == ret_value) {
        if (handle) { 
            HDassert(SUCCEED == H5FD_s3comms_s3r_close(handle)); 
        }
        if (file) {
            file = H5FL_FREE(H5FD_ros3_t, file);
        }
    } /* if null return value (error) */

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_open() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_close()
 *
 * Purpose:
 *
 *     Close an HDF5 file.
 *
 * Return:
 * 
 *     SUCCEED/FAIL
 *
 * Programmer: Jacob Smith
 *             2017-11-02
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_close(H5FD_t *_file)
{
    H5FD_ros3_t *file      = (H5FD_ros3_t *)_file;
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_close() called.\n");
#endif

    /* Sanity checks 
     */
    HDassert(file);
    HDassert(file->s3r_handle);

    /* Close the underlying request handle 
     */
    if (FAIL == H5FD_s3comms_s3r_close(file->s3r_handle)) {
        HGOTO_ERROR(H5E_VFL, H5E_CANTCLOSEFILE, FAIL, 
                    "unable to close S3 request handle")
    }

    /* Release the file info 
     */
    file = H5FL_FREE(H5FD_ros3_t, file);

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5FD_ros3_close() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_cmp()
 *
 * Purpose:
 *
 *     Compares two files belonging to this driver using an arbitrary 
 *     (but consistent) ordering.
 *              
 *     Uses the parsed url structure to compare, as though doing `strcmp` on
 *     the url strings used to open the file.
 *
 *     `strcmp`, element-by-element, of scheme, host, port, path, and query
 *     parameters in files' S3 Requests's `purl` parsed url structure.
 *
 *     - Scheme and Host are guaranteed present, and `strcmp`'d
 *     - If equivalent (value of 0), attempts to compare ports:
 *         - if both `f1` and `f2` have `port`, `strcmp` port number-strings
 *         - if one absent, set cmp value to 1 if `f1` has, -1 if `f2` has
 *         - if both absent, does not modify cmp value
 *     - If equivalent, attempts to compare paths using same apprach as with
 *       port.
 *     - If equivalent, attempts to compare query strings using same approach
 *       as with port.
 *     - If equivalent, attempts to compare `fa.aws_region`
 *     - If equivalent, attempts to compare `fa.secret_id`
 *     - If equivalent, attempts to compare `fa.secret_key`
 *
 * Return:
 *      
 *     - Equivalent:      0
 *     - Not Equivalent: -1 or 1, following algorithm outlined above.
 *
 * Programmer: Jacob Smith
 *             2017-11-06
 *
 *-------------------------------------------------------------------------
 */
static int
H5FD_ros3_cmp(const H5FD_t *_f1,
              const H5FD_t *_f2)
{
    const H5FD_ros3_t  *f1        = (const H5FD_ros3_t *)_f1;
    const H5FD_ros3_t  *f2        = (const H5FD_ros3_t *)_f2;
    const parsed_url_t *purl1     = NULL;
    const parsed_url_t *purl2     = NULL;
    int                 ret_value = 0;



    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_cmp() called.\n");
#endif

    HDassert(f1->s3r_handle);
    HDassert(f2->s3r_handle);

    purl1 = (const parsed_url_t *)f1->s3r_handle->purl;
    purl2 = (const parsed_url_t *)f2->s3r_handle->purl;
    HDassert(purl1);
    HDassert(purl2);
    HDassert(purl1->scheme);
    HDassert(purl2->scheme);
    HDassert(purl1->host);
    HDassert(purl2->host);

    /* URL: SCHEME */
    ret_value = strcmp(purl1->scheme, purl2->scheme);

    /* URL: HOST */
    if (0 == ret_value) {
        ret_value = strcmp(purl1->host, purl2->host);
    }

    /* URL: PORT */
    if (0 == ret_value) {
        if (purl1->port && purl2->port) {
	    ret_value = strcmp(purl1->port, purl2->port);
        } else if (purl1->port && !purl2->port) {
            ret_value = 1;
        } else if (purl2->port && !purl1->port) {
            ret_value = -1;
        }
    }

    /* URL: PATH */
    if (0 == ret_value) {
        if (purl1->path && purl2->path) {
	    ret_value = strcmp(purl1->path, purl2->path);
        } else if (purl1->path && !purl2->path) {
            ret_value = 1;
        } else if (purl2->path && !purl1->path) {
            ret_value = -1;
        }
    }

    /* URL: QUERY */
    if (0 == ret_value) {
        if (purl1->query && purl2->query) {
	    ret_value = strcmp(purl1->query, purl2->query);
        } else if (purl1->query && !purl2->query) {
            ret_value = 1;
        } else if (purl2->query && !purl1->query) {
            ret_value = -1;
        }
    }

    /* FAPL: AWS_REGION */
    if (0 == ret_value) {
        if (f1->fa.aws_region && f2->fa.aws_region) {
	    ret_value = strcmp(f1->fa.aws_region, f2->fa.aws_region);
        } else if (f1->fa.aws_region && !f2->fa.aws_region) {
            ret_value = 1;
        } else if (f2->fa.aws_region && !f1->fa.aws_region) {
            ret_value = -1;
        }
    }

    /* FAPL: SECRET_ID */
    if (0 == ret_value) {
        if (f1->fa.secret_id && f2->fa.secret_id) {
	    ret_value = strcmp(f1->fa.secret_id, f2->fa.secret_id);
        } else if (f1->fa.secret_id && !f2->fa.secret_id) {
            ret_value = 1;
        } else if (f2->fa.secret_id && !f1->fa.secret_id) {
            ret_value = -1;
        }
    }

    /* FAPL: SECRET_KEY */
    if (0 == ret_value) {
        if (f1->fa.secret_key && f2->fa.secret_key) {
	    ret_value = strcmp(f1->fa.secret_key, f2->fa.secret_key);
        } else if (f1->fa.secret_key && !f2->fa.secret_key) {
            ret_value = 1;
        } else if (f2->fa.aws_region && !f1->fa.aws_region) {
            ret_value = -1;
        }
    }

    /* constrain to -1, 0, 1--makes testing much easier
     */
    ret_value = (ret_value < 0) ? -1 : ret_value;
    ret_value = (ret_value > 0) ?  1 : ret_value;

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_cmp() */


/*-------------------------------------------------------------------------
 * Function:    H5FD_ros3_query
 *
 * Purpose:     Set the flags that this VFL driver is capable of supporting.
 *              (listed in H5FDpublic.h)
 *
 *              Note that since the ROS3 VFD is read only, most flags 
 *              are irrelevant.
 *
 *              The term "set" is highly misleading...
 *              stores/copies the supported flags in the out-pointer `flags`.
 *
 * Return:      SUCCEED (Can't fail)
 *
 * Programmer:  John Mainzer
 *              9/11/17
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_query(const H5FD_t H5_ATTR_UNUSED *_file, 
                unsigned long *flags /* out */)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_query() called.\n");
#endif

    /* Set the VFL feature flags that this driver supports */
    if (flags) {
        *flags = 0;
        /* OK to perform data sieving for faster raw data reads & writes */
        *flags |= H5FD_FEAT_DATA_SIEVE; 
    } /* end if */

    FUNC_LEAVE_NOAPI(SUCCEED)

} /* H5FD_ros3_query() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_get_eoa()
 *
 * Purpose:
 *
 *     Gets the end-of-address marker for the file. The EOA marker
 *     is the first address past the last byte allocated in the
 *     format address space.
 *
 * Return:
 *
 *     The end-of-address marker. (length of the file.)
 *
 * Programmer: Jacob Smith
 *             2017-11-02
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_ros3_get_eoa(const H5FD_t                *_file, 
                  H5FD_mem_t   H5_ATTR_UNUSED  type)
{
    const H5FD_ros3_t *file = (const H5FD_ros3_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_get_eoa() called.\n");
#endif

    FUNC_LEAVE_NOAPI(file->eoa)

} /* end H5FD_ros3_get_eoa() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_set_eoa()
 *
 * Purpose:
 *
 *     Set the end-of-address marker for the file.
 *     Not possible with this VFD.
 *
 * Return:
 *
 *      SUCCEED  (can't fail)
 *
 * Programmer: Jacob Smith
 *             2017-11-03
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_set_eoa(H5FD_t                    *_file, 
                  H5FD_mem_t H5_ATTR_UNUSED  type, 
                  haddr_t                    addr)
{
    H5FD_ros3_t *file = (H5FD_ros3_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_set_eoa() called.\n");
#endif

    file->eoa = addr;

    FUNC_LEAVE_NOAPI(SUCCEED)

} /* H5FD_ros3_set_eoa() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_get_eof()
 *
 * Purpose:
 *
 *     Returns the end-of-file marker, which is the greater of
 *     either the filesystem end-of-file or the HDF5 end-of-address
 *     markers.
 *
 * Return:
 *
 *     EOF: the first address past the end of the "file", either the 
 *     filesystem file or the HDF5 file.
 *
 * Programmer: Jacob Smith
 *             2017-11-02
 *
 *-------------------------------------------------------------------------
 */
static haddr_t
H5FD_ros3_get_eof(const H5FD_t                *_file, 
                  H5FD_mem_t   H5_ATTR_UNUSED  type)
{
    const H5FD_ros3_t *file = (const H5FD_ros3_t *)_file;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_get_eof() called.\n");
#endif

    FUNC_LEAVE_NOAPI(file->s3r_handle->filesize)

} /* end H5FD_ros3_get_eof() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_get_handle()
 *
 * Purpose:
 *
 *     Returns the S3 Request handle (s3r_t) of ros3 file driver.
 *
 * Returns:
 *
 *     SUCCEED/FAIL
 *
 * Programmer: Jacob Smith
 *             2017-11-02
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_get_handle(H5FD_t                *_file, 
                     hid_t H5_ATTR_UNUSED   fapl, 
                     void                 **file_handle)
{
    H5FD_ros3_t *file      = (H5FD_ros3_t *)_file;
    herr_t       ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_get_handle() called.\n");
#endif

    if(!file_handle)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "file handle not valid")

    *file_handle = file->s3r_handle;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5FD_ros3_get_handle() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_read()
 *
 * Purpose: 
 *
 *     Reads SIZE bytes of data from FILE beginning at address ADDR
 *     into buffer BUF according to data transfer properties in DXPL_ID.
 *
 * Return:
 *
 *     Success:    SUCCEED - Result is stored in caller-supplied
 *                           buffer BUF.
 *     Failure:    FAIL    - Contents of buffer BUF are undefined.
 *
 * Programmer: Jacob Smith
 *             2017-11-??
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_read(H5FD_t                    *_file, 
               H5FD_mem_t H5_ATTR_UNUSED  type, 
               hid_t      H5_ATTR_UNUSED  dxpl_id,
               haddr_t                    addr, /* start offset   */
               size_t                     size, /* length of read */
               void                      *buf)  /* out            */
{
    H5FD_ros3_t *file      = (H5FD_ros3_t *)_file;
    herr_t       ret_value = SUCCEED;                  /* Return value */

    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_read() called.\n");
#endif

    HDassert(file);
    HDassert(file->s3r_handle);
    HDassert(buf);

    if ((addr > file->s3r_handle->filesize) || 
        ((addr + size) > file->s3r_handle->filesize)) {
        HGOTO_ERROR(H5E_ARGS, H5E_OVERFLOW, FAIL, "range exceeds file address")
    }

    if (FAIL == H5FD_s3comms_s3r_read(file->s3r_handle, addr, size, buf) ) {
        HGOTO_ERROR(H5E_VFL, H5E_READERROR, FAIL, "unable to execute read")
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5FD_ros3_read() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_write()
 *
 * Purpose: 
 *
 *     Write bytes to file.
 *     UNSUPPORTED IN READ-ONLY ROS3 VFD.
 *
 * Return: 
 *
 *     FAIL (Not possible with Read-Only S3 file.)
 *
 * Programmer: Jacob Smith
 *             2017-10-23
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_write(H5FD_t     H5_ATTR_UNUSED *_file, 
                H5FD_mem_t H5_ATTR_UNUSED  type, 
                hid_t      H5_ATTR_UNUSED  dxpl_id,
                haddr_t    H5_ATTR_UNUSED  addr, 
                size_t     H5_ATTR_UNUSED  size, 
                const void H5_ATTR_UNUSED *buf)
{
    herr_t ret_value = FAIL;

    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_write() called.\n");
#endif

    HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL,
                "cannot write to read-only file.")

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_ros3_write() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_truncate()
 *
 * Purpose:
 *
 *     Makes sure that the true file size is the same (or larger)
 *     than the end-of-address.
 *
 *     NOT POSSIBLE ON READ-ONLY S3 FILES.
 *
 * Return:
 *
 *     FAIL (Not possible on Read-Only S3 files.)
 *
 * Programmer: Jacob Smith
 *             2017-10-23
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_truncate(H5FD_t  H5_ATTR_UNUSED *_file, 
                   hid_t   H5_ATTR_UNUSED  dxpl_id, 
                   hbool_t H5_ATTR_UNUSED  closing)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT

#if ROS3_DEBUG > 0
    HDfprintf(stdout, "H5FD_ros3_truncate() called.\n");
#endif

    HGOTO_ERROR(H5E_VFL, H5E_UNSUPPORTED, FAIL,
                "cannot truncate read-only file.")

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5FD_ros3_truncate() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_lock()
 *
 * Purpose:
 *
 *     Place an advisory lock on a file.
 *     No effect on Read-Only S3 file.
 *
 *     Suggestion: remove lock/unlock from class
 *               > would result in error at H5FD_[un]lock() (H5FD.c)
 *
 * Return:
 *
 *     SUCCEED (No-op always succeeds)
 *
 * Programmer: Jacob Smith
 *             2017-11-03
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_lock(H5FD_t  H5_ATTR_UNUSED *_file, 
               hbool_t H5_ATTR_UNUSED  rw)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR
    FUNC_LEAVE_NOAPI(SUCCEED)

} /* end H5FD_ros3_lock() */


/*-------------------------------------------------------------------------
 *
 * Function: H5FD_ros3_unlock()
 *
 * Purpose:
 *
 *     Remove the existing lock on the file.
 *     No effect on Read-Only S3 file.
 *
 * Return:
 *
 *     SUCCEED (No-op always succeeds)
 *
 * Programmer: Jacob Smith
 *             2017-11-03
 *
 * Changes: None.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5FD_ros3_unlock(H5FD_t H5_ATTR_UNUSED *_file)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR
    FUNC_LEAVE_NOAPI(SUCCEED)

} /* end H5FD_ros3_unlock() */


