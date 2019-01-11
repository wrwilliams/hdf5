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
 * This file contains public declarations for the H5R module.
 */
#ifndef _H5Rpublic_H
#define _H5Rpublic_H

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Gpublic.h"
#include "H5Ipublic.h"

/*****************/
/* Public Macros */
/*****************/

/* NULL reference */
#define HREF_NULL ((href_t)0)

/*******************/
/* Public Typedefs */
/*******************/

/*
 * Reference types allowed.
 */
typedef enum {
    H5R_BADTYPE     =   (-1),   /*invalid Reference Type                     */
    H5R_OBJECT,                 /*Object reference                           */
    H5R_REGION_COMPAT,          /*For backward compatibility (region)        */
    H5R_REGION,                 /*Region Reference                           */
    H5R_ATTR,                   /*Attribute Reference                        */
    H5R_MAXTYPE                 /*highest type (Invalid as true type)        */
} H5R_type_t;

/* Opaque reference type */
typedef struct href *href_t;

/********************/
/* Public Variables */
/********************/


/*********************/
/* Public Prototypes */
/*********************/

#ifdef __cplusplus
extern "C" {
#endif

/* Constructors */
H5_DLL herr_t   H5Rcreate_object(hid_t loc_id, const char *name, href_t *ref_ptr);
H5_DLL herr_t   H5Rcreate_region(hid_t loc_id, const char *name, hid_t space_id, href_t *ref_ptr);
H5_DLL herr_t   H5Rcreate_attr(hid_t loc_id, const char *name, const char *attr_name, href_t *ref_ptr);
H5_DLL herr_t   H5Rdestroy(href_t ref);

/* Info */
H5_DLL H5R_type_t   H5Rget_type(href_t ref);
H5_DLL htri_t   H5Requal(href_t ref1, href_t ref2);
H5_DLL herr_t   H5Rcopy(href_t src_ref, href_t *dest_ref_ptr);

/* Dereference */
H5_DLL hid_t    H5Ropen_object(href_t ref, hid_t oapl_id);
H5_DLL hid_t    H5Ropen_region(href_t ref);
H5_DLL hid_t    H5Ropen_attr(href_t ref, hid_t aapl_id);

/* Get type */
H5_DLL herr_t   H5Rget_obj_type3(href_t ref, H5O_type_t *obj_type);

/* Get name */
H5_DLL ssize_t  H5Rget_file_name(href_t ref, char *name, size_t size);
H5_DLL ssize_t  H5Rget_obj_name(href_t ref, char *name, size_t size);
H5_DLL ssize_t  H5Rget_attr_name(href_t ref, char *name, size_t size);

/* Serialization */
H5_DLL herr_t   H5Rencode(href_t ref, void *buf, size_t *nalloc);
H5_DLL herr_t   H5Rdecode(const void *buf, href_t *ref_ptr);

/* Symbols defined for compatibility with previous versions of the HDF5 API.
 *
 * Use of these symbols is deprecated.
 */
#ifndef H5_NO_DEPRECATED_SYMBOLS

/* Macros */
#define H5R_DATASET_REGION H5R_REGION_COMPAT

/* Typedefs */

/* Function prototypes */
H5_DLL H5G_obj_t H5Rget_obj_type1(hid_t id, H5R_type_t ref_type,
    const void *_ref);
H5_DLL hid_t H5Rdereference1(hid_t obj_id, H5R_type_t ref_type,
    const void *ref);

H5_DLL herr_t H5Rcreate(void *ref, hid_t loc_id, const char *name,
                        H5R_type_t ref_type, hid_t space_id);
H5_DLL herr_t H5Rget_obj_type2(hid_t id, H5R_type_t ref_type, const void *_ref,
    H5O_type_t *obj_type);
H5_DLL hid_t H5Rdereference2(hid_t obj_id, hid_t oapl_id, H5R_type_t ref_type,
    const void *ref);
H5_DLL hid_t H5Rget_region(hid_t dataset, H5R_type_t ref_type, const void *ref);
H5_DLL ssize_t H5Rget_name(hid_t loc_id, H5R_type_t ref_type, const void *ref,
    char *name, size_t size);

#endif /* H5_NO_DEPRECATED_SYMBOLS */

#ifdef __cplusplus
}
#endif

#endif  /* _H5Rpublic_H */

