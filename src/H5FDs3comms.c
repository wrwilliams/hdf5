/*
 * Source for S3 Communications module
 *
 * ***NOT A FILE DRIVER***
 *
 */

/****************/
/* Module Setup */
/****************/

/***********/
/* Headers */
/***********/

#include "H5private.h"
#include "H5FDs3comms.h"



/****************/
/* Local Macros */
/****************/

/* test performance one way or the other? */
#define GETSIZE_STATIC_BUFFER 1

/*
#define VERBOSE_CURL
*/



/******************/
/* Local Typedefs */
/******************/

/* struct s3r_datastruct
 * Structure passed to curl write callback
 * pointer to data region and record of bytes written (offset)
 */
struct s3r_datastruct {
    char   *data;
    size_t  size;
};



/********************/
/* Local Prototypes */
/********************/

size_t curlwritecallback(char   *ptr,
                         size_t  size,
                         size_t  nmemb,
                         void   *userdata);



/*********************/
/* Package Variables */
/*********************/

/*****************************/
/* Library Private Variables */
/*****************************/

/*******************/
/* Local Variables */
/*******************/




/*****************************************************************************
 *
 * Function: curlwritecallback()
 *
 * Purpose:
 *
 *    Function called by CURL to write received data.
 *    Writes bytes to `userdata`.
 *
 * Return:
 *
 *    Number of bytes processed.
 *    Should equal number of bytes passed to callback;
 *        failure will result in curl error: CURLE_WRITE_ERROR.
 *
 * Programmer: Jacob Smith
 *             2017-08-17
 *
 * Changes: None.
 *
 *****************************************************************************/
size_t
curlwritecallback(char   *ptr,
                  size_t  size,
                  size_t  nmemb,
                  void   *userdata)
{
    size_t written = 0;
    size_t product = (size * nmemb);
    struct s3r_datastruct *sds = (struct s3r_datastruct *)userdata;

    if (size > 0) {
        memcpy(&(sds->data[sds->size]), ptr, product);
        sds->size += product;
        written = product;
    }

    return written;

} /* curlwritecallback */


/****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_fl_destroy()
 *
 * Purpose:
 *
 *     Remove all references to a field list structure.
 *
 *     Traverses entire list, both forwards and backwards from supplied node.
 *
 *     Releases all string pointers and structure pointers.
 *
 *     If `L` is NULL, there is no effect.
 *
 * Return:
 *
 *     SUCCESS 
 *     FAIL iff `L` does not have appropriate `magic` component.
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 * Changes: None.
 *
 ****************************************************************************/
herr_t 
H5FD_s3comms_hrb_fl_destroy(hrb_fl_t_2 *L)
{
    hrb_fl_t_2 *ptr = NULL;



    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stdout, "called H5FD_s3comms_hrb_fl_destroy.\n");

    if (L != NULL) {
        HDassert(L->magic = S3COMMS_HRB_FL_MAGIC);

        /* go the "start" of the sorted list.
         */
        while (L->prev_lower != NULL) {
            L = L->prev_lower;
        }

        /* release each node in turn
         */
        do { 
            ptr = L->next_lower;

            free(L->name);
            free(L->value);
            free(L->cat);
            free(L->lowername);
            free(L);

            L = ptr;
        } while (L != NULL);   
    } /* if L != NULL */

    FUNC_LEAVE_NOAPI(SUCCEED)

} /* H5FD_s3comms_hrb_fl_destroy */


/****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_fl_first()
 *
 * Purpose:
 *
 *     Get the first item in the hrb_fl_t list, according to given ordering.
 *     
 *     If HRB_FL_ORD_GIVEN, returns first item entered into set.
 *     If HRB_FL_ORD_SORTED, returns item with "lowest" lowercase name
 *         as calculated with `strcmp`.
 *
 *     `L` may be NULL (the empty set), which returns NULL.
 *
 * Return:
 *
 *     Pointer to `hrb_fl_t_2` structure.
 *     If `L` is NULL, returns null.
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 * Changes: None.
 *
 ****************************************************************************/
hrb_fl_t_2 *
H5FD_s3comms_hrb_fl_first(hrb_fl_t_2      *L, 
                          enum HRB_FL_ORD  ord)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDfprintf(stdout, "called H5FD_s3comms_hrb_fl_first.\n");

    if (L != NULL) {
        HDassert(L->magic == S3COMMS_HRB_FL_MAGIC);
       if (ord == HRB_FL_ORD_SORTED) {
            while (L->prev_lower != NULL) {
                L = L->prev_lower;
            }
       } else {
            while (L->prev != NULL) {
                L = L->prev;
            }
        }
    }

    FUNC_LEAVE_NOAPI(L);

} /* H5FD_s3comms_hrb_fl_first */


/****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_fl_next()
 *
 * Purpose:
 *
 *     Logical wrapper to fetch the next item in the list,
 *     according to given ordering enum `ord`.
 *
 *        ORDERED - sorted that strncmp(n1->lowername, n2->lowername) < 0)
 *        GIVEN   - sorted in order added
 *
 *     Does not search for start of list -- proceeds to NEXT from given node.
 *     It is advised to call `...first()` to set the starting point of
 *     any iteration with `...next()`.
 *
 *     The following lines are equivalent, assuming no error:
 *
 *         `nxt = H5FD_s3comms_hrb_fl_next(node, how);`
 *         `nxt = (how == HRB_FL_ORD_GIVEN) ? node->next : node->next_lower;`
 *
 *
 * Return:
 *
 *     NULL if: 
 *         `L` is null
 *         next item in ordering is NULL
 *     if `ord` == HRB_FL_ORD_GIVEN
 *         L->next
 *     if `ord` = HRB_FL_ORD_SORTED -- sorted by lowercase name
 *         L->next_lower
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 * Changes: None.
 *
 ****************************************************************************/
hrb_fl_t_2 *
H5FD_s3comms_hrb_fl_next(hrb_fl_t_2     *L, 
                        enum HRB_FL_ORD  ord)
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

/* note: always-succeeds nature raises comipler warnings about unused stuff */

    HDfprintf(stdout, "called H5FD_s3comms_hrb_fl_next.\n");

    if (L != NULL) {
        HDassert(L->magic == S3COMMS_HRB_FL_MAGIC);
       if (ord == HRB_FL_ORD_SORTED) {
           L = L->next_lower;
       } else {
           L = L->next;
        }
    }

    FUNC_LEAVE_NOAPI(L);

} /* H5FD_s3comms_hrb_fl_next */


/****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_fl_set()
 *
 * Purpose:
 *
 *     Interact with the elements in a `hrb_fl_t_2` field list node, and
 *     its attached list nodes.
 *
 *     Create, 
 *     modify elements in,
 *     remove elements from,
 *     and insert elements into
 *     the given list.
 *
 *     If `L` is NULL, a new list node is created.
 *
 *     `name` cannot be null; if null, has no effect and `L` is returned.
 *
 *     Entries accessed with the lowercase representation of their name...
 *         "Host", "host", and "hOSt" would all access the same node
 *         but creating or modifying a node would result in a case-matching
 *         name (and "Name: Value" concatenation).
 *
 *    CREATE
 *
 *        If `L` is NULL and `name` and `value` are not NULL,
 *        a new node is created and returned, starting a list.
 *
 *    MODIFY
 *
 *        If a node is found with a matching lowercase name and `value`
 *        is not NULL, the existing name, value, and cat values are released
 *        and replaced with the new data.
 *
 *        No modifications are made to the list pointers.
 *
 *     REMOVE
 *
 *         If a list is called to set with a NULL `value`, will attempt to 
 *         remove node with matching lowercase name.
 *
 *         If no match found, has no effect.
 *
 *         When removing a node, all its resources is released.
 *
 *         If node removed is pointed to by `L`, attempts to return first of:
 *             L->prev_lower,
 *             L->next_lower
 *         If neither exists, `L` was the only node in the list; L is expunged
 *             and NULL is returned.
 *
 *    INSERT
 *
 *        If no nodes exists with matching lowercase name and `value`
 *        is not NULL, a new node is created.
 *
 *        New node is appended at end of GIVEN ordering, and
 *        inserted into SORTED ordering.
 *
 *        See `H5FD_s3comms_hrb_fl_next`
 *
 * Return:
 *
 *     Pointer to hrb_fl_t_2 node if
 *         passed-in if node is not removed
 *         new node if one is created from NULL input
 *         if `L` is removed,
 *             L->prev_lower (if exists) or
 *             L->prev_next (if exists)
 *     NULL if
 *         cannot create node and `L` is NULL 
 *            e.g., `set(NULL, NULL, "blah")`
 *                  `set(NULL, "anything", NULL)`
 *         only node in `L` was removed
 *
 * Programmer: Jacob Smith
 *             2017-09-22
 *
 ****************************************************************************/
hrb_fl_t_2 *
H5FD_s3comms_hrb_fl_set(hrb_fl_t_2 *L, 
                        const char *name, 
                        const char *value)
{
    size_t      i          = 0;
    hbool_t     is_looking = TRUE;
    char       *nvcat      = NULL;
    char       *lowername  = NULL;
    char       *namecpy    = NULL;
    hrb_fl_t_2 *new_node   = NULL;
    hrb_fl_t_2 *ptr        = NULL;
    char       *valuecpy   = NULL;
    hrb_fl_t_2 *ret_value  = NULL;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_hrb_fl_set.\n");

    if (name == NULL) {
        /* alternatively, warn and abort?
         */
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L,
                    "unable to operate on null name.\n");
    }



    /***********************
     * PREPARE ALL STRINGS *
     **********************/

    namecpy = (char *)malloc(sizeof(char) * strlen(name) + 1);
    if (namecpy == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L,
                    "cannot make space for name copy.\n");
    }
    memcpy(namecpy, name, strlen(name) + 1);

    /* if we have a value supplied, copy it and prepare name:value string
     */
    if (value != NULL) {
        valuecpy = (char *)malloc(sizeof(char) * strlen(value) + 1);
        if (valuecpy == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L,
                        "cannot make space for value copy.\n");
        }
        memcpy(valuecpy, value, strlen(value) + 1);

        nvcat = (char *)malloc(sizeof(char) * strlen(name)+strlen(value) + 3);
        if (nvcat == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L,
                        "cannot make space for concatenated string.\n");
        }
        sprintf(nvcat, "%s: %s", name, value);
    }

    /* copy and lowercase name
     */
    lowername = (char *)malloc(sizeof(char) * strlen(name) + 1);
    if (lowername == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L,
                    "cannot make space for lowercase name copy.\n");
    }
    for (i = 0; i < strlen(name); i++) {
        lowername[i] = (char)tolower((int)name[i]);
    }
    lowername[strlen(name)] = 0;

    /* create new_node, should we need it 
     */
    new_node = (hrb_fl_t_2 *)malloc(sizeof(hrb_fl_t_2));
    if (new_node == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, L, 
                        "cannot make space for new set.\n");
    }
    new_node->magic      = S3COMMS_HRB_FL_MAGIC;
    new_node->name       = NULL;
    new_node->value      = NULL;
    new_node->cat        = NULL;
    new_node->lowername  = NULL;
    new_node->next       = NULL;
    new_node->next_lower = NULL;
    new_node->prev       = NULL;
    new_node->prev_lower = NULL;



    /************************************
     * INSERT, MODIFY, OR REMOVE A NODE *
     ************************************/

    if (L == NULL)  {
        if (value != NULL) { /* must have value to create new list */

            /******************************
             * CREATE NEW OBJECT AND LIST *
             ******************************/

            new_node->magic      = S3COMMS_HRB_FL_MAGIC;
            new_node->cat        = nvcat;
            new_node->name       = namecpy;
            new_node->lowername  = lowername;
            new_node->value      = valuecpy;
    
            L = new_node;
        }

    } else {
        /* L is not NULL; look for insertion point and remove/modify/insert 
         */
        ptr = L;


        /********************************
         * FIND WHERE TO ACT, AND DO IT *
         ********************************/

        while (is_looking == TRUE) {
            /* look at each node
             * if lowername matches
             *     if value is NULL, remove node
             *     else,             replace value and cat    
             * if no match, create a new node and insert:
             *     in sorted order by `lowername`
             *     at end of `next`
             */
            if (strcmp(lowername, ptr->lowername) == 0) {
                /* lowername match with this node 
                 */

                if (value == NULL) {
    
                    /**********
                     * REMOVE *
                     **********/

                    /* shuffle pointers to unlink node
                     * be careful about null endpoints
                     */
                    if (ptr->next != NULL) {
                        ptr->next->prev = ptr->prev;
                    }
                    if (ptr->prev != NULL) {
                        ptr->prev->next = ptr->next;
                    }
                    if (ptr->next_lower != NULL) {
                        ptr->next_lower->prev_lower = ptr->prev_lower;
                    }
                    if (ptr->prev_lower != NULL) {
                        ptr->prev_lower->next_lower = ptr->next_lower;
                    }

                    /* if removing the pointer we received,
                     * look for replacement node
                     * ORDER: prev_lower, next_lower
                     * if neither exist, than list has been emptied as is NULL
                     */
                    if (ptr == L) {
                        if (ptr->prev_lower != NULL) {
                            L = ptr->prev_lower;
                        } else if (ptr->next_lower != NULL) {
                            L = ptr->next_lower;
                        } else {
                            L = NULL;
                        }
                    }

                    /* release resources 
                     */
                    free(ptr->cat);
                    free(ptr->lowername);
                    free(ptr->name);
                    free(ptr->value);
                    free(ptr);

                    free(lowername); lowername = NULL;
                    free(namecpy);   namecpy   = NULL;
                    free(new_node);  new_node  = NULL;
                    free(nvcat);     nvcat     = NULL;
                    free(valuecpy);  valuecpy  = NULL;
 
                    is_looking = FALSE; /* stop looking for where to act */

                } else { /* value != NULL */

                    /**********
                     * MODIFY *
                     **********/

                    /* release previous strings, and set new ones
                     */
                    free(ptr->cat);   ptr->cat   = nvcat;
                    free(ptr->name);  ptr->name  = namecpy;
                    free(ptr->value); ptr->value = valuecpy;

                    free(lowername);  lowername = NULL;
                    free(new_node);   new_node  = NULL;
                
                    is_looking = FALSE; /* stop looking for where to act */
                }
            } else if (strcmp(lowername, ptr->lowername) < 0) {
                /* given lowername is "before" this node (sorted)
                 */

                if (ptr->prev_lower == NULL) {
                    /* no node exists "before" this node (sorted)
                     */ 

                    if (value != NULL) {

                        /*****************************
                         * INSERT at FIRST of sorted *
                         *****************************/

                        new_node->magic     = S3COMMS_HRB_FL_MAGIC;
                        new_node->cat       = nvcat;
                        new_node->lowername = lowername;
                        new_node->name      = namecpy;
                        new_node->value     = valuecpy;

                        /* SET DOUBLY-LINKED LIST POINTERS 
                         */
                        new_node->prev_lower = NULL;
                        new_node->next_lower = ptr;

                        ptr->prev_lower      = new_node;

                        /* append new node to end of "given" order
                         */
                        while (ptr->next != NULL) { 
                            ptr = ptr->next; 
                        }
                        new_node->next = NULL;
                        ptr->next      = new_node;
                        new_node->prev = ptr;

                    } /* if (value != NULL) */

                  is_looking = FALSE;


                } else if (strcmp(lowername, ptr->prev_lower->lowername) > 0) {
                    /* lowername sits "between" this and previous node (sorted)
                     */

                    if (value != NULL) {

                        /******************
                         * INSERT BETWEEN *
                         ******************/

                        new_node->magic     = S3COMMS_HRB_FL_MAGIC;
                        new_node->cat       = nvcat;
                        new_node->lowername = lowername;
                        new_node->name      = namecpy;
                        new_node->value     = valuecpy;

                        /* SET DOUBLY-LINKED LIST POINTERS 
                         */
                        new_node->next_lower = ptr;
                        new_node->prev_lower = ptr->prev_lower;

                        ptr->prev_lower->next_lower = new_node;
                        ptr->prev_lower             = new_node;
                    
                        /* append new node to end of "given" order i
                         */
                        while (ptr->next != NULL) { 
                            ptr = ptr->next; 
                        }
                        new_node->next = NULL;
                        ptr->next      = new_node;
                        new_node->prev = ptr;

                    } /* if (value != NULL) */

                    is_looking = FALSE;

                } else { /* lowername > prev->lowername */
                    /* lowername before or at previous node (sorted)
                     */

                    /************************
                     * MOVE TO EARLIER NODE *
                     ************************/

                    ptr = ptr->prev_lower;

                } /* if lowername > ptr->prev->lowername */

            } else {
                /* lowername is "after" this node
                 */

                if (ptr->next_lower == NULL) {
                    /* no next node; create?
                     */

                    if (value != NULL) {

                        /***************************
                         * INSERT at END of sorted *
                         ***************************/

                        new_node->magic     = S3COMMS_HRB_FL_MAGIC;
                        new_node->cat       = nvcat;
                        new_node->lowername = lowername;
                        new_node->name      = namecpy;
                        new_node->value     = valuecpy;

                        /* SET DOUBLY-LINKED LIST POINTERS 
                         */
                        new_node->prev_lower = ptr;
                        new_node->next_lower = NULL;
                        ptr->next_lower      = new_node;

                        /* append new node to end of "given" order
                         */
                        while (ptr->next != NULL) { 
                            ptr = ptr->next; 
                        }
                        new_node->next = NULL;
                        ptr->next      = new_node;
                        new_node->prev = ptr;

                    } /* if (value != NULL) */
                   
                    is_looking = FALSE;

                } else { /* if ptr->next_lower == NULL */

                    /*********************
                     * MOVE TO NEXT NODE *
                     *********************/

                    ptr = ptr->next_lower;
                } /* if ptr->next_lower == NULL */

            } /* if lowername >, <, == ptr->lower */

        } /* while (is_looking) */

    } /* if/else L == NULL */

    ret_value = L;

done:
    /* in event of error upon creation (malloc error?), clean up 
     */
    if (ret_value == NULL) {
        if (nvcat     != NULL) free(nvcat);
        if (namecpy   != NULL) free(namecpy);
        if (lowername != NULL) free(lowername);
        if (valuecpy  != NULL) free(valuecpy);
        if (new_node  != NULL) free(new_node);
    }

    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_hrb_fl_set */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_destroy()
 *
 * Purpose:
 *
 *    Destroy and free all structure resources associated with
 *    an HTTP Buffer. 
 *
 *    It is left to programmer to ensure that all external references to the 
 *    buffer or any of its contents are unused after this call.
 *
 *    Headers list node at `first_headers` is not touched;
 *    programmer should re-use or destroy `hrb_fl_t_2` pointer as suits their
 *    purposes.
 *
 *    If buffer argument is NULL, there is no effect.
 *
 * Return: 
 *
 *     SUCCEED 
 *     FAIL iff `buf != NULL && buf->magic != S3COMMS_HRB_MAGIC`
 *
 * Programmer: Jacob Smith
 *             2017-07-21
 *
 * Changes:
 *
 *    Conditional free() of `hrb_fl_t` pointer properties based on
 *    `which_free` property.
 *            -- Jacob Smith 2017-08-08
 *
 *     Integrate with HDF5.
 *     Returns herr_t instead of nothing.
 *             -- Jacob Smith 2017-09-21
 *
 *****************************************************************************/
herr_t
H5FD_s3comms_hrb_destroy(hrb_t *buf)
{
    herr_t ret_value = SUCCEED;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_hrb_destroy.\n");

    if (buf != NULL) {
        if (buf->magic != S3COMMS_HRB_MAGIC) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "pointer's magic does not match.\n");
        }

        free(buf->verb);
        free(buf->version);
        free(buf->resource);
        free(buf);
    }
/*
 *  Flatten?
 *
 *  if (buf == NULL) {
 *      goto done;
 *  }
 *  if (buf->magic != S3COMMS_HRB_MAGIC) { 
 *      HGOTO_ERROR(...); 
 *  }
 *  free(buf->verb);
 *  ...
 */

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_hrb_destroy */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_hrb_init_request()
 *
 * Purpose:
 *
 *     Create a new HTTP Request Buffer, to build an HTTP request,
 *     element by element (header-by-header, plus optional body).
 *
 *     All non-null arguments should be null-terminated strings.
 *
 *     If `verb` is NULL, defaults to "GET".
 *     If `http_version` is NULL, defaults to "HTTP/1.1".
 *
 *     `resource` cannot be NULL; should be string beginning with slash
 *     character ('/').
 *
 *     All strings are copied into the structure, making them safe from 
 *     modification in source strings.    
 *
 * Return:
 *
 *     SUCCESS: pointer to new `hrb_t`
 *     FAILURE: null pointer
 *
 * Programmer: Jacob Smith
 *             2017-07-21
 *
 * Changes:
 *
 *     Update struct membership for newer 'generic' `hrb_t` format.
 *             -- Jacob Smith, 2017-07-24
 *
 *     Rename from `hrb_new()` to `hrb_request()`
 *             -- Jacob Smith, 2017-07-25
 *
 *     Integrate with HDF5.
 *     Rename from 'hrb_request()` to `H5FD_s3comms_hrb_init_request()`.
 *     Remove `host` from input paramters.
 *         Host, as with all other fields, must now be added through the
 *         add-field functions.
 *     Add `version` (HTTP version string, e.g. "HTTP/1.1") to parameters.
 *             -- Jacob Smith 2017-09-20
 *
 *     Update to use linked-list `hrb_fl_t_2` headers in structure.
 *             -- Jacob Smith 2017-10-05
 *
 ****************************************************************************/
hrb_t *
H5FD_s3comms_hrb_init_request(const char *_verb,
                              const char *_resource,
                              const char *_http_version)
{
    hrb_t  *request   = NULL;
    char   *res       = NULL;
    size_t  reslen    = 0;
    hrb_t  *ret_value = NULL;
    char   *verb      = NULL;
    size_t  verblen   = 0;
    char   *vrsn      = NULL;
    size_t  vrsnlen   = 0;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_hrb_init_request.\n");

    if (_resource == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "resource string cannot be null.\n");
    }

    /* populate valid NULLs with defaults
     */
    if (_verb == NULL) {
        _verb = "GET";
    }
    if (_http_version == NULL) {
        _http_version = "HTTP/1.1";
    }

    /* malloc space for and prepare structure
     */
    request = (hrb_t *)malloc(sizeof(hrb_t));
    if (request == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "no space for request structure");
    }

    request->magic        = S3COMMS_HRB_MAGIC;
    request->body         = NULL;
    request->body_len     = 0;
    request->first_header = NULL;



    /* malloc and copy strings for the structure
     */
    if (_resource[0] == '/') {
        reslen = strlen(_resource) + 1;
        res = (char *)malloc(sizeof(char) * reslen);
        if (res == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, NULL,
                        "no space for resource string");
        }
        strncpy(res, _resource, reslen);
    } else {
        reslen = strlen(_resource) + 2;
        res = (char *)malloc(sizeof(char) * reslen);
        if (res == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, NULL,
                        "no space for resource string");
        }
        sprintf(res, "/%s", _resource);
    } /* start resource string with '/' */

    verblen = strlen(_verb) + 1;
    verb = (char *)malloc(sizeof(char) * verblen);
    if (verb == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "no space for verb string");
    }
    strncpy(verb, _verb, verblen);

    vrsnlen = strlen(_http_version) + 1;
    vrsn = (char *)malloc(sizeof(char) * vrsnlen);
    if (vrsn == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "no space for http-version string");
    }
    strncpy(vrsn, _http_version, vrsnlen);



    /* place new copies into structure
     */
    request->resource = res;
    request->verb     = verb;
    request->version  = vrsn;

    ret_value = request;

done:

    /* if there is an error, clean up after ourselves
     */
    if (ret_value == NULL) {
        if (request != NULL) {
            free(request);
        }
        if (vrsn) { free(vrsn); }
        if (verb) { free(verb); }
        if (res)  { free(res);  }
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_hrb_init_request */



/****************************************************************************
 * S3R FUNCTIONS 
 ****************************************************************************/


/****************************************************************************
 *
 * Function: H5FD_s3comms_s3r_close()
 *
 * Purpose:
 *
 *    Close communications through given S3 Request Handle (`s3r_t`)
 *    and clean up associated resources.
 *
 * Return: Nothing.
 *
 * Programmer: Jacob Smith
 *             2017-08-31
 *
 * Changes:
 *
 *    Remove all messiness related to the now-gone "setopt" utility
 *        as it no longer exists in the handle.
 *    Return type to `void`.
 *            -- Jacob Smith 2017-09-01
 *
 *    Incorporate into HDF environment.
 *    Rename from `s3r_close()` to `H5FD_s3comms_s3r_close()`.
 *            -- Jacob Smith 2017-10-06
 *
 *    Change separate host, resource, port info to `parsed_url_t` struct ptr.
 *            -- Jacob Smith 2017-11-01
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_s3r_close(s3r_t *handle)
{
    herr_t ret_value = SUCCEED;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_s3r_close.\n");

    if (handle == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle cannot be null.\n");
    }
    if (handle->magic != S3COMMS_S3R_MAGIC) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has invalid magic.\n");
    }

    curl_easy_cleanup(handle->curlhandle);
    free(handle->secret_id);
    free(handle->region);
    free(handle->signing_key);
    HDassert(SUCCEED == H5FD_s3comms_free_purl(handle->purl));
    free(handle);

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_s3r_close */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_s3r_getsize()
 *
 * Purpose:
 *
 *    Get the number of bytes of handle's target resource.
 *
 *    Sets handle and curlhandle with to enact an HTTP HEAD request on file,
 *    and parses received headers to extract "Content-Length" from response
 *    headers, storing file size at `handle->filesize`.
 *
 *    Critical step in opening (initiating) an `s3r_t` handle.
 *
 *    Wraps `s3r_read()`. Sets curlhandle to write headers to a temporary
 *    buffer (using extant write callback) and provides no buffer for body.
 *    offset:len::0:0 gets length of entire file.
 *
 *    Upon exit, unsets HTTP HEAD settings from curl handle.
 *
 * Return:
 *
 *    S3E_OK upon success, else some other S3code.
 *
 * Programmer: Jacob Smith
 *             2017-08-23
 *
 * Changes:
 *
 *    Update to revised `s3r_t` format and life cycle.
 *            -- Jacob Smith 2017-09-01
 *
 *    Conditional change to static header buffer and structure.
 *            -- Jacob Smith 2017-09-05
 *
 *    Incorporate into HDF environment.
 *    Rename from `s3r_getsize()` to `H5FD_s3comms_s3r_getsize()`.
 *            Jacob Smith 2017-10-06
 *
 *****************************************************************************/
herr_t
H5FD_s3comms_s3r_getsize(s3r_t *handle)
{
    unsigned long int      content_length = 0;
    CURL                  *curlh          = NULL;
    char                  *end            = NULL;
    char                   headerresponse[CURL_MAX_HTTP_HEADER];
    herr_t                 ret_value      = SUCCEED;
    struct s3r_datastruct  sds            = { headerresponse, 0 };
    char                  *start          = NULL;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_s3r_getsize.\n");

    if (handle == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle cannot be null.\n");
    }
    if (handle->magic != S3COMMS_S3R_MAGIC) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has invalid magic.\n");
    }
    if (handle->curlhandle == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has bad (null) curlhandle.\n")
    }



    /********************
     * PREPARE FOR HEAD *
     ********************/

    curlh = handle->curlhandle;

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_NOBODY,
                         1L) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "error while setting CURL option (CURLOPT_NOBODY). "
                    "(placeholder flags)");
    }

    /* uses WRITEFUNCTION, as supplied in s3r_open */

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_HEADERDATA,
                         &sds) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "error while setting CURL option (CURLOPT_HEADERDATA). "
                    "(placeholder flags)");
    }

    handle->httpverb="HEAD"; /* compile warning re: `const` discard */



    /*******************
     * PERFORM REQUEST *
     *******************/

    /* these parameters fetch the entire file,
     * but, with a NULL destination and NOBODY and HEADERDATA supplied above,
     * only http metadata will be sent by server and recorded by s3comms
     */
    if (FAIL ==
        H5FD_s3comms_s3r_read(handle, 0, 0, NULL) )
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "problem in reading during getsize.\n");
    }



    /******************
     * PARSE RESPONSE *
     ******************/

    start = strstr(headerresponse,
                   "\r\nContent-Length: ");
    if (start == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "could not find \"Content-Length\" in response.\n");
    }

    /* move "start" to beginning of value in line; find end of line
     */
    start = start + strlen("\r\nContent-Length: ");
    end = strstr(start,
                 "\r\n");
    if (end == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "could not find end of content length line");
    }

    /* place null terminator at end of numbers 
     */
    end = '\0'; 

    content_length = strtoul((const char *)start,
                             NULL,
                             0);
    if (content_length == 0         || 
        content_length == ULONG_MAX || 
        errno          == ERANGE) /* errno set by strtoul */
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
            "could not convert found \"Content-Length\" response (\"%s\")",
            start); /* range is null-terminated, remember */
    }

    handle->filesize = (size_t)content_length;



    /**********************
     * UNDO HEAD SETTINGS *
     **********************/
    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_NOBODY,
                         0) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "error while setting CURL option (CURLOPT_NOBODY). "
                    "(placeholder flags)");
    }

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_HEADERDATA,
                         0) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "error while setting CURL option (CURLOPT_HEADERDATA). "
                    "(placeholder flags)");
    }


done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_s3r_getsize */


/****************************************************************************
 *
 * Function: H5FD_s3comms_s3r_open()
 *
 * Purpose:
 *
 *    Logically 'open' a file hosted on S3.
 *
 *    Create new Request Handle,
 *    copy supplied url,
 *    copy authentication info if supplied,
 *    create CURL handle,
 *    fetch size of file,
 *        connect with server and execute HEAD request
 *    return request handle ready for reads.
 *
 *    To use 'default' port to connect, `port` should be 0.
 *
 *    To prevent AWS4 authentication, pass null pointer to `region`, `id`,
 *    and ,`signing_key`.
 *
 *    Uses `H5FD_s3comms_parse_url()` to validate and parse url input.
 *
 * Return:
 *
 *    Pointer to new request handle.
 *    NULL if authentication strings are not all NULL or all populated,
 *         if url is NULL,
 *         if error while making copies,
 *         if error while performing `getsize()`
 *
 * Programmer: Jacob Smith
 *             2017-09-01
 *
 * Changes: 
 *
 *     Incorporate into HDF environment.
 *     Rename from `s3r_open()` to `H5FD_s3comms_s3r_open()`.
 *             -- Jacob Smith 2017-10-06
 *
 *     Remove port number from signautre.
 *     Name (`url`) must be complete url with http scheme and optional port
 *         number in string.
 *         e.g., "http://bucket.aws.com:9000/myfile.dat?query=param"
 *     Internal storage of host, resource, and port information moved into
 *     `parsed_url_t` struct pointer.
 *             -- Jacob Smith 2017-11-01
 *
 ****************************************************************************/
s3r_t *
H5FD_s3comms_s3r_open(const char          url[],
                      const char          region[],
                      const char          id[],
                      const unsigned char signing_key[])
{
    size_t        tmplen      = 0;
    CURL         *curlh       = NULL;
    s3r_t        *h           = NULL;
    parsed_url_t *purl        = NULL;
    s3r_t        *ret_value   = NULL;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_s3r_open.\n");



    if (url == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "url cannot be null.\n");
    }

    if (FAIL == H5FD_s3comms_parse_url(url, &purl)) {
        /* probably a malformed url, but could be internal error */
        HGOTO_ERROR(H5E_ARGS, H5E_CANTCREATE, NULL,
                    "unable to create parsed url structure");
    }
    HDassert(purl != NULL); /* if above passes, this must be true */

    h = (s3r_t *)malloc(sizeof(s3r_t)); /* "h" for handle */
    if (h == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "could not malloc space for handle.\n");
    }

    h->magic        = S3COMMS_S3R_MAGIC;
    h->purl         = purl;
    h->filesize     = 0;
    h->region       = NULL;
    h->secret_id    = NULL;
    h->signing_key  = NULL;



#if 0
    /* copy persistent data into structure
     *     optimizaion note: this `length, malloc, check, memcpy` pattern
     *                       appears repeatedly, here and for authetentication
     *                       info
     */
    tmplen = strlen(url) + 1;
    h->url = (char *)malloc(sizeof(char) * tmplen);
    if (h->url == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "could not malloc space for handle url copy.\n");
    }
    memcpy(h->url, url, tmplen);
#endif



    /*************************************
     * RECORD AUTHENTICATION INFORMATION *
     *************************************/

    if (region != NULL || id != NULL || signing_key != NULL)
    {
        /* if one exists, all three must exist
         */
        if (region == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "region cannot be null.\n");
        }
        if (id == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "secret id cannot be null.\n");
        }
        if (signing_key == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "signing key cannot be null.\n");
        }

        tmplen = strlen(region) + 1;
        h->region = (char *)malloc(sizeof(char) * tmplen);
        if (h->region == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "could not malloc space for handle region copy.\n");
        }
        memcpy(h->region, region, tmplen);

        tmplen = strlen(id) + 1;
        h->secret_id = (char *)malloc(sizeof(char) * tmplen);
        if (h->secret_id == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "could not malloc space for handle ID copy.\n");
        }
        memcpy(h->secret_id, id, tmplen);

        tmplen = SHA256_DIGEST_LENGTH;
        h->signing_key = (unsigned char *)malloc(sizeof(unsigned char) * \
                                                 tmplen);
        if (h->signing_key == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                        "could not malloc space for handle key copy.\n");
        }
        memcpy(h->signing_key, signing_key, tmplen);
    } /* if authentication information provided */




    /************************
     * INITIATE CURL HANDLE *
     ************************/

    curlh = curl_easy_init();

    if (curlh == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "problem creating curl easy handle!\n");
    }


    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_HTTPGET,
                         1L) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "error while setting CURL option (CURLOPT_HTTPGET). "
                    "(placeholder flags)");
    }

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_HTTP_VERSION,
                         CURL_HTTP_VERSION_1_1) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "error while setting CURL option (CURLOPT_HTTP_VERSION). "
                    "(placeholder flags)");
    }


    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_FAILONERROR,
                         1L) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "error while setting CURL option (CURLOPT_FAILONERROR). "
                    "(placeholder flags)");
    }

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_WRITEFUNCTION,
                         curlwritecallback) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "error while setting CURL option (CURLOPT_WRITEFUNCTION). "
                    "(placeholder flags)");
    }

    if ( CURLE_OK != 
        curl_easy_setopt(curlh,
                         CURLOPT_URL,
                         url) )
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                    "error while setting CURL option (CURLOPT_URL). "
                    "(placeholder flags)");
    }

    /*****************
     * for debugging *
     *****************/
#ifdef VERBOSE_CURL
    curl_easy_setopt(curlh, CURLOPT_VERBOSE, 1L);
#endif 

    h->curlhandle = curlh;



    /*******************
     * OPEN CONNECTION *
     * GET FILE SIZE   *
     *******************/

    if (FAIL == 
        H5FD_s3comms_s3r_getsize(h) ) 
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                     "problem in H5FD_s3comms_s3r_getsize.\n");
    }



    /*********************
     * FINAL PREPARATION *
     *********************/

    h->httpverb = "GET"; /* compiler warning re: discarding `const` */

    /* FROM CURL DOCUMENTATION:
     * If the file requested is found larger than this value, the transfer
     * will not start and CURLE_FILESIZE_EXCEEDED will be returned.
     *
     * The file size is not always known prior to download, and for such
     * files this option has no effect even if the file transfer ends up
     * being larger than this given limit.
     *
     * If you want a limit above 2GB, use CURLOPT_MAXFILESIZE_LARGE.
     */
/*
 *   curl_easy_setopt(curlh,
 *                    CURLOPT_MAXFILESIZE,
 *                    h->filesize);
 */

    ret_value = h;

done:
    if (ret_value == NULL) {
        if (curlh != NULL) {
            curl_easy_cleanup(curlh);
        }
        HDassert(SUCCEED == H5FD_s3comms_free_purl(purl));
        if (h != NULL) {
            free(h->region);
            free(h->secret_id);
            free(h->signing_key);

            free(h);
        }
    }

    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_s3r_open */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_s3r_read()
 *
 * Purpose:
 *
 *    Read file pointed to by request handle, writing specified
 *    `offset` .. `offset + len` bytes to buffer `dest`.
 *
 *    If `len` is 0, reads entirety of file starting at `offset`.
 *    If `offset` and `len` are both 0, reads entire file.
 *
 *    High-level routine to execute request as defined in provided handle.
 *
 *    Uses configured "curl easy handle" to perform request.
 *
 *    In event of error, buffer should remain unaltered.
 *
 *    If handle is set to authorize a request, creates a new (temporary)
 *    HTTP Request object (hrb_t) for generating requisite headers,
 *    which is then translated to a `curl slist` and set in the curl handle
 *    for the request.
 *
 *    `dest` may be NULL, but no body data will be recorded.
 *
 *    * above feature used for `s3r_getsize()`, in conjunction with
 *    *     CURLOPT_NOBODY to preempty transmission of file body data.
 *
 * Return:
 *
 *     FAIL    on error
 *     
 *     SUCCESS
 *
 * Programmer: Jacob Smith
 *             2017-08-22
 *
 * Changes:
 *
 *     Revise structure to prevent unnecessary hrb_t element creation.
 *     Rename tmprstr -> rangebytesstr to reflect purpose.
 *     Insert needed `free()`s, particularly for `sds`.
 *             -- Jacob Smith 2017-08-23
 *
 *     Revise heavily to accept buffer, range as parameters.
 *     Utilize modified s3r_t format.
 *             -- Jacob Smith 2017-08-31
 *
 *     Incorporate into HDF library.
 *     Rename from `s3r_read()` to `H5FD_s3comms_s3r_read()`.
 *     Return `herr_t` succeed/fail instead of S3code.
 *     Update to use revised `hrb_t` and `hrb_fl_t` structures.
 *             -- Jacob Smith 2017-10-06
 *
 *     Update to use `parsed_url_t *purl` in handle.
 *             --Jacob Smith 2017-11-01
 *
 *****************************************************************************/
herr_t
H5FD_s3comms_s3r_read(s3r_t *handle,
                      size_t  offset,
                      size_t  len,
                      void   *dest)
{
    char authorization[512];
        /* 512 :=
         *   67 <len("AWS4-HMAC-SHA256 Credential=///s3/aws4_request,SignedHeaders=,Signature=")>
         * + 8 <yyyyMMDD>
         * + 64 <hex(sha256())>
         * + len(secret_id) <128 max?>
         * + len(region) <20 max?>
         * + len(signed_headers) <128 reasonalbe max?>
         */
    char buffer1[512]; /* -> Canonical Request -> Signature */
    char buffer2[256]; /* -> String To Sign -> Credential */
    char iso8601now[ISO8601_SIZE];
    char signed_headers[48]; /* should be large enough for nominal listing:  
                              * "host;range;x-amz-content-sha256;x-amz-date" 
                              * + '\0', with "range;" possibly absent         
                              */
    CURL                  *curlh         = NULL;
    CURLcode               p_status      = CURLE_OK;
    struct curl_slist     *curlheaders   = NULL;
    hrb_fl_t_2            *headers       = NULL;
    char                  *hstr          = NULL; 
    hrb_fl_t_2            *node          = NULL;
    struct tm             *now           = NULL;
    char                  *rangebytesstr = NULL;
    hrb_t                 *request       = NULL;
    herr_t                 ret_value     = SUCCEED;
    struct s3r_datastruct *sds           = NULL;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_s3r_read.\n");

    authorization[0]  = 0;
    buffer1[0]        = 0;
    buffer2[0]        = 0;
    signed_headers[0] = 0;


    /*************************************
     * ABSLUTELY NECESSARY SANITY-CHECKS *
     *************************************/

    if (handle == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle cannot be null.\n");
    }
    if (handle->magic != S3COMMS_S3R_MAGIC) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has invalid magic.\n");
    }
    if (handle->curlhandle == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has bad (null) curlhandle.\n")
    }
    if (handle->purl == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "handle has bad (null) url.\n")
    }

    curlh = handle->curlhandle;



    /*********************
     * PREPARE WRITEDATA *
     *********************/

    if (dest != NULL) {
        sds = (struct s3r_datastruct *)malloc(sizeof(struct s3r_datastruct));
        if (sds == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                        "could not malloc destination datastructure.\n");
        }

        sds->data = (char *)dest;
        sds->size = 0;
        if (CURLE_OK !=
            curl_easy_setopt(curlh,
                             CURLOPT_WRITEDATA,
                             sds) )
        {
            HGOTO_ERROR(H5E_ARGS, H5E_UNINITIALIZED, FAIL,
                        "error while setting CURL option (CURLOPT_WRITEDATA). "
                        "(placeholder flags)");
        }
    }



    /*********************
     * FORMAT HTTP RANGE *
     *********************/

    if (len > 0) {
        rangebytesstr = (char *)malloc(sizeof(char) * 128);
        if (rangebytesstr == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                        "could not malloc range format string.\n");
        }
        sprintf(rangebytesstr,
                "bytes=%lu-%lu",
                offset,
                offset + len);
    } else if (offset > 0) {
        rangebytesstr = (char *)malloc(sizeof(char) * 128);
        if (rangebytesstr == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                        "could not malloc range format string.\n");
        }
        sprintf(rangebytesstr,
                "bytes=%lu-",
                offset);
    }



    /*******************
     * COMPILE REQUEST *
     *******************/

    if (handle->signing_key == NULL) {
        /* Do not authenticate.
         */
        if (rangebytesstr != NULL) {
            /* Pass in range directly
             */
            char *bytesrange_ptr = NULL; /* pointer past "bytes=" portion */
            bytesrange_ptr = strchr(rangebytesstr, '=');
            HDassert(bytesrange_ptr != NULL);
            bytesrange_ptr++; /* move to first char past '=' */
            HDassert(*bytesrange_ptr != '\0');

            if (CURLE_OK !=
                curl_easy_setopt(curlh,
                                 CURLOPT_RANGE,
                                 bytesrange_ptr) )
            {
                HGOTO_ERROR(H5E_VFL, H5E_UNINITIALIZED, FAIL,
                        "error while setting CURL option (CURLOPT_RANGE). ");
            } /* curl setopt failed */
        } /* if rangebytesstr is defined */
    } else {

        /**** VERIFY INFORMATION EXISTS ****/

        if (handle->region == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null region.\n");
        }
        if (handle->secret_id == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null secret_id.\n");
        }
        if (handle->signing_key == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null signing_key.\n");
        }
        if (handle->httpverb == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null httpverb.\n");
        }
        if (handle->purl->host == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null host.\n");
        }
        if (handle->purl->path == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "handle must have non-null resource.\n");
        }



        /**** CREATE HTTP REQUEST STRUCTURE (hrb_t) ****/

/* As a possible optimization, this hrb_t can be moved into s3r_t handle
 * in place of authorize flag.
 * Headers used are consistent, data in HTTP request changes little
 * Create request structure in open()?
 */

       request = H5FD_s3comms_hrb_init_request((const char *)handle->httpverb,
                                               (const char *)handle->purl->path,
                                               "HTTP/1.1");
        if (request == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "could not allocate hrb_t request.\n");
        }

        now = gmnow();
        if ( ISO8601NOW(iso8601now, now) != (ISO8601_SIZE - 1)) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "could not format ISO8601 time.\n");
        }

        // Host is persistent; can be set with open()
        headers = H5FD_s3comms_hrb_fl_set(headers,
                                          "Host", 
                                          (const char *)handle->purl->host);
        headers = H5FD_s3comms_hrb_fl_set(headers, 
                                          "Range", 
                                          (const char *)rangebytesstr);
        headers = H5FD_s3comms_hrb_fl_set(headers, 
                                          "x-amz-content-sha256", 
                                          (const char *)EMPTY_SHA256);
        headers = H5FD_s3comms_hrb_fl_set(headers, 
                                          "x-amz-date", 
                                          (const char *)iso8601now);

        if (headers == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "problem building headers list. "
                        "(placeholder flags)\n");
        }
#if 0
    /* explicit fetching of "first header"
     * Pros: faster, lower-overhead
     * 
     * Cons: more difficult to understand, ignores provided interface
     */
        node = headers;
        while (node->prev_lower != NULL) { node = node->prev_lower; }
        request->first_header = node;
#else
        request->first_header = H5FD_s3comms_hrb_fl_first(headers, 
                                                          HRB_FL_ORD_SORTED);
#endif
        /**** COMPUTE AUTHORIZATION ****/

        if (FAIL ==      /* buffer1 -> canonical request */
            H5FD_s3comms_aws_canonical_request(buffer1,
                    signed_headers,
                    request) )
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "not the right flags.\n");
        }
        if ( FAIL ==     /* buffer2->string-to-sign */
             H5FD_s3comms_tostringtosign(buffer2,
                                         buffer1,
                                         iso8601now,
                                         handle->region) )
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "not the right flags.\n");
        }
        if (FAIL ==     /* buffer1 -> signature */
            H5FD_s3comms_HMAC_SHA256(handle->signing_key,
                                     SHA256_DIGEST_LENGTH,
                                     buffer2,
                                     strlen(buffer2),
                                     buffer1) )
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "not the right flags.\n");
        }

        iso8601now[8] = 0; /* trim to yyyyMMDD */
        H5FD_S3COMMS_FORMAT_CREDENTIAL(buffer2, 
                                       handle->secret_id, 
                                       iso8601now, 
                                       handle->region, 
                                       "s3");

        sprintf(authorization,
                "AWS4-HMAC-SHA256 Credential=%s,SignedHeaders=%s,Signature=%s",
                buffer2,
                signed_headers,
                buffer1);

        /* append authorization header to http request buffer
         */
        headers = H5FD_s3comms_hrb_fl_set(headers, 
                                          "Authorization", 
                                          (const char *)authorization);

        /* update hrb's "first header" pointer
         */
        if (headers == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "problem building headers list. "
                        "(placeholder flags)\n");
        }
#if 0
/* again, lower-level operation option
 */
        node = headers;
        while (node->prev_lower != NULL) { node = node->prev_lower; }
        request->first_header = node;

#else
        request->first_header = H5FD_s3comms_hrb_fl_first(headers, 
                                                          HRB_FL_ORD_SORTED);
#endif

        /**** SET CURLHANDLE HTTP HEADERS FROM GENERATED DATA ****/

        node = request->first_header;
        while (node != NULL) {
            hstr = (char *)malloc(sizeof(char) *
                                  (strlen(node->name) + 
                                   strlen(node->value) + 
                                   3));
            if (hstr == NULL) {
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "could not malloc temporary header string.\n");
            }
 
            sprintf(hstr, "%s: %s", node->name, node->value);
            curlheaders = curl_slist_append(curlheaders, 
                                            (const char *)hstr);
            if (curlheaders == NULL) {
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "could not append header to curl slist. "
                            "(placeholder flags)\n");
            }

            free(hstr);
            hstr = NULL;
            node = node->next_lower;
        }




        /* sanity-check
         */
        if (curlheaders == NULL) {
            /* above loop was probably never run */
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "curlheaders was never populated.\n");
        }

        /* finally, set http headers in curl handle
         */
        if (CURLE_OK !=
            curl_easy_setopt(curlh,
                             CURLOPT_HTTPHEADER,
                             curlheaders) )
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "error while setting CURL option "
                        "(CURLOPT_HTTPHEADER). (placeholder flags)");
        }

    } /* if should authenticate (info provided) */




    /*******************
     * PERFORM REQUEST *
     *******************/
#ifdef VERBOSE_CURL
    {
        long int httpcode = 0;
        char     curlerrbuf[CURL_ERROR_SIZE];
        curlerrbuf[0] = '\0';

        HDassert(CURLE_OK == 
                 curl_easy_setopt(curlh, CURLOPT_ERRORBUFFER, curlerrbuf));

        p_status = curl_easy_perform(curlh);

        if (p_status != CURLE_OK) {
            HDassert(CURLE_OK ==
                 curl_easy_getinfo(curlh, CURLINFO_RESPONSE_CODE, &httpcode));
            HDprintf("CURL ERROR CODE: %d\nHTTP CODE: %d\n", 
                     p_status, httpcode);
            HDprintf("%s\n", curl_easy_strerror(p_status));
            HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL,
                    "problem while performing request.\n");
        }
        HDassert(CURLE_OK == 
                 curl_easy_setopt(curlh, CURLOPT_ERRORBUFFER, NULL));
    } /* VERBOSE block, for curlerrbuf buffer */
#else
    p_status = curl_easy_perform(curlh);

    if (p_status != CURLE_OK) {
        HGOTO_ERROR(H5E_VFL, H5E_CANTOPENFILE, FAIL,
                    "problem while performing request.\n");
    }
#endif

/* preserved for archival purposes
 *
 *  handle->curlcode = curl_easy_perform(curlh);
 *  handle->data_len = sds->size;
 *  curl_easy_getinfo(curlh,
 *                    CURLINFO_RESPONSE_CODE,
 *                    &handle->httpcode);
 */



done:
    /* clean any malloc'd resources
     */
    if (curlheaders   != NULL) curl_slist_free_all(curlheaders);
    if (hstr          != NULL) free(hstr);
    if (rangebytesstr != NULL) free(rangebytesstr);
    if (sds           != NULL) free(sds);
    if (request       != NULL) {
        H5FD_s3comms_hrb_fl_destroy(headers); /* check SUCCEED? */
        H5FD_s3comms_hrb_destroy(request); /* check SUCCEED? */
    }


    if (curlh != NULL) {
        curl_easy_setopt(curlh, CURLOPT_RANGE, NULL); /* clear any Range */
        curl_easy_setopt(curlh, CURLOPT_HTTPHEADER, NULL); /* clear headers */
    }

    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_s3r_read */



/****************************************************************************
 * MISCELLANEOUS FUNCTIONS
 ****************************************************************************/


/*****************************************************************************
 *
 * Function: gmnow()
 *
 * Purpose:
 *
 *    Get the output of `time.h`'s `gmtime()` call while minimizing setup 
 *    clutter where important.
 *
 * Return:
 *
 *    Pointer to resulting `struct tm`,as created by gmtime(time_t * T).
 *
 * Programmer: Jacob Smith
 *             2017-07-12
 *
 * Changes: None.
 *
 *****************************************************************************/
struct tm *
gmnow(void)
{
    time_t  now;
    time_t *now_ptr = &now;

    time(now_ptr);
    return gmtime(now_ptr);

} /* gmnow */


/****************************************************************************
 *
 * Function: H5FD_s3comms_aws_canonical_request()
 *
 * Purpose:
 *
 *     Compose AWS "Canonical Request" (and signed headers string)
 *     as defined in the REST API documentation.
 *
 *     Both destination strings are null-terminated.
 *      
 *     Destination string arguments must be provided with adequate space.
 *
 *     Canonical Request format:
 *
 *      <HTTP VERB>"\n"
 *      <resource path>"\n"
 *      <query string>"\n"
 *      <header1>"\n" (`lowercase(name)`":"`trim(value)`)
 *      <header2>"\n"
 *      ... (headers sorted by name)
 *      <header_n>"\n"
 *      "\n"
 *      <signed headers>"\n" (`lowercase(header 1 name)`";"`header 2 name`;...)
 *      <hex-string of sha256sum of body> ("e3b0c4429...", e.g.)
 *
 * Return:
 *
 *     SUCCESS If arguments are valid; operation "successful"
 *             writes canonical request to respective `...dest` strings.
 *     FAIL    If any input argument is NULL.
 *
 * Programmer: Jacob Smith
 *             2017-10-04
 *
 * Changes: None.
 *
 ****************************************************************************/
herr_t 
H5FD_s3comms_aws_canonical_request(char  *canonical_request_dest,
                                   char  *signed_headers_dest,
                                   hrb_t *http_request)
{
    hrb_fl_t_2 *node         = NULL;
    const char *query_params = ""; /* unused at present */
    herr_t      ret_value    = SUCCEED;
    char        tmpstr[256];

    /* "query params" refers to the optional element in the URL, e.g.
     *     http://bucket.aws.com/myfile.txt?max-keys=2&prefix=J
     *                                      ^-----------------^
     *
     * Not handled/implemented as of 2017-10-xx.
     * Element introduced as empty placeholder and reminder.
     * Further research to be done if this is ever releveant for the
     * VFD use-cases.
     */



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_aws_canonical_request.\n");

    if (http_request == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "hrb object cannot be null.\n");
    }
    HDassert(http_request->magic == S3COMMS_HRB_MAGIC);

    if (canonical_request_dest == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "canonical request destination cannot be null.\n");
    }

    if (signed_headers_dest == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "signed headers destination cannot be null.\n");
    }



    /* HTTP verb, resource path, and query string lines
     */
    sprintf(canonical_request_dest,
            "%s\n%s\n%s\n",
            http_request->verb,
            http_request->resource,
            query_params);

    /* write in canonical headers, building signed headers concurrently
     */
    node = http_request->first_header; /* assumed at first sorted */
    while (node != NULL) {
        sprintf(tmpstr, "%s:%s\n", node->lowername, node->value);
        strcat(canonical_request_dest, tmpstr);

        sprintf(tmpstr, "%s;", node->lowername);
        strcat(signed_headers_dest, tmpstr);

        node = node->next_lower;
    }

    /* remove tailing ';' from signed headers sequence 
     */
    signed_headers_dest[strlen(signed_headers_dest) - 1] = '\0';



    /* append signed headers and payload hash
     * NOTE: at present, no HTTP body is handled, per the nature of
     *       requests/range-gets
     */ 
    strcat(canonical_request_dest, "\n");
    strcat(canonical_request_dest, signed_headers_dest);
    strcat(canonical_request_dest, "\n");
    strcat(canonical_request_dest, EMPTY_SHA256);

done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_aws_canonical_request */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_bytes_to_hex()
 *
 * Purpose:
 *
 *    Produce human-readable hex string [0-9A-F] from sequence of bytes.
 *
 *    For each byte (char), writes two-character hexadecimal representation.
 *
 *    No null-terminator appended.
 *
 *    Assumes `dest` is allocated to enough size (msg_len * 2).
 *
 *    Fails if either `dest` or `msg` are null.
 *
 *    `msg_len` message length of 0 has no effect.
 *
 * Return:
 *
 *     SUCCEED
 *     FAIL if `dest` == NULL or `msg` == NULL
 *
 * Programmer: Jacob Smith
 *             2017-07-12
 *
 * Changes: 
 *
 *     Integrate into HDF.
 *     Rename from hex() to H5FD_s3comms_bytes_to_hex.
 *     Change return type from `void` to `herr_t`.
 *             -- Jacob Smtih 2017-09-14
 *
 *     Add bool parameter `lowercase` to configure upper/lowercase output
 *         of a-f hex characters.
 *             -- Jacob Smith 2017-09-19
 *
 *     Change bool type to `hbool_t`
 *             -- Jacob Smtih 2017-10-11
 *
 *****************************************************************************/
herr_t 
H5FD_s3comms_bytes_to_hex(char                *dest,
                          const unsigned char *msg,
                          size_t               msg_len,
                          hbool_t              lowercase)
{
    size_t i         = 0;
    herr_t ret_value = SUCCEED;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_bytes_to_hex.\n");

    if (dest == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "hex destination cannot be null.\n")

    if (msg == NULL)
       HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                   "bytes sequence cannot be null.\n")



    for (i = 0; i < msg_len; i++) {
        sprintf(&(dest[i * 2]), 
                (lowercase == TRUE) ? "%02x"
                                    : "%02X",
                msg[i]);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_bytes_to_hex */


/*****************************************************************************
 *
 * Function: H5FD_s3comms_free_purl()
 *
 * Purpose: 
 *
 *     Release resources from a (maybe not-NULL) parsed_url_t pointer.
 *
 * Return: 
 *
 *     SUCCEED (never fails)
 *
 * Programmer: Jacob Smith
 *             2017-11-01
 *
 * Changes: None.
 *
 *****************************************************************************/
herr_t
H5FD_s3comms_free_purl(parsed_url_t *purl) 
{
    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDprintf("called H5FD_s3comms_free_purl.\n");

    if (purl != NULL) {
        if (purl->scheme != NULL) free(purl->scheme); 
        if (purl->host   != NULL) free(purl->host);
        if (purl->port   != NULL) free(purl->port);
        if (purl->path   != NULL) free(purl->path);
        if (purl->query  != NULL) free(purl->query);
        free(purl);
    }

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5FD_s3comms_free_purl */


/****************************************************************************
 *
 * Function: H5FD_s3comms_HMAC_SHA256()
 *
 * Purpose:
 *
 *    Generate Hash-based Message Authentication Checksum using the SHA-256 
 *    hashing algorithm.
 *
 *    Given a key, message, and respective lengths (to accommodate null 
 *    characters in either), generate _hex string_ of authentication checksum 
 *    and write to `dest`.
 *
 *    `dest` must be at least `SHA256_DIGEST_LENGTH * 2` characters in size.
 *    Not enforceable by this function.
 *    `dest` will _not_ be null-terminated by this function.
 *
 * Return: 
 *
 *     FAIL    if dest is NULL or problem while generating hex string output.
 *
 *     SUCCEED
 *
 * Programmer: Jacob Smith
 *             2017-07-??
 *
 * Changes: 
 *
 *     Integrate with HDF5.
 *     Rename from `HMAC_SHA256` to `H5FD_s3comms_HMAC_SHA256`.
 *     Rename output paremeter from `md` to `dest`.
 *     Return `herr_t` type instead of `void`.
 *     Call `H5FD_s3comms_bytes_to_hex` to generate hex cleartext for output.
 *             -- Jacob Smith 2017-09-19
 *
 *     Use static char array instead of malloc'ing `md`
 *             -- Jacob Smith 2017-10-10
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_HMAC_SHA256(const unsigned char *key,
                         size_t               key_len,
                         const char          *msg,
                         size_t               msg_len,
                         char                *dest)
{
    unsigned char md[SHA256_DIGEST_LENGTH];
    unsigned int  md_len    = SHA256_DIGEST_LENGTH;
    herr_t        ret_value = SUCCEED;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_HMAC_SHA256.\n");

    if (dest == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "destination cannot be null.");
    }



    HMAC(EVP_sha256(),
         key,
         (int)key_len,
         (const unsigned char *)msg,
         msg_len,
         md,
         &md_len); 

    if (FAIL ==
        H5FD_s3comms_bytes_to_hex(dest, 
                                  (const unsigned char *)md, 
                                  (size_t)md_len,
                                  true))
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "could not convert to hex string.");
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_HMAC_SHA256 */


/****************************************************************************
 *
 * Function: H5FD_s3comms_nlowercase()
 *
 * Purpose:
 *
 *     From string starting at `s`, write `len` characters to `dest`,
 *     converting all to lowercase.
 *
 *     Behavior is undefined if `s` is NULL or `len` overruns the allocated
 *     space of either `s` or `dest`.
 *
 * Return:
 *
 *     FAIL    if `dest` is NULL.
 *
 *     SUCCEED
 *
 * Programmer: Jacob Smith
 *             2017-09-18
 *
 * Changes: None.
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_nlowercase(char       *dest,
                        const char *s,
                        size_t      len)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI_NOINIT 

    HDfprintf(stdout, "called H5FD_s3comms_nlowercase.\n");

    if (dest == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "destination cannot be null.\n");
    }

    if (len > 0) {
        memcpy(dest, s, len);
        do {
            len--;
            dest[len] = (char)tolower( (int)dest[len] );
        } while (len > 0);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_nlowercase */


/*****************************************************************************
 * 
 * Function: H5FD_s3comms_parse_url()
 *
 * Purpose:
 *
 *     Parse URL-like string and stuff URL componenets into 
 *     `parsed_url` structure, if possible.
 *
 *     Expects null-terminated string of format:
 *     SCHEME "://" HOST [":" PORT ] ["/" [ PATH ] ] ["?" QUERY]
 *     where SCHEME :: "[a-zA-Z/.-]+"
 *           PORT   :: "[0-9]"
 *
 *     Stores resulting structure in argument pointer `purl`, if successful,
 *     creating and populating new `parsed_url_t` structure pointer.
 *     Empty or absent elements are NULL in new purl structure.
 *
 * Return:
 *
 *     SUCCEED : `purl` pointer is populated
 *     FAIL    : unable to parse, `purl` is unaltered (probably NULL)
 *
 * Programmer: Jacob Smith
 *             2017-10-30
 *
 * Changes: None.
 *
 *****************************************************************************
 */
herr_t
H5FD_s3comms_parse_url(const char    *str,
                       parsed_url_t **_purl)
{
    parsed_url_t *purl         = NULL; /* pointer to new structure */
    const char   *tmpstr       = NULL; /* working pointer in string */
    const char   *curstr       = str;  /* "start" pointer in string */
    long int      len          = 0;    /* substring length */
    long int      urllen       = 0;    /* length of passed-in url string */
    unsigned int  i            = 0;
    herr_t        ret_value    = FAIL; 



    FUNC_ENTER_NOAPI_NOINIT;

    HDprintf("called H5FD_s3comms_parse_url.\n");

    if (str == NULL || *str == '\0') {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "invalid url string");
    }

    urllen = (long int)strlen(str);

    purl = (parsed_url_t *)malloc(sizeof(parsed_url_t));
    if (purl == NULL) { 
        HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                    "can't allocate space for parsed_url_t");
    }
    purl->scheme = NULL;
    purl->host   = NULL;
    purl->port   = NULL;
    purl->path   = NULL;
    purl->query  = NULL;



    /* read scheme
     */
    tmpstr = strchr(curstr, ':');
    if (tmpstr == NULL) { 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "invalid SCHEME consruction: probably not URL");
    }
    len = tmpstr - curstr;
    HDassert((0 <= len) && (len < urllen));

    /* check for restrictions 
     */
    for (i = 0; i < len; i++) {
        /* scheme = [a-zA-Z+-.]+ (terminated by ":") */
        if (!isalpha(curstr[i]) && 
             '+' != curstr[i] && 
             '-' != curstr[i] && 
             '.' != curstr[i])
        {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "invalid SCHEME consruction");
        }
    }
    /* copy 
     */
    purl->scheme = (char *)malloc(sizeof(char) * (size_t)(len + 1));
    if (purl->scheme == NULL) { /* cannot malloc */
        HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                    "can't allocate space for SCHEME");
    }
    (void)strncpy(purl->scheme, curstr, (size_t)len);
    purl->scheme[len] = 0;
    /* convert to lowercase 
     */
    for ( i = 0; i < len; i++ ) {
        purl->scheme[i] = (char)tolower(purl->scheme[i]);
    }

    /* Skip "://" */
    tmpstr += 3;
    curstr = tmpstr;

    /* read host
     */
    if (*curstr == '[') {
        /* IPv6 */
        while (']' != *tmpstr) {
            if (tmpstr == 0) { /* end of string reached! */
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "reached end of URL: incomplete IPv6 HOST");
            }
            tmpstr++;
        }
        tmpstr++;
    } else {
        while (0 != *tmpstr) {
            if (':' == *tmpstr || 
                '/' == *tmpstr || 
                '?' == *tmpstr) 
            {
                break;
            }
            tmpstr++;
        }
    } /* if IPv4 or IPv6 */
    len = tmpstr - curstr;
    if (len == 0) { 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "HOST substring cannot be empty");
    } else if (len > urllen) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "problem with length of HOST substring");
    }

    /* copy host 
     */
    purl->host = (char *)malloc(sizeof(char) * (size_t)(len + 1));
    if (purl->host == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                    "can't allocate space for HOST");
    }
    (void)strncpy(purl->host, curstr, (size_t)len);
    purl->host[len] = 0;



    /* read PORT 
     */
    if (':' == *tmpstr) {
        tmpstr += 1; /* advance past ':' */
        curstr = tmpstr;
        while ((0 != *tmpstr) && ('/' != *tmpstr) && ('?' != *tmpstr)) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        if (len == 0) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "PORT element cannot be empty");
        } else if (len > urllen) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "problem with length of PORT substring");
        }
        for (i = 0; i < len; i ++) {
            if (!isdigit(curstr[i])) { 
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "PORT is not a decimal string"); 
            }
        }

        /* copy port 
         */
        purl->port = (char *)malloc(sizeof(char) * (size_t)(len + 1));
        if (purl->port == NULL) { /* cannot malloc */
                HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                            "can't allocate space for PORT");
        }
        (void)strncpy(purl->port, curstr, (size_t)len);
        purl->port[len] = 0;
    } /* if PORT element */



    /* read PATH
     */
    if ('/' == *tmpstr) {
        /* advance past '/' */
        tmpstr += 1;
        curstr = tmpstr;
       
        /* seek end of PATH
         */
        while ((0 != *tmpstr) && ('?' != *tmpstr)) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        if (len > urllen) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "problem with length of PATH substring");
        }
        if (len > 0) {
            purl->path = (char *)malloc(sizeof(char) * (size_t)(len + 1));
            if (purl->path == NULL) {
                    HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                                "can't allocate space for PATH");
            } /* cannot malloc path pointer */
            (void)strncpy(purl->path, curstr, (size_t)len);
            purl->path[len] = 0;
        }
    } /* if PATH element */



    /* read QUERY
     */
    if ('?' == *tmpstr) {
        tmpstr += 1;
        curstr = tmpstr;
        while (0 != *tmpstr) {
            tmpstr++; 
        }
        len = tmpstr - curstr;
        if (len == 0) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "QUERY cannot be empty");
        } else if (len > urllen) {
            HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                        "problem with length of QUERY substring");
        }
        purl->query = (char *)malloc(sizeof(char) * (size_t)(len + 1));
        if (purl->query == NULL) {
            HGOTO_ERROR(H5E_ARGS, H5E_CANTALLOC, FAIL,
                        "can't allocate space for QUERY");
        } /* cannot malloc path pointer */
        (void)strncpy(purl->query, curstr, (size_t)len);
        purl->query[len] = 0;
    } /* if QUERY exists */



    *_purl = purl;
    ret_value =  SUCCEED;

done:
    if (ret_value == FAIL) {
        H5FD_s3comms_free_purl(purl);
    }
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_parse_url */


/****************************************************************************
 *
 * Function: H5FD_s3comms_percent_encode_char()
 *
 * Purpose:
 *
 *     "Percent-encode" utf-8 character `c`, e.g.,
 *         '$' -> "%24"
 *         '¢' -> "%C2%A2"
 *
 *    `c` cannot be null.
 *
 *    Does not (currently) accept multibyte characters...
 *    limit to (?) u+00ff, well below upper bound for two-byte utf-8 encoding
 *        (u+0080..u+07ff).
 *
 *     Writes output to `repr`.
 *     `repr` cannot be null.
 *     Assumes adequate space i `repr`...
 *         >>> char[4] or [7] for most chatacters,
 *         >>> [13] as theoretical maximum.
 *
 *     Representation `repr` is null-terminated.
 *
 *     Stores length of representation (without null termintor) at pointer
 *     `repr_len`.
 *
 * Return : SUCCEED/FAIL
 *
 * Programmer: Jacob Smith
 *
 * Changes:
 *
 *     Integrate into HDF.
 *     Rename from `hexutf8` to `H5FD_s3comms_percent_encode_char`.
 *             -- Jacob Smith 2017-09-15
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_percent_encode_char(char                *repr,
                                 const unsigned char  c,
                                 size_t              *repr_len)
{
    unsigned int        acc        = 0;
    unsigned int        i          = 0;
    unsigned int        k          = 0;
    unsigned int        stack[4]   = {0, 0, 0, 0};
    unsigned int        stack_size = 0;
    herr_t              ret_value  = SUCCEED;
#ifdef JAKESMITHDBG
    unsigned char       s[2]       = {c, 0}; 
    unsigned char       hex[3]     = {0, 0, 0};
#endif


    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_percent_encode_char.\n");

    /* ERROR_CHECKING */
    if (repr == NULL)
       HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "repr was null.\n")

#ifdef JAKESMITHDBG
    H5FD_s3comms_bytes_to_hex((char *)hex, s, 1);
    HDfprintf(stdout, "    CHAR: \'%s\'\n", s);
    HDfprintf(stdout, "    CHAR-HEX: \"%s\"\n", hex);
#endif



    if (c <= (unsigned char)0x7f) {
        /* character represented in a single "byte"
         * and single percent-code
         */
#ifdef JAKESMITHDBG
        HDfprintf(stdout, "    SINGLE-BYTE\n");
#endif
        *repr_len = 3;
        sprintf(repr, "%%%02X", c);
    } else {
        /* multi-byte, multi-percent representation
         */
#ifdef JAKESMITHDBG
        HDfprintf(stdout, "    MULTI-BYTE\n");
#endif
        stack_size = 0;
        k = (unsigned int)c;
        *repr_len = 0;
        do {
            /* push number onto stack in six-bit slices
             */
            acc = k;
            acc >>= 6; /* cull least */
            acc <<= 6; /* six bits   */
            stack[stack_size++] = k - acc; /* max six-bit number */
            k = acc >> 6;
        } while (k > 0);

        /* now have "stack" of two to four six-bit numbers
         * to be put into UTF-8 byte fields
         */

#ifdef JAKESMITHDBG
        HDfprintf(stdout, "    STACK:\n    {\n");
        for (i = 0; i < stack_size; i++) {
            H5FD_s3comms_bytes_to_hex((char *)hex, 
                                      (unsigned char *)(&stack[i]),
                                      1);
            hex[2] = 0;
            HDfprintf(stdout, "      %s,\n", hex);
        }
        HDfprintf(stdout, "    }\n");2
#endif

        /****************
         * leading byte *
         ****************/
        /* prepend 11[1[1]]0 to first byte */
        /* 110xxxxx, 1110xxxx, or 11110xxx */
        acc = 0xC0; /* 2^7 + 2^6 -> 11000000 */
        acc += (stack_size > 2) ? 0x20 : 0; 
        acc += (stack_size > 3) ? 0x10 : 0;
        stack_size -= 1;
        sprintf(repr, "%%%02X", acc + stack[stack_size]);
        *repr_len += 3;

        /************************
         * continuation byte(s) *
         ************************/
        /* 10xxxxxx */
        for (i = 0; i < stack_size; i++) {
            sprintf(&repr[i*3 + 3], "%%%02X", 128 + stack[stack_size - 1 - i]);
            *repr_len += 3;
        }
    }
    *(repr + *repr_len) = '\0';

done:
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_percent_encode_char */


/****************************************************************************
 *
 * Function: H5FD_s3comms_signing_key()
 *
 * Purpose:
 *
 *    Create AWS4 "Signing Key" from secret key, AWS region, and timestamp.
 *
 *    Sequentially runs HMAC_SHA256 on strings in specified order,
 *    generating re-usable checksum (according to documentation, valid for 
 *    7 days from time given).
 *
 *    `secret` is `access key id` for targeted service/bucket/resource.
 *
 *    `iso8601now` must conform to format, yyyyMMDD'T'hhmmss'Z'
 *    e.g. "19690720T201740Z".
 *
 *    `region` should be one of AWS service region names, e.g. "us-east-1".
 *
 *    Hard-coded "service" algorithm requirement to "s3".
 *
 *    Inputs must be null-terminated strings.
 *
 *    Writes to `md` the raw byte data, length of `SHA256_DIGEST_LENGTH`.
 *    Programmer must ensure that `md` is appropriatley allocated.
 *
 * Return:
 *
 *     FAIL    if any input parameters are NULL.
 *
 *     SUCCEED 
 *
 * Programmer: Jacob Smith
 *             2017-07-13
 *
 * Changes: 
 *
 *     Integrate into HDF5.
 *     Return herr_t type.
 *             -- Jacob Smith 2017-09-18
 *
 *     NULL check and fail of input parameters.
 *             -- Jacob Smith 2017-10-10
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_signing_key(unsigned char *md,
                         const char    *secret,
                         const char    *region,
                         const char    *iso8601now)
{
    char          *AWS4_secret     = NULL;
    size_t         AWS4_secret_len = 0;
    unsigned char  datekey[SHA256_DIGEST_LENGTH];
    unsigned char  dateregionkey[SHA256_DIGEST_LENGTH];
    unsigned char  dateregionservicekey[SHA256_DIGEST_LENGTH];
    herr_t         ret_value       = SUCCEED;


    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_signing_key.\n");



    if (md == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "Destination `md` cannot be NULL.\n")
    }
    if (secret == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "`secret` cannot be NULL.\n")
    }
    if (region == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "`region` cannot be NULL.\n")
    }
    if (iso8601now == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "`iso8601now` cannot be NULL.\n")
    }

    AWS4_secret_len = 4 + strlen(secret) + 1;
    AWS4_secret = (char*)malloc(sizeof(char *) * AWS4_secret_len);
    if (AWS4_secret == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "Could not allocate space.\n")
    }

    /* prepend "AWS4" to start of the secret key 
     */
    sprintf(AWS4_secret, "%s%s", "AWS4", secret);



    /* hash_func, key, len(key), msg, len(msg), digest_dest, digest_len_dest
     * we know digest length, so ignore via NULL
     */
    HMAC(EVP_sha256(),
         (const unsigned char *)AWS4_secret,
         (int)strlen(AWS4_secret),
         (const unsigned char*)iso8601now,
         8, /* 8 --> length of 8 --> "yyyyMMDD"  */
         datekey,
         NULL);
    HMAC(EVP_sha256(),
         (const unsigned char *)datekey,
         SHA256_DIGEST_LENGTH,
         (const unsigned char *)region,
         strlen(region),
         dateregionkey,
         NULL);
    HMAC(EVP_sha256(),
         (const unsigned char *)dateregionkey, 
         SHA256_DIGEST_LENGTH,
         (const unsigned char *)"s3",
         2,
         dateregionservicekey,
         NULL);
    HMAC(EVP_sha256(),
         (const unsigned char *)dateregionservicekey,
         SHA256_DIGEST_LENGTH,
         (const unsigned char *)"aws4_request",
         12,
         md,
         NULL);



done:
    free(AWS4_secret);
    FUNC_LEAVE_NOAPI(ret_value);

} /* H5FD_s3comms_signing_key */


/****************************************************************************
 *
 * Function: H5FD_s3comms_tostringtosign()
 *
 * Purpose:
 *
 *    Get AWS "String to Sign" from Canonical Request, timestamp, 
 *    and AWS "region".
 *
 *    Common bewteen single request and "chunked upload",
 *    conforms to:
 *        "AWS4-HMAC-SHA256\n" +
 *        <ISO8601 date format> + "\n" +  // yyyyMMDD'T'hhmmss'Z'
 *        <yyyyMMDD> + "/" + <AWS Region> + "/s3/aws4-request\n" +
 *        hex(SHA256(<CANONICAL-REQUEST>))
 *
 *    Inputs `creq` (canonical request string), `now` (ISO8601 format),
 *    and `region` (s3 region designator string) must all be
 *    null-terminated strings.
 *
 *    Result is written to `dest` with null-terminator.
 *    It is left to programmer to ensure `dest` has adequate space.
 *
 * Return:
 *
 *     FAIL    if any of the inputs are NULL, 
 *             or an error is encountered while computing checkshum.
 *
 *     SUCCEED 
 *
 * Programmer: Jacob Smith
 *             2017-07-??
 *
 * Changes: 
 *
 *     Integrate with HDF5.
 *     Rename from `tostringtosign` to `H5FD_s3comms_tostringtosign`.
 *     Return `herr_t` instead of characters written.
 *     Use HDF-friendly bytes-to-hex function (`H5FD_s3comms_bytes_to_hex`)
 *         instead of general-purpose, deprecated `hex()`.
 *     Adjust casts to openssl's `SHA256`.
 *     Input strings are now `const`.
 *             -- Jacob Smith 2017-09-19
 *
 ****************************************************************************/
herr_t
H5FD_s3comms_tostringtosign(char       *dest,
                            const char *req,
                            const char *now,
                            const char *region)
{
    unsigned char checksum[SHA256_DIGEST_LENGTH * 2 + 1];
    size_t        d         = 0;
    char          day[9];
    char          hexsum[SHA256_DIGEST_LENGTH * 2 + 1];
    size_t        i         = 0;
    herr_t        ret_value = SUCCEED;
    char          tmp[128];



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_tostringtosign.\n");

    HDassert(dest != NULL);

    if (req == NULL) 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "canonical request cannot be null.\n")
    if (now == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "Timestring cannot be NULL.\n")
    if (region == NULL)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "Region cannot be NULL.\n")



    for (i = 0; i < 128; i++) {
        tmp[i] = '\0';
    }
    for (i = 0; i < SHA256_DIGEST_LENGTH * 2 + 1; i++) {
        checksum[i] = '\0';
        hexsum[i] = '\0';
    }
    strncpy(day, now, 8);
    day[8] = '\0';

    sprintf(tmp, "%s/%s/s3/aws4_request", day, region);



    memcpy((dest + d), "AWS4-HMAC-SHA256\n", 17);
    d = 17;

    memcpy((dest+d), now, strlen(now));
    d += strlen(now);
    dest[d++] = '\n';

    memcpy((dest + d), tmp, strlen(tmp));
    d += strlen(tmp);
    dest[d++] = '\n';

    SHA256((const unsigned char *)req, 
           strlen(req), 
           checksum);

    if (FAIL == 
        H5FD_s3comms_bytes_to_hex(hexsum,
                                  (const unsigned char *)checksum,
                                  SHA256_DIGEST_LENGTH,
                                  true))
    {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "could not create hex string");
    }

    for (i = 0; i < SHA256_DIGEST_LENGTH * 2; i++) {
        dest[d++] = hexsum[i];
    }

    dest[d] = '\0';

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5ros3_tostringtosign */


/****************************************************************************
 *
 * Function: H5FD_s3comms_trim()
 *
 * Purpose:
 *
 *     Remove all whitespace characters from start and end of a string `s`
 *     of length `s_len`, writing trimmed string copy to `dest`. 
 *     Stores number of characters remaining at `n_written`.
 *
 *     Destination for trimmed copy `dest` cannot be null.
 *     `dest` must have adequate space allocated for trimmed copy.
 *         If inadequate space, behavior is undefined, possibly resulting
 *         in segfault or overwrite of other data.
 *
 *     If `s` is NULL or all whitespace, `dest` is untouched and `n_written`
 *     is set to 0.
 *
 * Return:
 *
 *     FAIL iff `dest` == NULL; else SUCCEED
 *
 * Programmer: Jacob Smith
 *             2017-09-18
 *
 * Changes: 
 *
 *     Rename from `trim()` to `H5FD_s3comms_trim()`.
 *     Incorporate into HDF5.
 *     Returns `herr_t` type.
 *
 ****************************************************************************/
herr_t 
H5FD_s3comms_trim(char   *dest,
                  char   *s,
                  size_t  s_len,
                  size_t *n_written)
{
    herr_t               ret_value = SUCCEED;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "called H5FD_s3comms_trim.\n");

    if (dest == NULL) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, 
                    "destination cannot be null.")
    }
    if (s == NULL) {
        s_len = 0;
    }



    if (s_len > 0) {
        /* Find first non-whitespace character from start;
         * reduce total length per character.
         */
        while ((s_len > 0) &&
               isspace((unsigned char)s[0]) && s_len > 0) 
        {
             s++;
             s_len--;
        }

        /* Find first non-whitespace chracter from tail;
         * reduce length per-character.
         * If length is 0 already, there is no non-whitespace character.
         */
        if (s_len > 0) {
            do {
                s_len--;
            } while( isspace((unsigned char)s[s_len]) );
            s_len++;

            /* write output into dest 
             */
            memcpy(dest, s, s_len);
        }
    }

    *n_written = s_len;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_trim */


/****************************************************************************
 *
 * Function: H5FD_s3comms_uriencode()
 *
 * Purpose:
 *
 *    URIencode (percent-encode) every byte except "[a-zA-Z0-9]-._~".
 *
 *    For each charcter in souce string `_s` from `s[0]` to `s[s_len-1]`,
 *    writes to `dest` either the raw character or its percent-encoded
 *    equivalent.
 *
 *    See `H5FD_s3comms_bytes_to_hex` for information on percent-encoding.
 *
 *    Space (' ') character encoded as "%20" (not "+")
 *
 *    Forward-slash ('/') encoded as "%2F" only when `encode_slash == true`.
 *
 *    Records number of characters written at `n_written`.
 *
 *    Assumes that `dest` has been allocated with enough space.
 *
 *    Neither `dest` nor `s` can be NULL.
 *
 *    `s_len == 0` will have no effect.
 *
 * Return:
 *
 *    FAIL    if source strings `s` or destination `dest` are NULL,
 *            or error while attempting to percent-encode a character.
 *
 *    SUCCEED if no errors encountered
 *
 * Programmer: Jacob Smith
 *             2017-07-??
 *
 * Changes:
 *
 *    Integrate to HDF environment.
 *    Rename from `uriencode` to `H5FD_s3comms_uriencode`.
 *    Change return from characters written to herr_t;
 *        move to i/o parameter `n_written`.
 *    No longer append null-terminator to string;
 *        programmer may append or not as appropriate upon return.
 *            -- Jacob Smith 2017-09-15
 *
 ****************************************************************************/
herr_t 
H5FD_s3comms_uriencode(char       *dest,
                       const char *s,
                       size_t      s_len,
                       hbool_t     encode_slash,
                       size_t     *n_written)
{
    char   c         = 0;
    size_t dest_off  = 0;
    char   hex_buffer[13]; 
    size_t hex_off   = 0; 
    size_t hex_len   = 0; 
    herr_t ret_value = SUCCEED;
    size_t s_off     = 0;



    FUNC_ENTER_NOAPI_NOINIT

    HDfprintf(stdout, "H5FD_s3comms_uriencode called.\n");

    if (s == NULL) 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "source string cannot be NULL");
    if (dest == NULL) 
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "destination cannot be NULL");

    /* Write characters to destination, converting to percent-encoded
     * "hex-utf-8" strings if necessary.
     * e.g., '$' -> "%24"
     */
    for (s_off = 0; s_off < s_len; s_off++) {
        c = s[s_off];
        if (isalnum(c) ||
            c == '.'   ||
            c == '-'   ||
            c == '_'   ||
            c == '~'   ||
            (c == '/' && encode_slash == FALSE))
        {
            dest[dest_off++] = c;
        } else {
            hex_off = 0;
            if (FAIL == 
                H5FD_s3comms_percent_encode_char(hex_buffer,
                                                 (const unsigned char)c,
                                                 &hex_len))
            {
                hex_buffer[0] = c;
                hex_buffer[1] = 0;
                HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                            "unable to percent-encode character \'%s\' "
                            "at %d in \"%s\"", hex_buffer, (int)s_off, s);
            }

            for (hex_off = 0; hex_off < hex_len; hex_off++) {
                dest[dest_off++] = hex_buffer[hex_off];
            }
        }
    }

    HDassert(dest_off >= s_len);

    *n_written = dest_off;

done:
    FUNC_LEAVE_NOAPI(ret_value)

} /* H5FD_s3comms_uriencode */


