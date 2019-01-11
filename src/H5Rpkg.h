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

/* Purpose:     This file contains declarations which are visible
 *              only within the H5R package. Source files outside the
 *              H5R package should include H5Rprivate.h instead.
 */
#if !(defined H5R_FRIEND || defined H5R_MODULE)
#error "Do not include this file outside the H5R package!"
#endif

#ifndef _H5Rpkg_H
#define _H5Rpkg_H

/* Get package's private header */
#include "H5Rprivate.h"

/* Other private headers needed by this file */
#include "H5Fprivate.h"         /* Files                                    */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Oprivate.h"         /* Object headers                           */
#include "H5Sprivate.h"         /* Dataspaces                               */


/**************************/
/* Package Private Macros */
/**************************/


/****************************/
/* Package Private Typedefs */
/****************************/

/* Internal data structures */
struct href {
    hid_t loc_id;               /* Cached location identifier */
    H5R_type_t ref_type;
    union {
        struct {
            size_t buf_size;    /* Size of serialized reference */
            void *buf;          /* Pointer to serialized reference */
        } serial;
        haddr_t addr;
    } ref;
};

/*****************************/
/* Package Private Variables */
/*****************************/


/******************************/
/* Package Private Prototypes */
/******************************/
H5_DLL herr_t   H5R__create_object(haddr_t obj_addr, href_t *ref_ptr);
H5_DLL herr_t   H5R__create_region(haddr_t obj_addr, H5S_t *space, href_t *ref_ptr);
H5_DLL herr_t   H5R__create_attr(haddr_t obj_addr, const char *attr_name, href_t *ref_ptr);
H5_DLL herr_t   H5R__destroy(href_t ref);

H5_DLL herr_t   H5R__set_loc_id(href_t ref, hid_t id);
H5_DLL hid_t    H5R__get_loc_id(href_t ref);

H5_DLL H5R_type_t   H5R__get_type(href_t ref);
H5_DLL htri_t   H5R__equal(href_t ref1, href_t ref2);
H5_DLL herr_t   H5R__copy(href_t src_ref, href_t *dest_ref_ptr);

H5_DLL herr_t   H5R__get_obj_addr(href_t ref, haddr_t *obj_addr_ptr);
H5_DLL H5S_t *  H5R__get_region(href_t ref);

H5_DLL ssize_t  H5R__get_file_name(href_t ref, char *name, size_t size);
H5_DLL ssize_t  H5R__get_attr_name(href_t ref, char *name, size_t size);

H5_DLL herr_t   H5R__encode(href_t ref, unsigned char *buf, size_t *nalloc);
H5_DLL herr_t   H5R__decode(const unsigned char *buf, size_t *nbytes, href_t *ref_ptr);

#endif /* _H5Rpkg_H */

