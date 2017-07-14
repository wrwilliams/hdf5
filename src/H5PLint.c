/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5. The full HDF5 copyright notice, including      *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Purpose: Internal routines for managing plugins.
 *
 */


/****************/
/* Module Setup */
/****************/

#include "H5PLmodule.h"          /* This source code file is part of the H5PL module */


/***********/
/* Headers */
/***********/
#include "H5private.h"      /* Generic Functions            */
#include "H5Eprivate.h"     /* Error handling               */
#include "H5MMprivate.h"    /* Memory management            */
#include "H5PLpkg.h"        /* Plugin                       */
#include "H5Zprivate.h"     /* Filter pipeline              */


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static htri_t H5PL__find(H5PL_type_t plugin_type, int type_id, const char *dir, const void **info);
static htri_t H5PL__open(H5PL_type_t pl_type, char *libname, int plugin_id, const void **pl_info);


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

/* Bitmask that controls whether classes of plugins
 * (e.g.: filters, VOL drivers) can be loaded.
 */
static unsigned int     H5PL_plugin_control_mask_g = H5PL_ALL_PLUGIN;

/* This flag will be set to FALSE if the HDF5_PLUGIN_PRELOAD
 * environment variable was set to H5PL_NO_PLUGIN at
 * package initialization.
 */
static hbool_t          H5PL_allow_plugins_g = TRUE;



/*-------------------------------------------------------------------------
 * Function:    H5PL__get_plugin_control_mask
 *
 * Purpose:     Gets the internal plugin control mask value.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__get_plugin_control_mask(unsigned int *mask /*out*/)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Check args - Just assert on package functions */
    HDassert(mask);

    /* Return the mask */
    *mask = H5PL_plugin_control_mask_g;

    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5PL__get_plugin_control_mask() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__set_plugin_control_mask
 *
 * Purpose:     Sets the internal plugin control mask value.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__set_plugin_control_mask(unsigned int mask)
{
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Only allow setting this if plugins have not been disabled.
     * XXX: Note that we don't consider this an error, but instead
     *      silently ignore it. We may want to consider this behavior
     *      more carefully.
     */
    if (H5PL_allow_plugins_g)
        H5PL_plugin_control_mask_g = mask;

    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5PL__set_plugin_control_mask() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__init_package
 *
 * Purpose:     Initialize any package-specific data and call any init
 *              routines for the package.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__init_package(void)
{
    char        *env_var = NULL;
    herr_t      ret_value = SUCCEED;

    FUNC_ENTER_PACKAGE

    /* Check the environment variable to determine if the user wants
     * to ignore plugins. The special symbol H5PL_NO_PLUGIN (defined in
     * H5PLpublic.h) means we don't want to load plugins.
     */
    if (NULL != (env_var = HDgetenv("HDF5_PLUGIN_PRELOAD")))
        if (!HDstrcmp(env_var, H5PL_NO_PLUGIN)) {
            H5PL_plugin_control_mask_g = 0;
            H5PL_allow_plugins_g = FALSE;
        }

    /* Create the table of previously-loaded plugins */
    if (H5PL__create_plugin_cache() < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINIT, FAIL, "can't create plugin cache")

    /* Create the table of search paths for dynamic libraries */
    if (H5PL__create_path_table() < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINIT, FAIL, "can't create plugin search path table")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__init_package() */


/*-------------------------------------------------------------------------
 * Function:    H5PL_term_package
 *
 * Purpose:     Terminate the H5PL interface: release all memory, reset all
 *              global variables to initial values. This only happens if all
 *              types have been destroyed from other interfaces.
 *
 * Return:      Success:    Positive if any action was taken that might
 *                          affect some other interface; zero otherwise
 *              Failure:    Negative
 *
 *-------------------------------------------------------------------------
 */
int
H5PL_term_package(void)
{
    hbool_t already_closed = FALSE;
    int     ret_value = 0;

    FUNC_ENTER_NOAPI_NOINIT

    if (H5_PKG_INIT_VAR) {

        /* Close the plugin cache.
         * We need to bump the return value if we did any real work here.
         */
        if (H5PL__close_plugin_cache(&already_closed) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTFREE, (-1), "problem closing plugin cache")
        if (!already_closed)
            ret_value++;

        /* Close the search path table and free the paths */
        if (H5PL__close_path_table() < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTFREE, (-1), "problem closing search path table")

        /* Mark the interface as uninitialized */
        if (0 == ret_value)
            H5_PKG_INIT_VAR = FALSE;
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL_term_package() */


/*-------------------------------------------------------------------------
 * Function:    H5PL_load
 *
 * Purpose:     Given the plugin type and identifier, this function searches
 *              and/or loads a dynamic plugin library first among the already
 *              opened libraries then in the designated location paths.
 *
 * Return:      Success:    A pointer to the plugin info
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
const void *
H5PL_load(H5PL_type_t type, int id)
{
    H5PL_search_params_t    search_params;
    hbool_t                 found = FALSE;          /* Whether the plugin was found */
    const void             *plugin_info = NULL;
    const void             *ret_value = NULL;

    FUNC_ENTER_NOAPI(NULL)

    switch (type) {
        case H5PL_TYPE_FILTER:
            if ((H5PL_plugin_control_mask_g & H5PL_FILTER_PLUGIN) == 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTLOAD, NULL, "required dynamically loaded plugin filter '%d' is not available", id)
            break;

        case H5PL_TYPE_ERROR:
        case H5PL_TYPE_NONE:
        default:
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTLOAD, NULL, "required dynamically loaded plugin '%d' is not valid", id)
    } /* end switch */

    /* Search in the table of already loaded plugin libraries */
    search_params.type = type;
    search_params.id = id;
    if(H5PL__get_cached_plugin(&search_params, &found, (void **)&plugin_info) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, NULL, "search in table failed")

    /* If not found, iterate through the path table to find the right dynamic library */
    if (!found) {
        size_t      i;                  /* Local index variable */
        size_t      num_paths;          /* Number of paths in table */

        num_paths = H5PL__get_num_paths();

        for (i = 0; i < num_paths; i++) {

            const char *path = H5PL__get_path((unsigned int)i);

            if ((found = H5PL__find(type, id, path, &plugin_info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, NULL, "search in paths failed")

            /* Break out if found */
            if (found) {
                HDassert(plugin_info);
                break;
            } /* end if */
        } /* end for */
    } /* end if */

    /* Check if we found the plugin */
    if (found)
        ret_value = plugin_info;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL_load() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__find
 *
 * Purpose:     Given a path, this function opens the directory and envokes
 *              another function to go through all files to find the right
 *              plugin library. Two function definitions are for Unix and
 *              Windows.
 *
 * Return:      TRUE on success,
 *              FALSE on not found,
 *              negative on failure
 *
 *-------------------------------------------------------------------------
 */
#ifndef H5_HAVE_WIN32_API
static htri_t
H5PL__find(H5PL_type_t plugin_type, int type_id, const char *dir, const void **info)
{
    char           *pathname = NULL;
    DIR            *dirp = NULL;
    struct dirent  *dp = NULL;
    htri_t         ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* Open the directory */
    if (!(dirp = HDopendir(dir)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory: %s", dir)

    /* Iterates through all entries in the directory to find the right plugin library */
    while (NULL != (dp = HDreaddir(dirp))) {
        /* The library we are looking for should be called libxxx.so... on Unix
         * or libxxx.xxx.dylib on Mac.
         */
#ifndef __CYGWIN__
        if (!HDstrncmp(dp->d_name, "lib", (size_t)3) &&
                (HDstrstr(dp->d_name, ".so") || HDstrstr(dp->d_name, ".dylib"))) {
#else
        if (!HDstrncmp(dp->d_name, "cyg", (size_t)3) &&
                HDstrstr(dp->d_name, ".dll") ) {

#endif
            h5_stat_t   my_stat;
            size_t      pathname_len;
            htri_t      found_in_dir;

            /* Allocate & initialize the path name */
            pathname_len = HDstrlen(dir) + HDstrlen(dp->d_name) + 2;
            if (NULL == (pathname = (char *)H5MM_malloc(pathname_len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")
            HDsnprintf(pathname, pathname_len, "%s/%s", dir, dp->d_name);

            /* Get info for directory entry */
            if (HDstat(pathname, &my_stat) == -1)
                HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "can't stat file: %s", HDstrerror(errno))

            /* If it is a directory, skip it */
            if (S_ISDIR(my_stat.st_mode))
                continue;

            /* Attempt to open the dynamic library as a filter library */
            if ((found_in_dir = H5PL__open(plugin_type, pathname, type_id, info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if (found_in_dir)
                HGOTO_DONE(TRUE)    /* Indicate success */
            pathname = (char *)H5MM_xfree(pathname);
        } /* end if */
    } /* end while */

done:
    if (dirp)
        if (HDclosedir(dirp) < 0)
            HDONE_ERROR(H5E_FILE, H5E_CLOSEERROR, FAIL, "can't close directory: %s", HDstrerror(errno))
    pathname = (char *)H5MM_xfree(pathname);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find() */
#else /* H5_HAVE_WIN32_API */
static htri_t
H5PL__find(H5PL_type_t plugin_type, int type_id, const char *dir, const void **info)
{
    WIN32_FIND_DATAA    fdFile;
    HANDLE              hFind;
    char                *pathname = NULL;
    char                service[2048];
    htri_t              ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* Specify a file mask. *.* = We want everything! */
    sprintf(service, "%s\\*.dll", dir);
    if ((hFind = FindFirstFileA(service, &fdFile)) == INVALID_HANDLE_VALUE)
        HGOTO_ERROR(H5E_PLUGIN, H5E_OPENERROR, FAIL, "can't open directory")

    do {
        /* Find first file will always return "."
         * and ".." as the first two directories.
         */
        if (HDstrcmp(fdFile.cFileName, ".") != 0 && HDstrcmp(fdFile.cFileName, "..") != 0) {
            size_t      pathname_len;
            htri_t      found_in_dir;

            /* Allocate & initialize the path name */
            pathname_len = HDstrlen(dir) + HDstrlen(fdFile.cFileName) + 2;
            if (NULL == (pathname = (char *)H5MM_malloc(pathname_len)))
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path")
            HDsnprintf(pathname, pathname_len, "%s\\%s", dir, fdFile.cFileName);

            /* Is the entity a File or Folder? */
            if (fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            if ((found_in_dir = H5PL__open(plugin_type, pathname, type_id, info)) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "search in directory failed")
            if (found_in_dir)
                HGOTO_DONE(TRUE)    /* Indicate success */
            pathname = (char *)H5MM_xfree(pathname);
        } /* end if */
    } while (FindNextFileA(hFind, &fdFile)); /* Find the next file. */

done:
    if (hFind)
        FindClose(hFind);
    if (pathname)
        pathname = (char *)H5MM_xfree(pathname);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__find() */
#endif /* H5_HAVE_WIN32_API */


/*-------------------------------------------------------------------------
 * Function:    H5PL__open
 *
 * Purpose:     Iterates through all files to find the right plugin library.
 *              It loads the dynamic plugin library and keeps it on the list
 *              of loaded libraries.
 *
 * Return:      TRUE on success,
 *              FALSE on not found,
 *              negative on failure
 *
 *-------------------------------------------------------------------------
 */
static htri_t
H5PL__open(H5PL_type_t pl_type, char *libname, int pl_id, const void **pl_info)
{
    H5PL_HANDLE    handle = NULL;
    htri_t         ret_value = FALSE;

    FUNC_ENTER_STATIC

    /* There are different reasons why a library can't be open, e.g. wrong architecture.
     * simply continue if we can't open it.
     */
    if (NULL == (handle = H5PL_OPEN_DLIB(libname))) {
        H5PL_CLR_ERROR; /* clear error */
    } /* end if */
    else {
        H5PL_get_plugin_info_t get_plugin_info = NULL;

        /* Return a handle for the function H5PLget_plugin_info in the dynamic library.
         * The plugin library is suppose to define this function.
         *
         * NOTE: We turn off -Wpedantic in gcc to quiet a warning about converting
         *       object pointers to function pointers, which is undefined in ANSI C.
         *       This is basically unavoidable due to the nature of dlsym() and *is*
         *       defined in POSIX, so it's fine.
         */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        if (NULL == (get_plugin_info = (H5PL_get_plugin_info_t)H5PL_GET_LIB_FUNC(handle, "H5PLget_plugin_info"))) {
#pragma GCC diagnostic pop
            if(H5PL__close(handle) < 0)
                HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
        } /* end if */
        else {
            const H5Z_class2_t *plugin_info;

            /* Invoke H5PLget_plugin_info to verify this is the right library we are looking for.
             * Move on if it isn't.
             */
            if (NULL == (plugin_info = (const H5Z_class2_t *)(*get_plugin_info)())) {
                if (H5PL__close(handle) < 0)
                    HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
                HGOTO_ERROR(H5E_PLUGIN, H5E_CANTGET, FAIL, "can't get plugin info")
            } /* end if */

            /* Successfully found plugin library.
             *
             * Check if the IDs match, and, if they do, add the new plugin to the cache
             * and set output values.
             */
            if (plugin_info->id == pl_id) {

                H5PL_plugin_t   new_plugin;

                new_plugin.handle   = handle;
                new_plugin.type     = pl_type;
                new_plugin.id       = plugin_info->id;

                /* Store the plugin in the cache */
                if (H5PL__add_plugin(&new_plugin))
                    HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to add new plugin to plugin cache")

                /* Set the plugin info to return */
                *pl_info = (const void *)plugin_info;

                /* Indicate success */
                ret_value = TRUE;
            } /* end if */
            else
                if (H5PL__close(handle) < 0)
                    HGOTO_ERROR(H5E_PLUGIN, H5E_CLOSEERROR, FAIL, "can't close dynamic library")
        } /* end if */
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__open() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__close
 *
 * Purpose:     Closes the handle for dynamic library
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__close(H5PL_HANDLE handle)
{
    FUNC_ENTER_PACKAGE_NOERR

    H5PL_CLOSE_LIB(handle);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* end H5PL__close() */

