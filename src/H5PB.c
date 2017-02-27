/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*-------------------------------------------------------------------------
 *
 * Created:             H5PB.c
 *
 * Purpose:             Page Buffer routines.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#define H5F_FRIEND		/*suppress error about including H5Fpkg	  */
#include "H5PBmodule.h"         /* This source code file is part of the H5PB module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fpkg.h"		/* Files				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5FLprivate.h"	/* Free List				*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5MMprivate.h"	/* Memory Management		  	*/
#include "H5PBpkg.h"            /* File access				*/
#include "H5SLprivate.h"	/* Skip List				*/

/****************/
/* Local Macros */
/****************/
#define H5PB__PREPEND(page_ptr, head_ptr, tail_ptr, len) {              \
        if ((head_ptr) == NULL) {                                       \
            (head_ptr) = (page_ptr);                                    \
            (tail_ptr) = (page_ptr);                                    \
        }                                                               \
        else {                                                          \
            (head_ptr)->prev = (page_ptr);                              \
            (page_ptr)->next = (head_ptr);                              \
            (head_ptr) = (page_ptr);                                    \
        }                                                               \
        (len)++;                                                        \
} /* H5PB__PREPEND() */

#define H5PB__REMOVE(page_ptr, head_ptr, tail_ptr, len) {               \
        if ((head_ptr) == (page_ptr)) {                                 \
            (head_ptr) = (page_ptr)->next;                              \
            if ((head_ptr) != NULL)                                     \
                (head_ptr)->prev = NULL;                                \
        }                                                               \
        else                                                            \
            (page_ptr)->prev->next = (page_ptr)->next;                  \
        if ((tail_ptr) == (page_ptr)) {                                 \
            (tail_ptr) = (page_ptr)->prev;                              \
            if ((tail_ptr) != NULL)                                     \
                (tail_ptr)->next = NULL;                                \
        }                                                               \
        else                                                            \
            (page_ptr)->next->prev = (page_ptr)->prev;                  \
        page_ptr->next = NULL;                                          \
        page_ptr->prev = NULL;                                          \
        (len)--;                                                        \
}

#define H5PB__INSERT_LRU(page_buf, page_ptr) {                          \
        HDassert( (page_buf) );                                         \
        HDassert( (page_ptr) );                                         \
        /* insert the entry at the head of the list. */                 \
        H5PB__PREPEND((page_ptr), (page_buf)->LRU_head_ptr,             \
                      (page_buf)->LRU_tail_ptr, (page_buf)->LRU_list_len) \
}

#define H5PB__REMOVE_LRU(page_buf, page_ptr) {                          \
        HDassert( (page_buf) );                                         \
        HDassert( (page_ptr) );                                         \
        /* remove the entry from the list. */                           \
        H5PB__REMOVE((page_ptr), (page_buf)->LRU_head_ptr,              \
                     (page_buf)->LRU_tail_ptr, (page_buf)->LRU_list_len) \
}

#define H5PB__MOVE_TO_TOP_LRU(page_buf, page_ptr) {                     \
        HDassert( (page_buf) );                                         \
        HDassert( (page_ptr) );                                         \
        /* Remove entry and insert at the head of the list. */          \
        H5PB__REMOVE((page_ptr), (page_buf)->LRU_head_ptr,              \
                     (page_buf)->LRU_tail_ptr, (page_buf)->LRU_list_len) \
        H5PB__PREPEND((page_ptr), (page_buf)->LRU_head_ptr,            \
                       (page_buf)->LRU_tail_ptr, (page_buf)->LRU_list_len) \
}

/******************/
/* Local Typedefs */
/******************/

/* Iteration context for flushing page buffer */
typedef struct {
    const H5F_t *f;
    H5P_genplist_t *dxpl;
} H5PB_ud1_t;

/* Iteration context for destroying page buffer */
typedef struct {
    H5PB_t *page_buf;
    hbool_t actual_slist;
} H5PB_ud2_t;


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/
static herr_t H5PB__insert_entry(H5PB_t *page_buf, H5PB_entry_t *page_entry);
static htri_t H5PB__make_space(const H5F_t *f, H5PB_t *page_buf, H5P_genplist_t *dxpl, H5FD_mem_t inserted_type);
static herr_t H5PB__write_entry(const H5F_t *f, H5PB_entry_t *page_entry, H5P_genplist_t *dxpl);

/*********************/
/* Package Variables */
/*********************/

/* Package initialization variable */
hbool_t H5_PKG_INIT_VAR = FALSE;

/*****************************/
/* Library Private Variables */
/*****************************/
/* Declare a free list to manage the H5PB_t struct */
H5FL_DEFINE_STATIC(H5PB_t);
/* Declare a free list to manage the H5PB_entry_t struct */
H5FL_DEFINE_STATIC(H5PB_entry_t);

/*******************/
/* Local Variables */
/*******************/



/*-------------------------------------------------------------------------
 * Function:	H5PB_reset_stats
 *
 * Purpose:     This function was created without documentation.
 *              What follows is my best understanding of Mohamad's intent.
 *
 *              Reset statistics collected for the page buffer layer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5PB_reset_stats(H5PB_t *page_buf)
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(page_buf);

    page_buf->accesses[0] = 0;
    page_buf->accesses[1] = 0;
    page_buf->hits[0] = 0;
    page_buf->hits[1] = 0;
    page_buf->misses[0] = 0;
    page_buf->misses[1] = 0;
    page_buf->evictions[0] = 0;
    page_buf->evictions[1] = 0;
    page_buf->bypasses[0] = 0;
    page_buf->bypasses[1] = 0;

done:
    FUNC_LEAVE_NOAPI(ret_value)

}  /* H5PB_reset_stats() */


/*-------------------------------------------------------------------------
 * Function:	H5PB_get_stats
 *
 * Purpose:     This function was created without documentation.
 *              What follows is my best understanding of Mohamad's intent.
 *
 *              Retrieve statistics collected about page accesses for the page buffer layer.
 *              --accesses: the number of metadata and raw data accesses to the page buffer layer
 *              --hits: the number of metadata and raw data hits in the page buffer layer
 *              --misses: the number of metadata and raw data misses in the page buffer layer
 *              --evictions: the number of metadata and raw data evictions from the page buffer layer
 *              --bypasses: the number of metadata and raw data accesses that bypass the page buffer layer
 *
 * Return:	    Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5PB_get_stats(const H5PB_t *page_buf, int accesses[2], int hits[2], int misses[2], int evictions[2], int bypasses[2])
{
    herr_t ret_value = SUCCEED;

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(page_buf);

    accesses[0] = page_buf->accesses[0];
    accesses[1] = page_buf->accesses[1];
    hits[0] = page_buf->hits[0];
    hits[1] = page_buf->hits[1];
    misses[0] = page_buf->misses[0];
    misses[1] = page_buf->misses[1];
    evictions[0] = page_buf->evictions[0];
    evictions[1] = page_buf->evictions[1];
    bypasses[0] = page_buf->bypasses[0];
    bypasses[1] = page_buf->bypasses[1];

done:
    FUNC_LEAVE_NOAPI(ret_value)

}  /* H5PB_get_stats */


/*-------------------------------------------------------------------------
 * Function:	H5PB_print_stats()
 *
 * Purpose:     This function was created without documentation.
 *              What follows is my best understanding of Mohamad's intent.
 *
 *              Print out statistics collected for the page buffer layer.
 *
 * Return:	    Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_print_stats(const H5PB_t *page_buf)
{
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOERR

    HDassert(page_buf);

    printf("PAGE BUFFER STATISTICS:\n");

    printf("******* METADATA\n");
    printf("\t Total Accesses: %d\n", page_buf->accesses[0]);
    printf("\t Hits: %d\n", page_buf->hits[0]);
    printf("\t Misses: %d\n", page_buf->misses[0]);
    printf("\t Evictions: %d\n", page_buf->evictions[0]);
    printf("\t Bypasses: %d\n", page_buf->bypasses[0]);
    printf("\t Hit Rate = %f%%\n", ((double)page_buf->hits[0]/(page_buf->accesses[0] - page_buf->bypasses[0]))*100);
    printf("*****************\n\n");

    printf("******* RAWDATA\n");
    printf("\t Total Accesses: %d\n", page_buf->accesses[1]);
    printf("\t Hits: %d\n", page_buf->hits[1]);
    printf("\t Misses: %d\n", page_buf->misses[1]);
    printf("\t Evictions: %d\n", page_buf->evictions[1]);
    printf("\t Bypasses: %d\n", page_buf->bypasses[1]);
    printf("\t Hit Rate = %f%%\n", ((double)page_buf->hits[1]/(page_buf->accesses[1]-page_buf->bypasses[0]))*100);
    printf("*****************\n\n");

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_print_stats */


/*-------------------------------------------------------------------------
 * Function:	H5PB_create
 *
 * Purpose:	Create and setup the PB on the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_create(H5F_t *f, size_t size, unsigned page_buf_min_meta_perc, unsigned page_buf_min_raw_perc)
{
    H5PB_t *page_buf = NULL;
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);
    HDassert(f->shared);

    /* check args */
    if(f->shared->fs_strategy != H5F_FSPACE_STRATEGY_PAGE)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "Enabling Page Buffering requires PAGE file space strategy")
    /* round down the size if it is larger than the page size */
    else if(size > f->shared->fs_page_size) {
        hsize_t temp_size;

        temp_size = (size / f->shared->fs_page_size) * f->shared->fs_page_size;
        H5_CHECKED_ASSIGN(size, size_t, temp_size, hsize_t);
    } /* end if */
    else if(0 != size % f->shared->fs_page_size)
        HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTINIT, FAIL, "Page Buffer size must be >= to the page size");

    /* Allocate the new page buffering structure */
    if(NULL == (page_buf = H5FL_CALLOC(H5PB_t)))
	HGOTO_ERROR(H5E_PAGEBUF, H5E_NOSPACE, FAIL, "memory allocation failed")

    page_buf->max_size = size;
    H5_CHECKED_ASSIGN(page_buf->page_size, size_t, f->shared->fs_page_size, hsize_t);
    page_buf->min_meta_perc = page_buf_min_meta_perc;
    page_buf->min_raw_perc = page_buf_min_raw_perc;

    /* Calculate the minimum page count for metadata and raw data
     * based on the fractions provided 
     */
    page_buf->min_meta_count = (unsigned)((size * page_buf_min_meta_perc) / (f->shared->fs_page_size * 100));
    page_buf->min_raw_count = (unsigned)((size * page_buf_min_raw_perc) / (f->shared->fs_page_size * 100));

//fprintf(stderr, "Creating a page buffer of size %zu, Page size = %zu\n", size, (size_t)f->shared->fs_page_size);
//fprintf(stderr, "MIN metadata count = %u, MIN raw data count = %u\n", page_buf->min_meta_count, page_buf->min_raw_count);

    if(NULL == (page_buf->slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTCREATE, FAIL, "can't create skip list")
    if(NULL == (page_buf->mf_slist_ptr = H5SL_create(H5SL_TYPE_HADDR, NULL)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTCREATE, FAIL, "can't create skip list")

    f->shared->page_buf = page_buf;

done:
    if(ret_value < 0) {
        if(page_buf != NULL) {
            if(page_buf->slist_ptr != NULL)
                H5SL_close(page_buf->slist_ptr);
            page_buf = H5FL_FREE(H5PB_t, page_buf);
        } /* end if */
    } /* end if */

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_create */


/*-------------------------------------------------------------------------
 * Function:	H5PB__flush_cb
 *
 * Purpose:	Callback to flush PB skiplist entries.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PB__flush_cb(void *item, void H5_ATTR_UNUSED *key, void *_op_data)
{
    H5PB_entry_t *page_entry = (H5PB_entry_t *)item;    /* Pointer to page entry node */
    H5PB_ud1_t *op_data = (H5PB_ud1_t *)_op_data;
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    /* Sanity checks */
    HDassert(page_entry);
    HDassert(op_data);
    HDassert(op_data->f);
    HDassert(op_data->f->shared);
    HDassert(op_data->f->shared->page_buf);

    /* Flush the page if it's dirty */
    if(page_entry->is_dirty)
        if(H5PB__write_entry(op_data->f, page_entry, op_data->dxpl) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_WRITEERROR, FAIL, "file write failed")

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB__flush_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5PB_flush
 *
 * Purpose:	Flush/Free all the PB entries to the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_flush(H5F_t *f, hid_t dxpl_id)
{
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity check */
    HDassert(f);

    /* Flush all the entries in the PB skiplist, if we have write access on the file */
    if(f->shared->page_buf && (H5F_ACC_RDWR & H5F_INTENT(f))) {
        H5PB_t *page_buf = f->shared->page_buf;
        H5PB_ud1_t op_data;     /* Context for iteration */
        H5P_genplist_t *dxpl;   /* Data transfer property list struct */

        /* Get the pointer to the DXPL */
        if(NULL == (dxpl = (H5P_genplist_t *)H5I_object(dxpl_id)))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_BADTYPE, FAIL, "can't get property list")

        /* Set up context info */
        op_data.f = f;
        op_data.dxpl = dxpl;

        /* Iterate over all entries in page buffer skip list */
        if(H5SL_iterate(page_buf->slist_ptr, H5PB__flush_cb, &op_data))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_BADITER, FAIL, "can't flush page buffer skip list")
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_flush */


/*-------------------------------------------------------------------------
 * Function:	H5PB__dest_cb
 *
 * Purpose:	Callback to free PB skiplist entries.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PB__dest_cb(void *item, void H5_ATTR_UNUSED *key, void *_op_data)
{
    H5PB_entry_t *page_entry = (H5PB_entry_t *)item;       /* Pointer to page entry node */
    H5PB_ud2_t *op_data = (H5PB_ud2_t *)_op_data;

    FUNC_ENTER_STATIC_NOERR

    /* Sanity checking */
    HDassert(page_entry);
    HDassert(FALSE == page_entry->is_dirty);
    HDassert(op_data);
    HDassert(op_data->page_buf);

    /* Remove entry from LRU list */
    if(op_data->actual_slist) {
        H5PB__REMOVE_LRU(op_data->page_buf, page_entry)
        H5MM_free(page_entry->page_buf_ptr);
        page_entry->page_buf_ptr = NULL;
    } /* end if */

    /* Free page entry */
    page_entry = H5FL_FREE(H5PB_entry_t, page_entry);

    FUNC_LEAVE_NOAPI(SUCCEED)
} /* H5PB__dest_cb() */


/*-------------------------------------------------------------------------
 * Function:	H5PB_dest
 *
 * Purpose:	destroy the PB on the file.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_dest(H5F_t *f)
{
    herr_t  ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    /* Sanity checks */
    HDassert(f);

    /* Destroy page buffer info, if there is any */
    if(f->shared->page_buf) {
        H5PB_t *page_buf = f->shared->page_buf;
        H5PB_ud2_t op_data;                 /* Iteration context */

        /* Set up context info */
        op_data.page_buf = page_buf;

        /* Destroy the skip list containing all the entries in the PB */
        op_data.actual_slist = TRUE;
        if(H5SL_destroy(page_buf->slist_ptr, H5PB__dest_cb, &op_data))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTCLOSEOBJ, FAIL, "can't destroy page buffer skip list")

        /* Destroy the skip list containing the new entries */
        op_data.actual_slist = FALSE;
        if(H5SL_destroy(page_buf->mf_slist_ptr, H5PB__dest_cb, &op_data))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTCLOSEOBJ, FAIL, "can't destroy page buffer skip list")

#ifdef QAK
H5PB_print_stats(page_buf);
#endif /* QAK */

        f->shared->page_buf = H5FL_FREE(H5PB_t, page_buf);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_dest */


/*-------------------------------------------------------------------------
 * Function:	H5PB_add_new_page
 *
 * Purpose:	Add a new page to the new page skip list. This is called 
 *              from the MF layer when a new page is allocated to 
 *              indicate to the page buffer layer that a read of the page 
 *              from the file is not necessary since it's an empty page.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5PB_add_new_page(H5F_t *f, H5FD_mem_t type, haddr_t page_addr)
{
    H5PB_t *page_buf = f->shared->page_buf;
    H5PB_entry_t *page_entry = NULL;    /* pointer to the corresponding page entry */
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(page_buf);
    //fprintf(stderr, "New page MF INSERT type %d at addr %llu\n", type, page_addr);

    /* if there is an existing page, this means that at some point the
       file space manager freed and re-allocated a page at the same
       address. no need to do anything */
    /* MSC - to be safe, might want to dig in the MF layer and remove
       the page when it is freed from this list if it still exists and
       remove this check */
    if(H5SL_search(page_buf->mf_slist_ptr, &(page_addr)))
        HGOTO_DONE(SUCCEED);

    /* create the new PB entry */
    if(NULL == (page_entry = H5FL_CALLOC(H5PB_entry_t)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_NOSPACE, FAIL, "memory allocation failed");

    page_entry->addr = page_addr;
    page_entry->type = (H5F_mem_page_t)type;
    page_entry->is_dirty = FALSE;

    /* insert entry in skip list */
    if(H5SL_insert(page_buf->mf_slist_ptr, page_entry, &(page_entry->addr)) < 0)
        HGOTO_ERROR(H5E_PAGEBUF, H5E_BADVALUE, FAIL, "Can't insert entry in skip list");

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_add_new_page */

#ifdef H5_HAVE_PARALLEL

/*-------------------------------------------------------------------------
 * Function:	H5PB_update_entry
 *
 * Purpose:	In PHDF5, entries that are written by other processes and just 
 *              marked clean by this process have to have their corresponding 
 *              pages updated if they exist in the page buffer. 
 *              This routine checks and update the pages.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5PB_update_entry(H5PB_t *page_buf, haddr_t addr, size_t size, const void *buf)
{
    H5PB_entry_t *page_entry = NULL;    /* pointer to the corresponding page entry */
    haddr_t page_addr;
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI_NOERR

    HDassert(page_buf);
    HDassert(size <= page_buf->page_size);
    HDassert(buf);

    /* calculate the aligned address of the first page */
    page_addr = (addr/page_buf->page_size) * page_buf->page_size;

    /* search for the page and update if found */
    page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&page_addr));
    if(page_entry) {
        haddr_t offset;

        HDassert(addr + size <= page_addr + page_buf->page_size);
        offset = addr - page_addr;
        HDmemcpy((uint8_t *)page_entry->page_buf_ptr + offset, buf, size);

        /* move to top of LRU list */
        H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
    }

    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_update_entry */
#endif /* H5_HAVE_PARALLEL */


/*-------------------------------------------------------------------------
 * Function:    H5PB_remove_entry
 *
 * Purpose:     Remove possible metadata entry with ADDR from the PB cache.
 *              This is in response to the data corruption bug from fheap.c 
 *              with page buffering + page strategy.
 *              Note: Large metadata page bypasses the PB cache.
 *              Note: Update of raw data page (large or small sized) is handled by the PB cache.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Vailin Choi; Feb 2017
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_remove_entry(const H5F_t *f, H5FD_mem_t type, haddr_t addr, hsize_t size)
{
    H5PB_t *page_buf = f->shared->page_buf;
    H5PB_entry_t *page_entry = NULL;        /* pointer to the page entry being searched */
    herr_t ret_value = SUCCEED;             /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

    HDassert(page_buf);
    HDassert(size == page_buf->page_size);
    HDassert(type != H5FD_MEM_DRAW);

    /* Search for address in the skip list */
    page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&addr));

    /* If found, remove the entry from the PB cache */
    if(page_entry) {
        HDassert(page_entry->type != H5FD_MEM_DRAW);
        if(NULL == H5SL_remove(page_buf->slist_ptr, &(page_entry->addr)))
            HGOTO_ERROR(H5E_CACHE, H5E_BADVALUE, FAIL, "Page Entry is not in skip list")

        /* remove from LRU list */
        H5PB__REMOVE_LRU(page_buf, page_entry)
        HDassert(H5SL_count(page_buf->slist_ptr) == page_buf->LRU_list_len);

        page_buf->meta_count--;

        H5MM_free(page_entry->page_buf_ptr);
        page_entry->page_buf_ptr = NULL;
        page_entry = H5FL_FREE(H5PB_entry_t, page_entry);
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB_remove_entry */


/*-------------------------------------------------------------------------
 * Function:	H5PB_read
 *
 * Purpose:	Reads in the data from the page containing it if it exists 
 *              in the PB cache; otherwise reads in the page through the VFD.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_read(const H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size,
    hid_t dxpl_id, void *buf/*out*/)
{
    H5PB_t *page_buf = f->shared->page_buf;
    H5PB_entry_t *page_entry = NULL;    /* pointer to the corresponding page entry */
    haddr_t offset, buf_offset;
    haddr_t search_addr;
    H5P_genplist_t *dxpl = NULL;
    haddr_t first_page_addr, last_page_addr;
    size_t access_size;
    hsize_t i;
    hsize_t num_touched_pages;
    hbool_t mpio_bypass_pb = FALSE;
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

#if H5PB_COLLECT_STATS
        if(page_buf) {
            if(type == H5FD_MEM_DRAW)
                page_buf->accesses[1] ++;
            else
                page_buf->accesses[0] ++;
        }
#endif /* H5PB_COLLECT_STATS */

#ifdef H5_HAVE_PARALLEL
    if(H5F_HAS_FEATURE(f, H5FD_FEAT_HAS_MPI)) {
#if 1
        mpio_bypass_pb = TRUE;
#else
        /* MSC - why this stopped working ? */
        int mpi_size;

        if((mpi_size = H5F_mpi_get_size(f)) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "can't retrieve MPI communicator size");
        if(1 != mpi_size)
            mpio_bypass_pb = TRUE;
#endif
    }
#endif
    //fprintf(stderr, "%s: addr %llu, size %zu\n", FUNC, addr, size);
    if(NULL == (dxpl = (H5P_genplist_t *)H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_BADTYPE, FAIL, "can't get property list")

    /*  if page buffering is disabled,
        or the I/O size is large than that of a single page,
        or if this is a parallel raw data access,
        go to VFD directly */
    /*  Instead of going to the VFD directly, the accumulator is enabled.
     *  Passing through the metadata accumulator layer fixes the long running time 
     *  for test/fheap.c with the core driver.
     *  But the real issue is with the core driver which will be addressed later.
     */
    if(NULL == page_buf || size >= page_buf->page_size ||
       (mpio_bypass_pb && H5FD_MEM_DRAW == type)) {
        H5F_io_info_t fio_info;             /* I/O info for operation */

        /* Set up I/O info for operation */
        fio_info.f = f;
        fio_info.dxpl = dxpl;
        if(H5F__accum_read(&fio_info, type, addr, size, buf) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "read through metadata accumulator failed")
#if H5PB_COLLECT_STATS
        if(page_buf) {
            if(type == H5FD_MEM_DRAW)
                page_buf->bypasses[1] ++;
            else
                page_buf->bypasses[0] ++;
        }
#endif /* H5PB_COLLECT_STATS */
    }

    /* If page buffering is disabled, 
       or if this is a large metadata access, 
       or if this is parallel raw data access, 
       we are done here */
    if(NULL == page_buf || (size >= page_buf->page_size && H5FD_MEM_DRAW != type) ||
       (mpio_bypass_pb && H5FD_MEM_DRAW == type))
        HGOTO_DONE(ret_value);

    /* calculate the aligned address of the first page */
    first_page_addr = (addr/page_buf->page_size) * page_buf->page_size;

    /* For Raw data calculate the aligned address of the last page and
       the number of pages accessed if more than 1 page is accessed */
    if(H5FD_MEM_DRAW == type) {
        last_page_addr = (addr+size-1)/page_buf->page_size * page_buf->page_size;

        /* how many pages does this write span */
        num_touched_pages = (last_page_addr/page_buf->page_size + 1) - 
            (first_page_addr/page_buf->page_size);
        if(first_page_addr == last_page_addr) {
            HDassert(1 == num_touched_pages);
            last_page_addr = HADDR_UNDEF;
        }
    }
    /* Otherwise set last page addr to HADDR_UNDEF */
    else {
        num_touched_pages = 1;
        last_page_addr = HADDR_UNDEF;
    }

    /* Copy raw data from dirty pages into the read buffer if the read
       request spans pages in the page buffer*/
    if(H5FD_MEM_DRAW == type && size >= page_buf->page_size) {
        H5SL_node_t *node = NULL;

        //fprintf(stderr, "Checking RAW read that bypassed PB\n");

        /* for each touched page in the page buffer, check if it
           exists in the page Buffer and is dirty. If it does, we
           update the buffer with what's in the page so we get the up
           to date data into the buffer after the big read from the
           file. */

        node = H5SL_find(page_buf->slist_ptr, (void *)(&first_page_addr));

        for(i=0 ; i<num_touched_pages ; i++) {
            search_addr = i*page_buf->page_size + first_page_addr;

            /* if we still haven't located a starting page, search again */
            if(!node && i!=0)
                node = H5SL_find(page_buf->slist_ptr, (void *)(&search_addr));

            /* if the current page is in the Page Buffer, do the updates */
            if(node) {
                page_entry = (H5PB_entry_t *)H5SL_item(node);

                HDassert(page_entry);

                /* If the current page address falls out of the access
                   block, then there are no more pages to go over */
                if(page_entry->addr >= addr + size)
                    break;

                HDassert(page_entry->addr == search_addr);

                if(page_entry->is_dirty) {
                    /* special handling for the first page if it is not a full page access */
                    if(i == 0 && first_page_addr != addr) {
                        offset = addr - first_page_addr;
                        HDassert(page_buf->page_size > offset);

                        HDmemcpy(buf, (uint8_t *)page_entry->page_buf_ptr + offset, 
                                 page_buf->page_size - (size_t)offset);

                        /* move to top of LRU list */
                        H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
                    }
                    /* special handling for the last page if it is not a full page access */
                    else if(num_touched_pages > 1 && i == num_touched_pages-1 && search_addr < addr+size) {
                        offset = (num_touched_pages-2)*page_buf->page_size + 
                            (page_buf->page_size - (addr - first_page_addr));

                        HDmemcpy((uint8_t *)buf+offset, page_entry->page_buf_ptr,
                                 (size_t)((addr+size) - last_page_addr));

                        /* move to top of LRU list */
                        H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
                    }
                    /* copy the entire fully accessed pages */
                    else {
                        offset = i*page_buf->page_size;

                        HDmemcpy((uint8_t *)buf+(i*page_buf->page_size) , page_entry->page_buf_ptr, 
                             page_buf->page_size);
                    }
                }
                node = H5SL_next(node);
            }
        }
        HGOTO_DONE(ret_value);
    }

    /* a raw data access could span 1 or 2 PB entries at this point so
       we need to handle that */
    HDassert(1 == num_touched_pages || 2 == num_touched_pages);

    for(i=0 ; i<num_touched_pages ; i++) {

        if(1 == i && HADDR_UNDEF == last_page_addr) {
            HDassert(1 == num_touched_pages);
            HGOTO_DONE(ret_value);
        }

        /* calculate the aligned address of the page to search for it in the skip list */
        search_addr = (0==i ? first_page_addr : last_page_addr);

        /* calculate the access size if the access spans more than 1 page */
        if(1 == num_touched_pages)
            access_size = size;
        else
            access_size = (0==i ? (size_t)(first_page_addr + page_buf->page_size - addr) : (size-access_size));

        /* lookup the page in the skip list */
        page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&search_addr));

        /* if found */
        if(page_entry) {
#if H5PB_COLLECT_STATS
            if(type == H5FD_MEM_DRAW)
                page_buf->hits[1] ++;
            else
                page_buf->hits[0] ++;
#endif /* H5PB_COLLECT_STATS */
            offset = (0==i ? addr - page_entry->addr : 0);
            buf_offset = (0==i ? 0 : size-access_size);
            /* copy the requested data from the page into the input buffer */
            HDmemcpy((uint8_t *)buf+buf_offset, (uint8_t *)page_entry->page_buf_ptr + offset, access_size);

            /* update LRU */
            H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
        }
        /* if not found */ 
        else {
            void *new_page_buf = NULL;
            size_t page_size = page_buf->page_size;
            haddr_t eoa;

#if H5PB_COLLECT_STATS
            if(type == H5FD_MEM_DRAW)
                page_buf->misses[1] ++;
            else
                page_buf->misses[0] ++;
#endif /* H5PB_COLLECT_STATS */

            /* make space for new entry */
            if((H5SL_count(page_buf->slist_ptr) * page_buf->page_size) >= page_buf->max_size) {
                htri_t can_make_space;

                /* check if we can make space in page buffer */
                if((can_make_space = H5PB__make_space(f, page_buf, dxpl, type)) < 0)
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_SYSTEM, FAIL, "make space in Page buffer Failed");

                /* if make_space returns 0, then we can't use the page
                   buffer for this I/O and we need to bypass */
                if(0 == can_make_space) {
                    /* make space can't return FALSE on second touched page since the first is of the same type */
                    HDassert(0 == i);

                    /* read entire block from VFD and return */
                    if(H5FD_read(f->shared->lf, dxpl, type, addr, size, buf) < 0)
                        HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "driver read request failed");
                    HGOTO_DONE(SUCCEED);
                }
            }

            /* read page from VFD */
            if(NULL == (new_page_buf = H5MM_malloc(page_size)))
                HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTALLOC, FAIL, "memory allocation failed for page buffer entry");

            /* Read page through the VFD layer, but make sure we don't read past the EOA. */

            /* Retrieve the 'eoa' for the file */
            if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, type)))
                HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "driver get_eoa request failed");

            /* if the entire page falls outside the EOA, then fail */
            if(search_addr > eoa)
                HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "Reading an entire page that is outside the file EOA");

            /* adjust the read size to not go beyond the EOA */
            if(search_addr + page_size > eoa)
                page_size = (size_t)(eoa - search_addr);

            /* read page from VFD */
            if(H5FD_read(f->shared->lf, dxpl, type, search_addr, page_size, new_page_buf) < 0)
                HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "driver read request failed");

            /* copy the requested data from the page into the input buffer */
            offset = (0==i ? addr - search_addr : 0);
            buf_offset = (0==i ? 0 : size-access_size);
            HDmemcpy((uint8_t *)buf+buf_offset, (uint8_t *)new_page_buf + offset, access_size);

            /* create the new PB entry */
            if(NULL == (page_entry = H5FL_CALLOC(H5PB_entry_t)))
                HGOTO_ERROR(H5E_PAGEBUF, H5E_NOSPACE, FAIL, "memory allocation failed");

            page_entry->page_buf_ptr = new_page_buf;
            page_entry->addr = search_addr;
            page_entry->type = (H5F_mem_page_t)type;
            page_entry->is_dirty = FALSE;

            /* insert page into PB */
            if(H5PB__insert_entry(page_buf, page_entry) < 0)
                HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTSET, FAIL, "error inserting new page in page buffer");
        }
    }
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PB_read() */


/*-------------------------------------------------------------------------
 * Function:	H5PB_write
 *
 * Purpose: Write data into the Page Buffer. If the page exists in the
 *          cache, update it; otherwise read it from disk, update it, and
 *          insert into cache.
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5PB_write(const H5F_t *f, H5FD_mem_t type, haddr_t addr, size_t size,
    hid_t dxpl_id, const void *buf)
{
    H5PB_t *page_buf = f->shared->page_buf;
    H5PB_entry_t *page_entry = NULL;    /* pointer to the corresponding page entry */
    haddr_t offset, buf_offset;
    haddr_t search_addr;
    H5P_genplist_t *dxpl = NULL;
    haddr_t first_page_addr, last_page_addr;
    size_t access_size;
    hsize_t i;
    hsize_t num_touched_pages;
    hbool_t mpio_bypass_pb = FALSE;
    herr_t  ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_NOAPI(FAIL)

#if H5PB_COLLECT_STATS
        if(page_buf) {
            if(type == H5FD_MEM_DRAW)
                page_buf->accesses[1] ++;
            else
                page_buf->accesses[0] ++;
        }
#endif /* H5PB_COLLECT_STATS */
    //fprintf(stderr, "%s: addr %llu, size %zu\n", FUNC, addr, size);
#ifdef H5_HAVE_PARALLEL
    if(H5F_HAS_FEATURE(f, H5FD_FEAT_HAS_MPI)) {
#if 1
        mpio_bypass_pb = TRUE;
#else
        /* MSC - why this stopped working ? */
        int mpi_size;

        if((mpi_size = H5F_mpi_get_size(f)) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "can't retrieve MPI communicator size");
        if(1 != mpi_size)
            mpio_bypass_pb = TRUE;
#endif
    }
#endif

    if(NULL == (dxpl = (H5P_genplist_t *)H5I_object(dxpl_id)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_BADTYPE, FAIL, "can't get property list")

    /*  If page buffering is not enabled,
        or the I/O size is large than that of a single page,
        or if this is parallel access with mpio vfd,
        go to VFD directly */
    /*  Instead of going to the VFD directly, the accumulator is enabled.
     *  Passing through the metadata accumulator layer fixes the long running time 
     *  for test/fheap.c with the core driver.
     *  But the real issue is with the core driver which will be addressed later.
     */
    if(NULL == page_buf || size >= page_buf->page_size || mpio_bypass_pb) {
        H5F_io_info_t fio_info;     /* I/O info for operation */

        /* Set up I/O info for operation */
        fio_info.f = f;
        fio_info.dxpl = dxpl;
        if(H5F__accum_write(&fio_info, type, addr, size, buf) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_WRITEERROR, FAIL, "write through metadata accumulator failed")
#if H5PB_COLLECT_STATS
        if(page_buf) {
            if(type == H5FD_MEM_DRAW)
                page_buf->bypasses[1] ++;
            else
                page_buf->bypasses[0] ++;
        }
#endif /* H5PB_COLLECT_STATS */
    }

    /* If page buffering is disabled, 
       or if this is a large metadata access, 
       or if this is a parallel raw data access, 
       we are done here */
    if(NULL == page_buf || (size >= page_buf->page_size && H5FD_MEM_DRAW != type) ||
       (mpio_bypass_pb && H5FD_MEM_DRAW == type))
        HGOTO_DONE(ret_value);

#ifdef H5_HAVE_PARALLEL
    if(mpio_bypass_pb) {
        HDassert(H5FD_MEM_DRAW != type);
        if(H5PB_update_entry(page_buf, addr, size, buf) > 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_SYSTEM, FAIL, "Failed to update PB with metadata cache\n");
        HGOTO_DONE(ret_value);
    }
#endif

    /* calculate the aligned address of the first page */
    first_page_addr = (addr/page_buf->page_size) * page_buf->page_size;

    /* For Raw data calculate the aligned address of the last page and
       the number of pages accessed if more than 1 page is accessed */
    if(H5FD_MEM_DRAW == type) {
        last_page_addr = (addr+size-1)/page_buf->page_size * page_buf->page_size;

        /* how many pages does this write span */
        num_touched_pages = (last_page_addr/page_buf->page_size + 1) - 
            (first_page_addr/page_buf->page_size);
        if(first_page_addr == last_page_addr) {
            HDassert(1 == num_touched_pages);
            last_page_addr = HADDR_UNDEF;
        }
    }
    /* Otherwise set last page addr to HADDR_UNDEF */
    else {
        num_touched_pages = 1;
        last_page_addr = HADDR_UNDEF;
    }

    //fprintf(stderr, "First Page addr: %llu ; Last page addr: %llu\n", first_page_addr, last_page_addr);

    /* Check if existing pages for raw data need to be updated since raw data access is not atomic */
    if(H5FD_MEM_DRAW == type && size >= page_buf->page_size) {
        //fprintf(stderr, "Checking RAW write that bypassed PB\n");

        /* For each touched page, check if it exists in the page
           buffer, and update it with the data in the buffer to keep
           it up to date */
        for(i=0 ; i<num_touched_pages ; i++) {
            search_addr = i*page_buf->page_size + first_page_addr;

            /* special handling for the first page if it is not a full page update */
            if(i == 0 && first_page_addr != addr) {
                /* lookup the page in the skip list */
                page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&search_addr));
                if(page_entry) {
                    offset = addr - first_page_addr;
                    HDassert(page_buf->page_size > offset);

                    HDmemcpy((uint8_t *)page_entry->page_buf_ptr + offset, buf, page_buf->page_size - (size_t)offset);

                    page_entry->is_dirty = TRUE;
                    H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
                }
            }
            /* special handling for the last page if it is not a full page update */
            else if(num_touched_pages > 1 && i == num_touched_pages-1 && 
                    search_addr+page_buf->page_size != addr+size) {
                HDassert(search_addr+page_buf->page_size > addr+size);

                /* lookup the page in the skip list */
                page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&search_addr));
                if(page_entry) {
                    offset = (num_touched_pages-2)*page_buf->page_size + 
                        (page_buf->page_size - (addr - first_page_addr));

                    HDmemcpy(page_entry->page_buf_ptr, (const uint8_t *)buf+offset, 
                             (size_t)((addr+size) - last_page_addr));

                    page_entry->is_dirty = TRUE;
                    H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
                }
            }
            /* discard all fully written pages from the page buffer*/
            else {
                page_entry = (H5PB_entry_t *)H5SL_remove(page_buf->slist_ptr, (void *)(&search_addr));
                if(page_entry) {
                    /* remove from LRU list */
                    H5PB__REMOVE_LRU(page_buf, page_entry)

                    if(H5F_MEM_PAGE_DRAW == page_entry->type || H5F_MEM_PAGE_GHEAP == page_entry->type)
                        page_buf->raw_count --;
                    else
                        page_buf->meta_count --;

                    H5MM_free(page_entry->page_buf_ptr);
                    page_entry->page_buf_ptr = NULL;
                    page_entry = H5FL_FREE(H5PB_entry_t, page_entry);
                }
            }
        }
        HGOTO_DONE(ret_value);
    }

    /* a raw data access could span 1 or 2 PBs at this point so we need to handle that */
    HDassert(1 == num_touched_pages || 2 == num_touched_pages);
    for(i=0 ; i<num_touched_pages ; i++) {

        if(1 == i && HADDR_UNDEF == last_page_addr) {
            HDassert(1 == num_touched_pages);
            HGOTO_DONE(ret_value);
        }

        /* calculate the aligned address of the page to search for it in the skip list */
        search_addr = (0==i ? first_page_addr : last_page_addr);

        /* calculate the access size if the access spans more than 1 page */
        if(1 == num_touched_pages)
            access_size = size;
        else
            access_size = (0==i ? (size_t)(first_page_addr + page_buf->page_size - addr) : (size-access_size));

        /* lookup the page in the skip list */
        page_entry = (H5PB_entry_t *)H5SL_search(page_buf->slist_ptr, (void *)(&search_addr));

        /* if found */
        if(page_entry) {
#if H5PB_COLLECT_STATS
            if(type == H5FD_MEM_DRAW)
                page_buf->hits[1] ++;
            else
                page_buf->hits[0] ++;
#endif /* H5PB_COLLECT_STATS */
            //fprintf(stderr, "Existing page at addr %llu, buf ptr %p\n", page_entry->addr, page_entry->page_buf_ptr);
            offset = (0==i ? addr - page_entry->addr : 0);
            buf_offset = (0==i ? 0 : size-access_size);

            //fprintf(stderr, "Memcpy to offset %llu size %zu\n", offset, access_size);
            /* copy the requested data from the input buffer into the page */
            HDmemcpy((uint8_t *)page_entry->page_buf_ptr + offset, (const uint8_t *)buf + buf_offset, access_size);

            if(FALSE == page_entry->is_dirty)
                page_entry->is_dirty = TRUE;

            /* update LRU */
            H5PB__MOVE_TO_TOP_LRU(page_buf, page_entry)
        }
        /* if not found */ 
        else {
            void *new_page_buf = NULL;
            size_t page_size = page_buf->page_size;

            /* make space for new entry */
            if((H5SL_count(page_buf->slist_ptr) * page_buf->page_size) >= page_buf->max_size) {
                htri_t can_make_space;
                /* check if we can make space in page buffer */
                if((can_make_space = H5PB__make_space(f, page_buf, dxpl, type)) < 0)
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_SYSTEM, FAIL, "make space in Page buffer Failed");

                /* if make_space returns 0, then we can't use the page
                   buffer for this I/O and we need to bypass */
                if(0 == can_make_space) {
                    HDassert(0==i);
                    /* write to VFD and return */
                    if(H5FD_write(f->shared->lf, dxpl, type, addr, size, buf) < 0)
                        HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "driver read request failed");
                    HGOTO_DONE(SUCCEED);
                }
            }

            /* don't bother searching if there is no write access */
            if(H5F_ACC_RDWR & H5F_INTENT(f))
                /* lookup & remove the page from the new skip list page if
                   it exists to see if this is a new page from the MF layer */
                page_entry = (H5PB_entry_t *)H5SL_remove(page_buf->mf_slist_ptr, (void *)(&search_addr));

            /* calculate offset into the buffer of the page and the user buffer */
            offset = (0==i ? addr - search_addr : 0);
            buf_offset = (0==i ? 0 : size-access_size);

            /* if found, then just update the buffer pointer to the newly allocate buffer */
            if(page_entry) {
#if H5PB_COLLECT_STATS
                if(type == H5FD_MEM_DRAW)
                    page_buf->hits[1] ++;
                else
                    page_buf->hits[0] ++;
#endif /* H5PB_COLLECT_STATS */
                //fprintf(stderr, "New page EXISTING at addr %llu, buf ptr %p\n", search_addr, new_page_buf);

                /* allocate space for the page buffer */
                if(NULL == (new_page_buf = H5MM_malloc(page_buf->page_size)))
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTALLOC, FAIL, "memory allocation failed for page buffer entry");
                HDmemset(new_page_buf, 0, (size_t)offset);
                HDmemset((uint8_t *)new_page_buf+offset+access_size , 0, page_size-((size_t)offset+access_size));

                page_entry->page_buf_ptr = new_page_buf;
            }
            /* Otherwise read page through the VFD layer, but make sure we don't read past the EOA. */
            else {
                haddr_t eoa, eof = 0;

                //fprintf(stderr, "New page READ at addr %llu, buf ptr %p\n", search_addr, new_page_buf);

                /* allocate space for the page buffer */
                if(NULL == (new_page_buf = H5MM_calloc(page_size)))
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTALLOC, FAIL, "memory allocation failed for page buffer entry");

                /* create the new loaded PB entry */
                if(NULL == (page_entry = H5FL_CALLOC(H5PB_entry_t)))
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_NOSPACE, FAIL, "memory allocation failed");

                page_entry->page_buf_ptr = new_page_buf;
                page_entry->addr = search_addr;
                page_entry->type = (H5F_mem_page_t)type;

                /* Retrieve the 'eoa' for the file */
                if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, type)))
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "driver get_eoa request failed");
                /* if the entire page falls outside the EOA, then fail */
                if(search_addr > eoa)
                    HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "Writing to a page that is outside the file EOA");

                /* Retrieve the 'eof' for the file - The MPI-VFD EOF
                   returned will most likely be HADDR_UNDEF, so skip
                   that check. */
                if(!H5F_HAS_FEATURE(f, H5FD_FEAT_HAS_MPI))
                    if(HADDR_UNDEF == (eof = H5FD_get_eof(f->shared->lf, H5FD_MEM_DEFAULT)))
                        HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "driver get_eof request failed")

                /* adjust the read size to not go beyond the EOA */
                if(search_addr + page_size > eoa)
                    page_size = (size_t)(eoa - search_addr);

                if(search_addr < eof) {
#if H5PB_COLLECT_STATS
                    if(type == H5FD_MEM_DRAW)
                        page_buf->misses[1] ++;
                    else
                        page_buf->misses[0] ++;
#endif /* H5PB_COLLECT_STATS */

                    if(H5FD_read(f->shared->lf, dxpl, type, search_addr, page_size, new_page_buf) < 0)
                        HGOTO_ERROR(H5E_PAGEBUF, H5E_READERROR, FAIL, "driver read request failed")
                }
            }

            /* copy the requested data from the page into the input buffer */
            //fprintf(stderr, "Memcpy to offset %llu size %zu\n", offset, access_size);
            HDmemcpy((uint8_t *)new_page_buf + offset, (const uint8_t *)buf+buf_offset, access_size);

            page_entry->is_dirty = TRUE;

            /* insert page into PB, evicting other pages as necessary */
            if(H5PB__insert_entry(page_buf, page_entry) < 0)
                HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTSET, FAIL, "error inserting new page in page buffer");
        }
    }

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5PB_write() */


/*-------------------------------------------------------------------------
 * Function:	H5PB__insert_entry()
 *
 * Purpose: ??? 
 *
 *          This function was created without documentation.
 *          What follows is my best understanding of Mohamad's intent.
 *
 *	    Insert the supplied page into the page buffer, both the
 *          skip list and the LRU.
 *
 *          As best I can tell, this function imposes no limit on the
 *          number of entries in the page buffer beyond an assertion
 *          failure it the page count exceeds the limit.
 *
 *                                               JRM -- 12/22/16
 *
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5PB__insert_entry(H5PB_t *page_buf, H5PB_entry_t *page_entry)
{
    herr_t ret_value = SUCCEED;    /* Return value */
    FUNC_ENTER_STATIC

    /* insert entry in skip list */
    if(H5SL_insert(page_buf->slist_ptr, page_entry, &(page_entry->addr)) < 0)
        HGOTO_ERROR(H5E_PAGEBUF, H5E_BADVALUE, FAIL, "Can't insert entry in skip list")
    HDassert(H5SL_count(page_buf->slist_ptr) * page_buf->page_size <= page_buf->max_size);

    if(H5F_MEM_PAGE_DRAW == page_entry->type ||H5F_MEM_PAGE_GHEAP == page_entry->type)
        page_buf->raw_count ++;
    else
        page_buf->meta_count ++;

    /* insert entry in LRU */
    H5PB__INSERT_LRU(page_buf, page_entry)
done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB__insert_entry */


/*-------------------------------------------------------------------------
 * Function:	H5PB__make_space()
 *
 * Purpose: ??? 
 *
 *          This function was created without documentation.
 *          What follows is my best understanding of Mohamad's intent.
 *
 *          If necessary and if possible, evict a page from the page 
 *          buffer to make space for the supplied page.  Depending on 
 *	    the page buffer configuration and contents, and the page 
 *          supplied this may or may not be possible.
 *
 *                                             JRM -- 12/22/16
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static htri_t 
H5PB__make_space(const H5F_t *f, H5PB_t *page_buf, H5P_genplist_t *dxpl, H5FD_mem_t inserted_type)
{
    H5PB_entry_t *page_entry;
    htri_t ret_value = 1;    /* Return value */

    FUNC_ENTER_STATIC

    page_entry = page_buf->LRU_tail_ptr;

    if(H5FD_MEM_DRAW == inserted_type) {
        /* If threshould is 100% metadata and page buffer is full of
           metadata, then we can't make space for raw data */
        if(0 == page_buf->raw_count && page_buf->min_meta_count == page_buf->meta_count) {
            HDassert(page_buf->meta_count*page_buf->page_size == page_buf->max_size);
            HGOTO_DONE(0);
        }

        /* check the metadata threshold before evicting metadata items */
        while (1) {
            if(page_entry->prev && H5F_MEM_PAGE_META == page_entry->type && 
               page_buf->min_meta_count >= page_buf->meta_count)
                page_entry = page_entry->prev;
            else
                break;
        }
    }
    else {
        /* If threshould is 100% raw data and page buffer is full of
           raw data, then we can't make space for meta data */
        if(0 == page_buf->meta_count && page_buf->min_raw_count == page_buf->raw_count) {
            HDassert(page_buf->raw_count*page_buf->page_size == page_buf->max_size);
            HGOTO_DONE(0);
        }

        /* check the raw data threshold before evicting raw data items */
        while (1) {
            if(page_entry->prev && (H5F_MEM_PAGE_DRAW == page_entry->type || H5F_MEM_PAGE_GHEAP == page_entry->type) && 
               page_buf->min_raw_count >= page_buf->raw_count)
                page_entry = page_entry->prev;
            else
                break;
        }
    }

    if(NULL == H5SL_remove(page_buf->slist_ptr, &(page_entry->addr)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_BADVALUE, FAIL, "Tail Page Entry is not in skip list")

    /* remove entry from LRU list */
    H5PB__REMOVE_LRU(page_buf, page_entry)
    HDassert(H5SL_count(page_buf->slist_ptr) == page_buf->LRU_list_len);

    if(H5F_MEM_PAGE_DRAW == page_entry->type || H5F_MEM_PAGE_GHEAP == page_entry->type)
        page_buf->raw_count --;
    else
        page_buf->meta_count --;

    if(page_entry->is_dirty)
        if(H5PB__write_entry(f, page_entry, dxpl) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_WRITEERROR, FAIL, "file write failed")

#if H5PB_COLLECT_STATS
    if(page_entry->type == H5F_MEM_PAGE_DRAW || H5F_MEM_PAGE_GHEAP == page_entry->type)
        page_buf->evictions[1] ++;
    else
        page_buf->evictions[0] ++;
#endif /* H5PB_COLLECT_STATS */

    H5MM_free(page_entry->page_buf_ptr);
    page_entry->page_buf_ptr = NULL;
    page_entry = H5FL_FREE(H5PB_entry_t, page_entry);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB__make_space */


/*-------------------------------------------------------------------------
 * Function:	H5PB__write_entry()
 *
 * Purpose: ??? 
 *
 *          This function was created without documentation.
 *          What follows is my best understanding of Mohamad's intent.
 *
 *
 * Return:	Non-negative on success/Negative on failure
 *
 * Programmer:	Mohamad Chaarawi
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5PB__write_entry(const H5F_t *f, H5PB_entry_t *page_entry, H5P_genplist_t *dxpl)
{
    haddr_t eoa;
    H5FD_mem_t type;
    H5P_genplist_t *my_dxpl = dxpl;
    herr_t ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_STATIC

    HDassert(page_entry);

    type = (H5FD_mem_t)page_entry->type;

#ifdef H5_DEBUG_BUILD
    if(H5FD_MEM_DRAW == type || H5FD_MEM_GHEAP == type) {
        if(NULL == (my_dxpl = (H5P_genplist_t *)H5I_object(H5AC_rawdata_dxpl_id)))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_BADTYPE, FAIL, "can't get property list")
    } else {
        if(NULL == (my_dxpl = (H5P_genplist_t *)H5I_object(H5AC_ind_read_dxpl_id)))
            HGOTO_ERROR(H5E_PAGEBUF, H5E_BADTYPE, FAIL, "can't get property list")
    }
#endif /* H5_DEBUG_BUILD */

    /* if the starting address of the page is larger than
       the EOA, then the entire page is discarded without writing. */

    /* Retrieve the 'eoa' for the file */
    if(HADDR_UNDEF == (eoa = H5F_get_eoa(f, type)))
        HGOTO_ERROR(H5E_PAGEBUF, H5E_CANTGET, FAIL, "driver get_eoa request failed");

    if(page_entry->addr <= eoa) {
        size_t page_size = f->shared->page_buf->page_size;

        /* adjust the page length if it exceeds the EOA */
        if(page_entry->addr + page_size > eoa)
            page_size = (size_t)(eoa - page_entry->addr);

        if(H5FD_write(f->shared->lf, my_dxpl, type, page_entry->addr, 
                      page_size, page_entry->page_buf_ptr) < 0)
            HGOTO_ERROR(H5E_PAGEBUF, H5E_WRITEERROR, FAIL, "file write failed");
    }

    page_entry->is_dirty = FALSE;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* H5PB__write_entry */

