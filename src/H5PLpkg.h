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

/* Max number of paths supported */
#define H5PL_MAX_PATH_NUM       16

/* Whether to pre-load pathnames for plugin libraries */
#define H5PL_DEFAULT_PATH       H5_DEFAULT_PLUGINDIR

/* Special symbol to indicate no plugin loading */
#define H5PL_NO_PLUGIN          "::"


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

#ifdef H5_HAVE_WIN32_API
#define H5PL_EXPAND_ENV_VAR {                                                            \
        long bufCharCount;                                                               \
        char *tempbuf;                                                                   \
        if(NULL == (tempbuf = (char *)H5MM_malloc(H5PL_EXPAND_BUFFER_SIZE)))             \
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for expanded path")                          \
        if((bufCharCount = ExpandEnvironmentStringsA(dl_path, tempbuf, H5PL_EXPAND_BUFFER_SIZE)) > H5PL_EXPAND_BUFFER_SIZE) { \
            tempbuf = (char *)H5MM_xfree(tempbuf);                                       \
            HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "expanded path is too long")      \
        }                                                                                \
        if(bufCharCount == 0) {                                                          \
            tempbuf = (char *)H5MM_xfree(tempbuf);                                       \
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "failed to expand path")          \
        }                                                                                \
        dl_path = (char *)H5MM_xfree(dl_path);                                           \
        dl_path = tempbuf;                                                               \
 }
#else
#define H5PL_EXPAND_ENV_VAR
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


/******************************/
/* Package Private Prototypes */
/******************************/

#endif /* _H5PLpkg_H */

