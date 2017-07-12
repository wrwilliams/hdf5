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
 * Purpose: This file contains declarations which are visible only within
 *          the H5PL package.  Source files outside the H5PL package should
 *          include H5PLprivate.h instead.
 */

#if !(defined H5PL_FRIEND || defined H5PL_MODULE)
#error "Do not include this file outside the H5PL package!"
#endif

#ifndef _H5PLpkg_H
#define _H5PLpkg_H

/* Include private header file */
#include "H5PLprivate.h"          /* Filter functions                */

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/* Whether to pre-load pathnames for plugin libraries */
#define H5PL_DEFAULT_PATH       H5_DEFAULT_PLUGINDIR


/****************************/
/* Macros for supporting    */
/* both Windows and POSIX   */
/****************************/

/*******************/
/* Windows support */
/*******************/
/*
 * SPECIAL WINDOWS NOTE
 *
 * Some of the Win32 API functions expand to fooA or fooW depending on
 * whether UNICODE or _UNICODE are defined. You MUST explicitly use
 * the A version of the functions to force char * behavior until we
 * work out a scheme for proper Windows Unicode support.
 *
 * If you do not do this, people will be unable to incorporate our
 * source code into their own CMake builds if they define UNICODE.
 */
#ifdef H5_HAVE_WIN32_API

#   define H5PL_PATH_SEPARATOR     ";"

    /* Handle for dynamic library */
#   define H5PL_HANDLE HINSTANCE

    /* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#   define H5PL_OPEN_DLIB(S) LoadLibraryExA(S, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)

    /* Get the address of a symbol in dynamic library */
#   define H5PL_GET_LIB_FUNC(H,N) GetProcAddress(H,N)

    /* Close dynamic library */
#   define H5PL_CLOSE_LIB(H) FreeLibrary(H)

    /* Clear error - nothing to do */
#   define H5PL_CLR_ERROR

    /* maximum size for expanding env vars */
#   define H5PL_EXPAND_BUFFER_SIZE 32767

    typedef const void *(__cdecl *H5PL_get_plugin_info_t)(void);

#else /* H5_HAVE_WIN32_API */

    /*****************/
    /* POSIX support */
    /*****************/

#   define H5PL_PATH_SEPARATOR     ":"

    /* Handle for dynamic library */
#   define H5PL_HANDLE void *

    /* Get a handle to a plugin library.  Windows: TEXT macro handles Unicode strings */
#   define H5PL_OPEN_DLIB(S) dlopen(S, RTLD_LAZY)

    /* Get the address of a symbol in dynamic library */
#   define H5PL_GET_LIB_FUNC(H,N) dlsym(H,N)

    /* Close dynamic library */
#   define H5PL_CLOSE_LIB(H) dlclose(H)

    /* Clear error */
#   define H5PL_CLR_ERROR HERROR(H5E_PLUGIN, H5E_CANTGET, "can't dlopen:%s", dlerror())

    typedef const void *(*H5PL_get_plugin_info_t)(void);
#endif /* H5_HAVE_WIN32_API */


/****************************/
/* Package Private Typedefs */
/****************************/

/* Type for the list of info for opened plugin libraries */
typedef struct H5PL_table_t {
    H5PL_type_t pl_type;            /* plugin type          */
    int         pl_id;              /* ID for the plugin    */
    H5PL_HANDLE handle;             /* plugin handle        */
} H5PL_table_t;


/*****************************/
/* Package Private Variables */
/*****************************/

/* These variables are defined in H5PLint.c */

/* Table for opened plugin libraries */
extern size_t           H5PL_table_alloc_g;
extern size_t           H5PL_table_used_g;
extern H5PL_table_t    *H5PL_table_g;

/* Bitmask that controls whether classes of plugins
 * (e.g.: filters, VOL drivers) can be loaded.
 */
extern unsigned int     H5PL_plugin_control_mask_g;

/* This flag will be set to TRUE if the HDF5_PLUGIN_PRELOAD
 * environment variable was set to H5PL_NO_PLUGIN at
 * package initialization.
 */
extern hbool_t          H5PL_never_allow_plugins_g;


/******************************/
/* Package Private Prototypes */
/******************************/

herr_t H5PL__init_path_table(void);
herr_t H5PL__close_path_table(void);
unsigned H5PL__get_num_paths(void);
herr_t H5PL__append_path(const char *path);
herr_t H5PL__prepend_path(const char *path);
herr_t H5PL__replace_path(const char *path, unsigned int index);
herr_t H5PL__insert_path(const char *path, unsigned int index);
herr_t H5PL__remove_path(unsigned int index);
const char *H5PL__get_path(unsigned int index);

#endif /* _H5PLpkg_H */

