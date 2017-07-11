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

/* This is required to expand Microsoft Windows environment variable strings
 * containing things like %variableName%. The ExpandEnvironmentStrings() API
 * call will replace the variables with the user's current values for them.
 */
/* XXX: This should not be a macro */
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


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Local Prototypes */
/********************/

static herr_t H5PL__insert_at(const char *path, unsigned int index);
static herr_t H5PL__make_space_at(unsigned int index);
static herr_t H5PL__replace_at(const char *path, unsigned int index);

/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/

/* Stored plugin paths to search */
char          **H5PL_paths_g = NULL;

/* The number of stored paths */
size_t          H5PL_num_paths_g = 0;

/* XXX: ???? */
hbool_t         H5PL_path_found_g = FALSE;


/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:    H5PL__insert_at()
 *
 * Purpose:     Insert a path at a particular index in the path table.
 *              Does not clobber! Will move existing paths up to make
 *              room. Use H5PL__replace_at(index) if you want to clobber.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__insert_at(const char *path, unsigned int index)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Is the table full? */
    if (H5PL_num_paths_g == H5PL_MAX_PATH_NUM)
        HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "no room in path table to add new path")

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

    /* Clean up Microsoft Windows environment variables in the path string */
    H5PL_EXPAND_ENV_VAR

    /* If the table entry is in use, make some space */
    if (H5PL_paths_g[index])
        if (H5PL__make_space_at(index) < 0)
            HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "unable to make space in the table for the new entry")

    /* Insert the copy of the search path into the table at the specified index */
    H5PL_paths_g[index] = path_copy;
    H5PL_num_paths_g++;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__insert_at() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__make_space_at()
 *
 * Purpose:     Free up a slot in the path table, moving existing path
 *              entries as necessary.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__make_space_at(unsigned int index)
{
    size_t  u;                      /* iterator */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(index < H5PL_MAX_PATH_NUM);

    /* Check if the path table is full */
    if (H5PL_num_paths_g == H5PL_MAX_PATH_NUM)
        HGOTO_ERROR(H5E_PLUGIN, H5E_NOSPACE, FAIL, "no room in path table to add new path")

    /* Copy the paths back to make a space  */
    for (u = H5PL_num_paths_g; u > index; u--)
        H5PL_paths_g[u] = H5PL_paths_g[u-1];

    H5PL_paths_g[index] = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__make_space_at() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__replace_at()
 *
 * Purpose:     Replace a path at a particular index in the path table.
 *              The path in the table must exist and will be freed by this
 *              function.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PL__replace_at(const char *path, unsigned int index)
{
    char    *path_copy = NULL;      /* copy of path string (for storing) */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Check that the table entry is in use */
    if (!H5PL_paths_g[index])
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTFREE, FAIL, "path entry at index %u in the table is NULL", index)

    /* Copy the path for storage so the caller can dispose of theirs */
    if (NULL == (path_copy = H5MM_strdup(path)))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't make internal copy of path")

    /* Clean up Microsoft Windows environment variables in the path string */
    H5PL_EXPAND_ENV_VAR

    /* Free the existing path entry */
    H5PL_paths_g[index] = (char *)H5MM_xfree(H5PL_paths_g[index]);

    /* Copy the search path into the table at the specified index */
    H5PL_paths_g[index] = path_copy;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_at() */


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

    /* Allocate memory for the path table */
    if (NULL == (H5PL_paths_g = (char *)H5MM_calloc((size_t)H5PL_MAX_PATH_NUM * sizeof(char *))))
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTALLOC, FAIL, "can't allocate memory for path table")

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
    size_t      u;                      /* iterator */
    herr_t      ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE_NOERR

    /* Free paths */
    for (u = 0; u < H5PL_num_paths_g; u++)
        if (H5PL_paths_g[u])
            H5PL_paths_g[u] = (char *)H5MM_xfree(H5PL_paths_g[u]);

    /* Free path table */
    H5PL_paths_g = (char *)H5MM_xfree(H5PL_paths_g);

    /* Reset values */
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
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Insert the path at the end of the table */
    if (H5PL__insert_at(path, (unsigned int)H5PL_num_paths_g) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to append search path")

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
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));

    /* Insert the path at the beginning of the table */
    if (H5PL__insert_at(path, 0) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to prepend search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__prepend_path() */


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
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));
    HDassert(index < H5PL_MAX_PATH_NUM);

    /* Insert the path at the requested index */
    if (H5PL__replace_at(path, index) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to replace search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__insert_path
 *
 * Purpose:     Insert a path at particular index in the table, moving
 *              any existing paths back to make space.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__insert_path(const char *path, unsigned int index)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(path);
    HDassert(HDstrlen(path));
    HDassert(index < H5PL_MAX_PATH_NUM);

    /* Insert the path at the requested index */
    if (H5PL__insert_at(path, index) < 0)
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTINSERT, FAIL, "unable to insert search path")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__insert_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__remove_path
 *
 * Purpose:     Remove a path at particular index in the table, freeing
 *              the path string and moving the paths down to close the gap.
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PL__remove_path(unsigned int index)
{
    size_t      u;                  /* iterator */
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Check args - Just assert on package functions */
    HDassert(index < H5PL_MAX_PATH_NUM);

    /* Check if the path at that index is set */
    if (!H5PL_paths_g[index])
        HGOTO_ERROR(H5E_PLUGIN, H5E_CANTDELETE, FAIL, "search path at index %u is NULL", index)

    /* Delete the path */
    H5PL_num_paths_g--;
    H5PL_paths_g[index] = (char *)H5MM_xfree(H5PL_paths_g[index]);

    /* Shift the paths down to close the gap */
    for (u = index; u < H5PL_num_paths_g; u++)
        H5PL_paths_g[u] = H5PL_paths_g[u+1];

    /* Set the (former) last path to NULL */
    H5PL_paths_g[H5PL_num_paths_g] = NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__remove_path() */


/*-------------------------------------------------------------------------
 * Function:    H5PL__get_path
 *
 * Purpose:     Get a pointer to a path at particular index in the table.
 *
 * Return:      Success:    A pointer to a path string stored in the table
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
const char *
H5PL__get_path(unsigned int index)
{
    char    *ret_value = NULL;    /* Return value */

    FUNC_ENTER_PACKAGE

    /* Insert the path at the requested index */
    if (index >= H5PL_num_paths_g)
        HGOTO_ERROR(H5E_ARGS, H5E_BADRANGE, NULL, "path index %u is out of range in table", index)

    return H5PL_paths_g[index];
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PL__replace_path() */

