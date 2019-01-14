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

/****************/
/* Module Setup */
/****************/

#include "H5Rmodule.h"          /* This source code file is part of the H5R module */


/***********/
/* Headers */
/***********/
#include "H5private.h"          /* Generic Functions                        */
#include "H5ACprivate.h"        /* Metadata cache                           */
#include "H5Dprivate.h"         /* Datasets                                 */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5HGprivate.h"        /* Global Heaps                             */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5MMprivate.h"        /* Memory management                        */
#include "H5Oprivate.h"         /* Object headers                           */
#include "H5Sprivate.h"         /* Dataspaces                               */
#include "H5Tprivate.h"         /* Datatypes                                */
#include "H5Rpkg.h"             /* References                               */


/****************/
/* Local Macros */
/****************/

#define H5R_MAX_ATTR_REF_NAME_LEN (64 * 1024)

/******************/
/* Local Typedefs */
/******************/

/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/

/* Flag indicating "top" of interface has been initialized */
static hbool_t H5R_top_package_initialize_s = FALSE;


/*--------------------------------------------------------------------------
NAME
   H5R__init_package -- Initialize interface-specific information
USAGE
    herr_t H5R__init_package()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
herr_t
H5R__init_package(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Mark "top" of interface as initialized */
    H5R_top_package_initialize_s = TRUE;

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5R__init_package() */


/*--------------------------------------------------------------------------
 NAME
    H5R_top_term_package
 PURPOSE
    Terminate various H5R objects
 USAGE
    void H5R_top_term_package()
 RETURNS
    void
 DESCRIPTION
    Release IDs for the atom group, deferring full interface shutdown
    until later (in H5R_term_package).
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5R_top_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    /* Mark closed if initialized */
    if(H5R_top_package_initialize_s)
        if(0 == n)
            H5R_top_package_initialize_s = FALSE;

    FUNC_LEAVE_NOAPI(n)
} /* end H5R_top_term_package() */


/*--------------------------------------------------------------------------
 NAME
    H5R_term_package
 PURPOSE
    Terminate various H5R objects
 USAGE
    void H5R_term_package()
 RETURNS
    void
 DESCRIPTION
    Release the atom group and any other resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...

     Finishes shutting down the interface, after H5R_top_term_package()
     is called
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5R_term_package(void)
{
    int	n = 0;

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    if(H5_PKG_INIT_VAR) {
        /* Sanity checks */
        HDassert(FALSE == H5R_top_package_initialize_s);

        /* Mark closed */
        if(0 == n)
            H5_PKG_INIT_VAR = FALSE;
    }

    FUNC_LEAVE_NOAPI(n)
} /* end H5R_term_package() */


/*-------------------------------------------------------------------------
 * Function:    H5R__create_object
 *
 * Purpose: Creates an object reference.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__create_object(haddr_t obj_addr, href_t *ref_ptr)
{
    struct href *ref = NULL;    /* Reference to be returned */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(ref_ptr);

    /* Create new reference */
    if(NULL == (ref = (struct href *)H5MM_malloc(sizeof(struct href))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")

    ref->loc_id = H5I_INVALID_HID;
    ref->ref_type = H5R_OBJECT;
    ref->ref.addr = obj_addr;
    *ref_ptr = ref;

done:
    if(ret_value < 0)
        H5MM_free(ref);
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__create_object() */


/*-------------------------------------------------------------------------
 * Function:    H5R__create_region
 *
 * Purpose: Creates a region reference.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__create_region(haddr_t obj_addr, H5S_t *space, href_t *ref_ptr)
{
    struct href *ref = NULL;    /* Reference to be returned */
    uint8_t *p = NULL;          /* Pointer to data to store */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(space);
    HDassert(ref_ptr);

    /* Create new reference */
    if(NULL == (ref = (struct href *)H5MM_malloc(sizeof(struct href))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")
    ref->loc_id = H5I_INVALID_HID;
    ref->ref_type = H5R_REGION;

    /* Get the amount of space required to serialize the selection */
    if(H5S_encode(space, &p, &ref->ref.serial.buf_size) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTENCODE, FAIL, "Cannot determine amount of space needed for serializing selection")

    /* Increase buffer size to allow for the dataset object address */
    ref->ref.serial.buf_size += sizeof(obj_addr);

    /* Allocate the space to store the serialized information */
    if(NULL == (ref->ref.serial.buf = H5MM_malloc(ref->ref.serial.buf_size)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTALLOC, FAIL, "Cannot allocate buffer to serialize selection")

    /* Serialize information for dataset object address into buffer */
    p = (uint8_t *)ref->ref.serial.buf;
    H5F_addr_encode_len(sizeof(obj_addr), &p, obj_addr);

    /* Serialize the selection into buffer */
    if(H5S_encode(space, &p, &ref->ref.serial.buf_size) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTENCODE, FAIL, "Unable to serialize selection")

    *ref_ptr = ref;

done:
    if(ret_value < 0) {
        H5MM_free(ref->ref.serial.buf);
        H5MM_free(ref);
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5R__create_region */


/*-------------------------------------------------------------------------
 * Function:    H5R__create_attr
 *
 * Purpose: Creates an attribute reference.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__create_attr(haddr_t obj_addr, const char *attr_name, href_t *ref_ptr)
{
    struct href *ref = NULL;    /* Reference to be returned */
    size_t attr_name_len;       /* Length of the attribute name */
    uint8_t *p;                 /* Pointer to data to store */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(attr_name);
    HDassert(ref_ptr);

    /* Create new reference */
    if(NULL == (ref = (struct href *)H5MM_malloc(sizeof(struct href))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")
    ref->loc_id = H5I_INVALID_HID;
    ref->ref_type = H5R_ATTR;

    /* Get the amount of space required to serialize the attribute name */
    attr_name_len = HDstrlen(attr_name);
    if(attr_name_len >= H5R_MAX_ATTR_REF_NAME_LEN)
        HGOTO_ERROR(H5E_REFERENCE, H5E_ARGS, FAIL, "attribute name too long")

    /* Compute buffer size, allow for the attribute name length and object address */
    ref->ref.serial.buf_size = attr_name_len + sizeof(uint16_t) + sizeof(obj_addr);

    /* Allocate the space to store the serialized information */
    if (NULL == (ref->ref.serial.buf = H5MM_malloc(ref->ref.serial.buf_size)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTALLOC, FAIL, "Cannot allocate buffer to serialize selection")

    /* Serialize information for object address into buffer */
    p = (uint8_t *)ref->ref.serial.buf;
    H5F_addr_encode_len(sizeof(obj_addr), &p, obj_addr);

    /* Serialize information for attribute name length into the buffer */
    UINT16ENCODE(p, attr_name_len);

    /* Copy the attribute name into the buffer */
    HDmemcpy(p, attr_name, attr_name_len);

    /* Sanity check */
    HDassert((size_t)((p + attr_name_len) - (uint8_t *)ref->ref.serial.buf) == ref->ref.serial.buf_size);

    *ref_ptr = ref;

done:
    if(ret_value < 0) {
        H5MM_free(ref->ref.serial.buf);
        H5MM_free(ref);
    }
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5R__create_attr */


/*-------------------------------------------------------------------------
 * Function:    H5R__destroy
 *
 * Purpose: Destroy reference.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__destroy(href_t ref)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(ref != HREF_NULL);

    if(H5R_OBJECT != ref->ref_type)
        H5MM_free(ref->ref.serial.buf);
    if((ref->loc_id != H5I_INVALID_HID) && (H5I_dec_ref(ref->loc_id) < 0))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTDEC, FAIL, "decrementing location ID failed")
    H5MM_free(ref);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__destroy() */


/*-------------------------------------------------------------------------
 * Function:    H5R__set_loc_id
 *
 * Purpose: Attach location ID to reference and increment location refcount.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__set_loc_id(href_t ref, hid_t id)
{
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(ref != HREF_NULL);
    HDassert(id != H5I_INVALID_HID);

    /* If a location ID was previously assigned, decrement refcount and assign new one */
    if((ref->loc_id != H5I_INVALID_HID) && (H5I_dec_ref(ref->loc_id) < 0))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTDEC, FAIL, "decrementing location ID failed")
    ref->loc_id = id;
    /* Prevent location ID from being freed until reference is destroyed */
    if(H5I_inc_ref(ref->loc_id, FALSE) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTINC, FAIL, "incrementing location ID failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__set_loc_id() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_loc_id
 *
 * Purpose: Retrieve location ID attached to existing reference.
 *
 * Return:  Valid ID on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5R__get_loc_id(href_t ref)
{
    hid_t ret_value = H5I_INVALID_HID;  /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    HDassert(ref != HREF_NULL);

    ret_value = ref->loc_id;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_loc_id() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_type
 *
 * Purpose: Given a reference to some object, return the type of that reference.
 *
 * Return:  Type of the reference
 *
 *-------------------------------------------------------------------------
 */
H5R_type_t
H5R__get_type(href_t ref)
{
    H5R_type_t ret_value = H5R_BADTYPE;

    FUNC_ENTER_PACKAGE_NOERR

    HDassert(ref != HREF_NULL);
    ret_value = ref->ref_type;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_type() */


/*-------------------------------------------------------------------------
 * Function:    H5R__equal
 *
 * Purpose: Compare two references
 *
 * Return:  TRUE if equal, FALSE if unequal, FAIL if error
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5R__equal(href_t ref1, href_t ref2)
{
    htri_t ret_value = FAIL;

    FUNC_ENTER_PACKAGE

    HDassert(ref1 != HREF_NULL);
    HDassert(ref2 != HREF_NULL);

    if (ref1->ref_type != ref2->ref_type)
        HGOTO_DONE(FALSE);

    switch (ref1->ref_type) {
        case H5R_OBJECT:
            if (ref1->ref.addr != ref2->ref.addr)
                HGOTO_DONE(FALSE);
            break;
        case H5R_REGION:
        case H5R_ATTR:
            if (ref1->ref.serial.buf_size != ref2->ref.serial.buf_size)
                HGOTO_DONE(FALSE);
            if (0 != HDmemcmp(ref1->ref.serial.buf, ref2->ref.serial.buf, ref1->ref.serial.buf_size))
                HGOTO_DONE(FALSE);
            break;
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

    ret_value = TRUE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__equal() */


/*-------------------------------------------------------------------------
 * Function:    H5R__copy
 *
 * Purpose: Copy a reference
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__copy(href_t src_ref, href_t *dest_ref_ptr)
{
    struct href *dest_ref = NULL;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    HDassert(src_ref != HREF_NULL);
    HDassert(dest_ref_ptr);

    /* Allocate the space to store the serialized information */
    if(NULL == (dest_ref = (struct href *)H5MM_malloc(sizeof(struct href))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")
    if(H5R__set_loc_id(dest_ref, src_ref->loc_id) < 0)
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTSET, FAIL, "cannot set reference location ID")
    dest_ref->ref_type = src_ref->ref_type;

    switch (src_ref->ref_type) {
        case H5R_OBJECT:
            dest_ref->ref.addr = src_ref->ref.addr;
            break;
        case H5R_REGION:
        case H5R_ATTR:
            if (0 == (dest_ref->ref.serial.buf_size = src_ref->ref.serial.buf_size))
                HGOTO_ERROR(H5E_REFERENCE, H5E_BADVALUE, FAIL, "Invalid reference buffer size")
            if (NULL == (dest_ref->ref.serial.buf = H5MM_malloc(src_ref->ref.serial.buf_size)))
                HGOTO_ERROR(H5E_REFERENCE, H5E_CANTALLOC, FAIL, "Cannot allocate reference buffer")
            HDmemcpy(dest_ref->ref.serial.buf, src_ref->ref.serial.buf, src_ref->ref.serial.buf_size);
            break;
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

    *dest_ref_ptr = dest_ref;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__copy() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_obj_addr
 *
 * Purpose: Given a reference to some object, get the encoded object addr.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__get_obj_addr(href_t ref, haddr_t *obj_addr_ptr)
{
    haddr_t obj_addr;           /* Object address */
    const uint8_t *p = NULL;    /* Pointer to reference buffer */
    herr_t ret_value = SUCCEED; /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(ref != HREF_NULL);
    HDassert(ref->ref_type > H5R_BADTYPE && ref->ref_type < H5R_MAXTYPE);
    HDassert(obj_addr_ptr);

    /* Point to reference buffer now */
    p = (const uint8_t *)ref->ref.serial.buf;

    /* Decode reference */
    switch (ref->ref_type) {
        case H5R_OBJECT:
            obj_addr = ref->ref.addr;
            break;
        case H5R_REGION:
        case H5R_ATTR:
            /* Get the object addr for the dataset */
            H5F_addr_decode_len(sizeof(obj_addr), &p, &obj_addr);
            break;
        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

    /* Sanity check */
    if(!H5F_addr_defined(obj_addr) || obj_addr == 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "undefined object address")
    *obj_addr_ptr = obj_addr;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_object() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_region
 *
 * Purpose: Given a reference to some object, creates a copy of the dataset
 * pointed to's dataspace and defines a selection in the copy which is the
 * region pointed to.
 *
 * Return:  Pointer to the dataspace on success/NULL on failure
 *
 *-------------------------------------------------------------------------
 */
H5S_t *
H5R__get_region(href_t ref)
{
    const uint8_t *p = NULL;    /* Pointer to reference buffer */
    H5S_t *ret_value = NULL;    /* Return value */

    FUNC_ENTER_PACKAGE

    HDassert(ref != HREF_NULL);
    HDassert(ref->ref_type == H5R_REGION);

    /* Point to reference buffer now */
    p = (const uint8_t *)ref->ref.serial.buf;

    /* Skip information for object's addr */
    p += sizeof(haddr_t);

    /* Deserialize the selection */
    if (NULL == (ret_value = H5S_decode(&p)))
        HGOTO_ERROR(H5E_REFERENCE, H5E_CANTDECODE, NULL, "can't deserialize selection")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_region() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_file_name
 *
 * Purpose: Given a reference to some object, determine a file name of the
 * object located into.
 *
 * Return:  Non-negative length of the path on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5R__get_file_name(href_t ref, char *name, size_t size)
{
    ssize_t ret_value = -1;     /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args */
    HDassert(ref != HREF_NULL);

    switch (ref->ref_type) {
        case H5R_OBJECT:
        case H5R_REGION:
        case H5R_ATTR:
        {
            const uint8_t *p;    /* Pointer to reference to decode */
            size_t filename_len; /* Length of the file name */
            size_t copy_len;

            /* Get the file name length */
            p = (const uint8_t *)ref->ref.serial.buf;
            UINT16DECODE(p, filename_len);
            copy_len = filename_len;

            /* Get the attribute name */
            if (name) {
                copy_len = MIN(copy_len, size - 1);
                HDmemcpy(name, p, copy_len);
                name[copy_len] = '\0';
            }
            ret_value = (ssize_t)(copy_len + 1);
        }
            break;
        case H5R_BADTYPE:
        case H5R_MAXTYPE:
        default:
            HDassert("unknown reference type" && 0);
            HGOTO_ERROR(H5E_REFERENCE, H5E_UNSUPPORTED, FAIL, "internal error (unknown reference type)")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_file_name() */


/*-------------------------------------------------------------------------
 * Function:    H5R__get_attr_name
 *
 * Purpose: Given a reference to some attribute, determine its name.
 *
 * Return:  Non-negative length of the path on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5R__get_attr_name(href_t ref, char *name, size_t size)
{
    ssize_t ret_value = -1;     /* Return value */
    const uint8_t *p = NULL;    /* Pointer to reference to decode */
    size_t attr_name_len;       /* Length of the attribute name */
    size_t copy_len;

    FUNC_ENTER_PACKAGE_NOERR

    /* Check args */
    HDassert(ref != HREF_NULL);
    HDassert(ref->ref_type == H5R_ATTR);

    /* Point to reference buffer now */
    p = (const uint8_t *)ref->ref.serial.buf;

    /* Skip information for object's addr */
    p += sizeof(haddr_t);

    /* Get the attribute name length */
    UINT16DECODE(p, attr_name_len);
    HDassert(attr_name_len < H5R_MAX_ATTR_REF_NAME_LEN);
    copy_len = attr_name_len;

    /* Get the attribute name */
    if (name) {
        copy_len = MIN(copy_len, size - 1);
        HDmemcpy(name, p, copy_len);
        name[copy_len] = '\0';
    }

    ret_value = (ssize_t)(copy_len + 1);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__get_attr_name() */


/*-------------------------------------------------------------------------
 * Function:    H5R__encode
 *
 * Purpose: Private function for H5Rencode.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__encode(href_t ref, unsigned char *buf, size_t *nalloc)
{
    size_t ref_buf_size, buf_size;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE_NOERR

    HDassert(ref != HREF_NULL);
    HDassert(nalloc);

    ref_buf_size = (ref->ref_type == H5R_OBJECT) ? sizeof(ref->ref.addr) :
        ref->ref.serial.buf_size;
    buf_size = ref_buf_size + sizeof(uint64_t) + 1;

    /* Don't encode if buffer size isn't big enough or buffer is empty */
    if(buf && *nalloc >= buf_size) {
        const void *ref_buf = (ref->ref_type == H5R_OBJECT) ? &ref->ref.addr :
            ref->ref.serial.buf;

        /* Encode the type of the information */
        *buf++ = ref->ref_type;

        /* Encode buffer size */
        UINT64ENCODE(buf, ref_buf_size);

        /* Encode into user's buffer */
        HDmemcpy(buf, ref_buf, ref_buf_size);
    } /* end if */

    *nalloc = buf_size;

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__encode() */


/*-------------------------------------------------------------------------
 * Function:    H5R__decode
 *
 * Purpose: Private function for H5Rdecode.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5R__decode(const unsigned char *buf, size_t *_nbytes, href_t *ref_ptr)
{
    struct href *ref = NULL;
    size_t buf_size;
    size_t nbytes = 0;
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    HDassert(buf);
    HDassert(ref_ptr);

    /* Create new reference */
    if(NULL == (ref = (struct href *)H5MM_malloc(sizeof(struct href))))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")

    ref->ref_type = H5I_INVALID_HID;
    ref->ref_type = (H5R_type_t)*buf++;
    nbytes++;
    if(ref->ref_type <= H5R_BADTYPE || ref->ref_type >= H5R_MAXTYPE)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference type");

    UINT64DECODE(buf, buf_size);
    nbytes += sizeof(uint64_t);
    if (!buf_size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid reference size");
    if (ref->ref_type == H5R_OBJECT)
        HDmemcpy(ref->ref.addr, buf, buf_size);
    else {
        ref->ref.serial.buf_size = buf_size;
        if(NULL == (ref->ref.serial.buf = H5MM_malloc(buf_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL, "Cannot allocate memory for reference")
        HDmemcpy(ref->ref.serial.buf, buf, buf_size);
    }
    nbytes += buf_size;

    if (_nbytes) *_nbytes = nbytes;
    *ref_ptr = ref;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5R__decode() */
