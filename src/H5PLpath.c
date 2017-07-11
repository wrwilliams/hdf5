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


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/

/* Stored plugin paths to search */
char           *H5PL_paths_g[H5PL_MAX_PATH_NUM];

/* The number of stored paths */
size_t          H5PL_num_paths_g = 0;

/* XXX: ???? */
hbool_t         H5PL_path_found_g = FALSE;


/*******************/
/* Local Variables */
/*******************/


/*-------------------------------------------------------------------------
 * Function:    H5PL_init_path_table
 *
 * Purpose:     Initialize the collection of paths that will be searched
 *              when loading plugins.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__init_path_table(void)
{
    char        *env_var= NULL;         /* Path string from environment variable */
    char        *paths = NULL;          /* Delimited paths string. Either from the
                                         * environment variable or the default.
                                         */
    char        *next_path = NULL;      /* A path tokenized from the paths string */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Retrieve paths from HDF5_PLUGIN_PATH if the user sets it
     * or from the default paths if it isn't set.
     */
    env_var = HDgetenv("HDF5_PLUGIN_PATH");
    if (NULL == env_var)
        paths = H5MM_strdup(H5PL_DEFAULT_PATH);
    else
        paths = H5MM_strdup(env_var);

    if (NULL == paths)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path copy")

    /* XXX: Try to minimize this usage */
    H5PL_EXPAND_ENV_VAR

    /* Separate the paths and store them */
    /* XXX: strtok() is not thread-safe */
    next_path = HDstrtok(paths, H5PL_PATH_SEPARATOR);
    while (next_path) {

        /* The path collection can only hold so many paths, so complain if
         * there are too many.
         */
        if (H5PL_MAX_PATH_NUM == H5PL_num_paths_g)
            HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "maximum number of plugin search directories stored")

        /* Insert the path into the table */
        if (H5PL__append_path(next_path) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't insert path: %s", next_path)

        /* Get the next path from the environment string */
        next_path = HDstrtok(NULL, H5PL_PATH_SEPARATOR);
    } /* end while */

    /* XXX: What is this for? */
    H5PL_path_found_g = TRUE;

done:
    if (paths)
        paths = (char *)H5MM_xfree(paths);

    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__init_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__close_path_table
 *
 * Purpose:     Initialize the collection of paths that will be searched
 *              when loading plugins.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__close_path_table(void)
{
    size_t      u;
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    for (u = 0; u < H5PL_num_paths_g; u++)
        if (H5PL_paths_g[u])
            H5PL_paths_g[u] = (char *)H5MM_xfree(H5PL_paths_g[u]);

    H5PL_num_paths_g = 0;
    H5PL_path_found_g = FALSE;

    FUNC_LEAVE_NOAPI(ret_value)

} /* end H5PL__close_path_table() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__get_num_paths
 *
 * Purpose:     Gets the number of plugin paths that have been stored.
 *
 * Return:      Success:    The number of paths
 *              Failture:   Can't fail
 *-------------------------------------------------------------------------
 */
size_t
H5PL__get_num_paths(void)
{
    FUNC_ENTER_PACKAGE_NOERR

    FUNC_LEAVE_NOAPI(H5PL_num_paths_g)

} /* end H5PL__get_num_paths() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__append_path
 *
 * Purpose:     Insert a path at the end of the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__append_path(const char *path)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Is the table full? */
    if (H5PL_num_paths_g == H5PL_MAX_PATH_NUM)
        HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "no room in path table to add new path")

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

    /* XXX: Try to minimize this usage */
    H5PL_EXPAND_ENV_VAR

    /* Insert the copy of the search path into the table at the current index */
    H5PL_paths_g[H5PL_num_paths_g] = path_copy;
    H5PL_num_paths_g++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__append_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__prepend_path
 *
 * Purpose:     Insert a path at the beginning of the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__prepend_path(const char *path)
{
    size_t u;                       /* iterator */
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Is the table full? */
    if (H5PL_num_paths_g == H5PL_MAX_PATH_NUM)
        HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "no room in path table to add new path")

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

    /* XXX: Try to minimize this usage */
    H5PL_EXPAND_ENV_VAR

    /* Copy the paths back to make a space at the head */
    for (u = H5PL_num_paths_g; u > 0; u--)
        H5PL_paths_g[u] = H5PL_paths_g[u-1];

    /* Insert the copy of the search path into the table at the head */
    H5PL_paths_g[0] = path_copy;
    H5PL_num_paths_g++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__append_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__replace_path
 *
 * Purpose:     Replace a path at particular index in the table.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__replace_path(const char *path, unsigned int index)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));
    HDassert(index < H5PL_MAX_PATH_NUM);

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

    /* XXX: Try to minimize this usage */
    H5PL_EXPAND_ENV_VAR

    /* Free up any existing path */
    if (H5PL_paths_g[index])
        H5PL_paths_g[index] = (char *)H5MM_xfree(H5PL_paths_g[index]);

    /* Insert the copy of the search path into the table at the index */
    H5PL_paths_g[index] = path_copy;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_path() */

