/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
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
 * Purpose:     This is a "pass through" VOL connector, which forwards each
 *              VOL callback to an underlying connector.
 *
 *              It is designed as an example VOL connector for developers to
 *              use when creating new connectors, especially connectors that
 *              are outside of the HDF5 library.  As such, it should _NOT_
 *              include _any_ private HDF5 header files.  This connector should
 *              therefore only make public HDF5 API calls and use standard C /
 *              POSIX calls.
 */


/* Header files needed */
/* (Public HDF5 and standard C / POSIX only) */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>

#include "hdf5.h"
#include "H5VLprovnc.h"

/**********/
/* Macros */
/**********/

/* Whether to display log messge when callback is invoked */
/* (Uncomment to enable) */
/* #define ENABLE_PROVNC_LOGGING */

/* Hack for missing va_copy() in old Visual Studio editions
 * (from H5win2_defs.h - used on VS2012 and earlier)
 */
#if defined(_WIN32) && defined(_MSC_VER) && (_MSC_VER < 1800)
#define va_copy(D,S)      ((D) = (S))
#endif

/************/
/* Typedefs */
/************/

/* The pass through VOL info object */

typedef struct H5VL_provenance_t {
    hid_t  under_vol_id;        /* ID for underlying VOL connector */
    void   *under_object;       /* Info object for underlying VOL connector */
    prov_helper* prov_helper;
    H5VL_provenance_info_t* shared_file_info;
    void* prov_info;
} H5VL_provenance_t;

/* The pass through VOL wrapper context */
typedef struct H5VL_provenance_wrap_ctx_t {
    hid_t under_vol_id;         /* VOL ID for under VOL */
    void *under_wrap_ctx;       /* Object wrapping context for under VOL */
} H5VL_provenance_wrap_ctx_t;

//======================================= statistics =======================================
//typedef struct H5VL_prov_t {
//    void   *under_object;
//    char* func_name;
//    int func_cnt;//stats
//} H5VL_prov_t;

typedef struct H5VL_prov_datatype_info_t {
    char* dtype_name;
    int datatype_commit_cnt;
    int datatype_get_cnt;

} H5VL_prov_datatype_info_t;

typedef struct H5VL_prov_dataset_info_t {
    char* dset_name;
    H5T_class_t dt_class;//data type class
    H5S_class_t ds_class;//data space class
    H5D_layout_t layout;
    unsigned int dimension_cnt;
    unsigned int dimensions[H5S_MAX_RANK];
    size_t dset_type_size;
    hsize_t dset_space_size;//unsigned long long
    hsize_t total_bytes_read;
    hsize_t total_bytes_written;
    hsize_t total_read_time;
    hsize_t total_write_time;
    int dataset_read_cnt;
    int dataset_write_cnt;
    H5VL_provenance_info_t* shared_file_info;

} H5VL_prov_dataset_info_t;

typedef struct H5VL_prov_group_info_t {
    int func_cnt;//stats
    int group_get_cnt;
    int group_specific_cnt;

} H5VL_prov_group_info_t;

//typedef struct H5VL_prov_link_info_t {
//    char* func_name;
//    int func_cnt;//stats
//    int link_get_cnt;
//    int link_specific_cnt;
//
//} H5VL_prov_link_info_t;

typedef struct H5VL_prov_file_info_t {//assigned when a file is closed, serves to store stats (copied from shared_file_info)
    int ds_created;
    int ds_accessed;//ds opened
} H5VL_prov_file_info_t;


//======================================= statistics =======================================

/********************* */
/* Function prototypes */
/********************* */

/* Helper routines */
static herr_t H5VL_provenance_file_specific_reissue(void *obj, hid_t connector_id,
    H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, ...);
static herr_t H5VL_provenance_request_specific_reissue(void *obj, hid_t connector_id,
    H5VL_request_specific_t specific_type, ...);
static H5VL_provenance_t *H5VL_provenance_new_obj(void *under_obj,
    hid_t under_vol_id, prov_helper* helper);
static herr_t H5VL_provenance_free_obj(H5VL_provenance_t *obj);

/* "Management" callbacks */
static herr_t H5VL_provenance_init(hid_t vipl_id);
static herr_t H5VL_provenance_term(void);
static void *H5VL_provenance_info_copy(const void *info);
static herr_t H5VL_provenance_info_cmp(int *cmp_value, const void *info1, const void *info2);
static herr_t H5VL_provenance_info_free(void *info);
static herr_t H5VL_provenance_info_to_str(const void *info, char **str);
static herr_t H5VL_provenance_str_to_info(const char *str, void **info);
static void *H5VL_provenance_get_object(const void *obj);
static herr_t H5VL_provenance_get_wrap_ctx(const void *obj, void **wrap_ctx);
static herr_t H5VL_provenance_free_wrap_ctx(void *obj);
static void *H5VL_provenance_wrap_object(void *obj, void *wrap_ctx);

/* Attribute callbacks */
static void *H5VL_provenance_attr_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req);
static void *H5VL_provenance_attr_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t aapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_attr_read(void *attr, hid_t mem_type_id, void *buf, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_attr_write(void *attr, hid_t mem_type_id, const void *buf, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_attr_get(void *obj, H5VL_attr_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_attr_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_attr_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_attr_close(void *attr, hid_t dxpl_id, void **req);

/* Dataset callbacks */
static void *H5VL_provenance_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void *H5VL_provenance_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                                    hid_t file_space_id, hid_t plist_id, void *buf, void **req);
static herr_t H5VL_provenance_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void *buf, void **req);
static herr_t H5VL_provenance_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_dataset_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_dataset_close(void *dset, hid_t dxpl_id, void **req);

/* Datatype callbacks */
static void *H5VL_provenance_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req);
static void *H5VL_provenance_datatype_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t tapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_datatype_get(void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_datatype_specific(void *obj, H5VL_datatype_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_datatype_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_datatype_close(void *dt, hid_t dxpl_id, void **req);

/* File callbacks */
static void *H5VL_provenance_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
static void *H5VL_provenance_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_file_specific(void *file, H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_file_optional(void *file, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_file_close(void *file, hid_t dxpl_id, void **req);

/* Group callbacks */
static void *H5VL_provenance_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req);
static void *H5VL_provenance_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t gapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_group_get(void *obj, H5VL_group_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_group_specific(void *obj, H5VL_group_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_group_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_group_close(void *grp, hid_t dxpl_id, void **req);

/* Link callbacks */
static herr_t H5VL_provenance_link_create(H5VL_link_create_type_t create_type, void *obj, const H5VL_loc_params_t *loc_params, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_link_move(void *src_obj, const H5VL_loc_params_t *loc_params1, void *dst_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_link_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_link_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_link_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_link_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);

/* Object callbacks */
static void *H5VL_provenance_object_open(void *obj, const H5VL_loc_params_t *loc_params, H5I_type_t *opened_type, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params, const char *src_name, void *dst_obj, const H5VL_loc_params_t *dst_loc_params, const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_provenance_object_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_object_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_provenance_object_optional(void *obj, hid_t dxpl_id, void **req, va_list arguments);

/* Async request callbacks */
static herr_t H5VL_provenance_request_wait(void *req, uint64_t timeout, H5ES_status_t *status);
static herr_t H5VL_provenance_request_notify(void *obj, H5VL_request_notify_t cb, void *ctx);
static herr_t H5VL_provenance_request_cancel(void *req);
static herr_t H5VL_provenance_request_specific(void *req, H5VL_request_specific_t specific_type, va_list arguments);
static herr_t H5VL_provenance_request_optional(void *req, va_list arguments);
static herr_t H5VL_provenance_request_free(void *req);

/*******************/
/* Local variables */
/*******************/

/* Pass through VOL connector class struct */
static const H5VL_class_t H5VL_provenance_cls = {
    H5VL_PROVNC_VERSION,                          /* version      */
    (H5VL_class_value_t)H5VL_PROVNC_VALUE,        /* value        */
    H5VL_PROVNC_NAME,                             /* name         */
    0,                                              /* capability flags */
    H5VL_provenance_init,                         /* initialize   */
    H5VL_provenance_term,                         /* terminate    */
    sizeof(H5VL_provenance_info_t),               /* info size    */
    H5VL_provenance_info_copy,                    /* info copy    */
    H5VL_provenance_info_cmp,                     /* info compare */
    H5VL_provenance_info_free,                    /* info free    */
    H5VL_provenance_info_to_str,                  /* info to str  */
    H5VL_provenance_str_to_info,                  /* str to info  */
    H5VL_provenance_get_object,                   /* get_object   */
    H5VL_provenance_get_wrap_ctx,                 /* get_wrap_ctx */
    H5VL_provenance_wrap_object,                  /* wrap_object  */
    H5VL_provenance_free_wrap_ctx,                /* free_wrap_ctx */
    {                                           /* attribute_cls */
//        H5VL_provenance_attr_create,                       /* create */
//        H5VL_provenance_attr_open,                         /* open */
//        H5VL_provenance_attr_read,                         /* read */
//        H5VL_provenance_attr_write,                        /* write */
//        H5VL_provenance_attr_get,                          /* get */
//        H5VL_provenance_attr_specific,                     /* specific */
//        H5VL_provenance_attr_optional,                     /* optional */
//        H5VL_provenance_attr_close                         /* close */
    },
    {                                           /* dataset_cls */
        H5VL_provenance_dataset_create,                    /* create */
        H5VL_provenance_dataset_open,                      /* open */
        H5VL_provenance_dataset_read,                      /* read */
        H5VL_provenance_dataset_write,                     /* write */
        H5VL_provenance_dataset_get,                       /* get */
        H5VL_provenance_dataset_specific,                  /* specific */
        H5VL_provenance_dataset_optional,                  /* optional */
        H5VL_provenance_dataset_close                      /* close */
    },
    {                                               /* datatype_cls */
        H5VL_provenance_datatype_commit,                   /* commit */
        H5VL_provenance_datatype_open,                     /* open */
        H5VL_provenance_datatype_get,                      /* get_size */
        H5VL_provenance_datatype_specific,                 /* specific */
        H5VL_provenance_datatype_optional,                 /* optional */
        H5VL_provenance_datatype_close                     /* close */
    },
    {                                           /* file_cls */
        H5VL_provenance_file_create,                       /* create */
        H5VL_provenance_file_open,                         /* open */
        H5VL_provenance_file_get,                          /* get */
        H5VL_provenance_file_specific,                     /* specific */
        H5VL_provenance_file_optional,                     /* optional */
        H5VL_provenance_file_close                         /* close */
    },
    {                                           /* group_cls */
        H5VL_provenance_group_create,                      /* create */
        H5VL_provenance_group_open,                        /* open */
        H5VL_provenance_group_get,                         /* get */
        H5VL_provenance_group_specific,                    /* specific */
        H5VL_provenance_group_optional,                    /* optional */
        H5VL_provenance_group_close                        /* close */
    },
    {                                           /* link_cls */
        H5VL_provenance_link_create,                       /* create */
        H5VL_provenance_link_copy,                         /* copy */
        H5VL_provenance_link_move,                         /* move */
        H5VL_provenance_link_get,                          /* get */
        H5VL_provenance_link_specific,                     /* specific */
        H5VL_provenance_link_optional,                     /* optional */
    },
    {                                           /* object_cls */
        H5VL_provenance_object_open,                       /* open */
        H5VL_provenance_object_copy,                       /* copy */
        H5VL_provenance_object_get,                        /* get */
        H5VL_provenance_object_specific,                   /* specific */
        H5VL_provenance_object_optional,                   /* optional */
    },
    {                                           /* request_cls */
        H5VL_provenance_request_wait,                      /* wait */
        H5VL_provenance_request_notify,                    /* notify */
        H5VL_provenance_request_cancel,                    /* cancel */
        H5VL_provenance_request_specific,                  /* specific */
        H5VL_provenance_request_optional,                  /* optional */
        H5VL_provenance_request_free                       /* free */
    },
    NULL                                        /* optional */
};

/* The connector identification number, initialized at runtime */
static hid_t prov_connector_id_global = H5I_INVALID_HID;


H5VL_prov_datatype_info_t* new_datatype_info(void){
    H5VL_prov_datatype_info_t* info = calloc(1, sizeof(H5VL_prov_datatype_info_t));
    return info;
}

H5VL_prov_dataset_info_t * new_dataset_info(void){
    H5VL_prov_dataset_info_t* info = calloc(1, sizeof(H5VL_prov_dataset_info_t));
    return info;
}

H5VL_prov_group_info_t * new_group_info(void){
    H5VL_prov_group_info_t* info = calloc(1, sizeof(H5VL_prov_group_info_t));
    return info;
}

H5VL_prov_file_info_t* new_file_info(void) {
    H5VL_prov_file_info_t* info = calloc(1, sizeof(H5VL_prov_file_info_t));
    return info;
}

void dataset_release_info(H5VL_prov_dataset_info_t* info){
    if(info->dset_name)
        free(info->dset_name);
    free(info);
}

void dataset_stats_prov_write(H5VL_prov_dataset_info_t* ds_info){
    if(!ds_info)
        printf("dataset_stats_prov_write(): ds_info is NULL.\n");

    printf("Dataset name = %s,\ndata type class = %d, data space class = %d, data space size = %llu, data type size =%llu.\n", ds_info->dset_name, ds_info->dt_class, ds_info->ds_class,  ds_info->dset_space_size, ds_info->dset_type_size);
    printf("Dataset is %d dimensions.\n", ds_info->dimension_cnt);
    printf("Dataset is read %d time, %llu bytes in total, costs %llu us.\n", ds_info->dataset_read_cnt, ds_info->total_bytes_read, ds_info->total_read_time);
    printf("Dataset is written %d time, %llu bytes in total, costs %llu us.\n", ds_info->dataset_write_cnt, ds_info->total_bytes_written, ds_info->total_write_time);


}

//not H5VL_prov_file_info_t!
void file_stats_prov_write(H5VL_provenance_info_t* file_info) {
    if(!file_info)
        printf("file_stats_prov_write(): ds_info is NULL.\n");

    printf("H5 file closed, %d datasets are created, %d datasets are accessed.\n", file_info->ds_created, file_info->ds_accessed);

}

void datatype_stats_prov_write(H5VL_prov_datatype_info_t* dt_info) {
    if(!dt_info)
        printf("datatype_stats_prov_write(): ds_info is NULL.\n");
    printf("Datatype name = %s, commited %d times, datatype get is called %d times.\n", dt_info->dtype_name, dt_info->datatype_commit_cnt, dt_info->datatype_get_cnt);
}

void group_stats_prov_write(H5VL_prov_group_info_t* grp_info) {
    printf("group_stats_prov_write() is yet to be implemented.\n");
}

prov_helper* prov_helper_init( char* file_path, Prov_level prov_level, char* prov_line_format){
    prov_helper* new_helper = calloc(1, sizeof(prov_helper));
    //printf("%s:%d, file_path = %s\n", __func__, __LINE__, file_path);
    if(prov_level >= 2) {//write to file
        if(!file_path){
            printf("prov_helper_init() failed, provenance file path is not set.\n");
            return NULL;
        }
    }

    new_helper->prov_file_path = strdup(file_path);

    new_helper->prov_line_format = strdup(prov_line_format);

    new_helper->prov_level = prov_level;

    new_helper->pid = getpid();

    //VOL_DRIVER_ID = H5VLregister_connector_by_value((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
            //H5VLget_driver_id("provenance");
    //assert(VOL_CONNECTOR_ID > 0);
    //pthread_id_np_t   tid;

    //pid_t tid = gettid(); Linux

    pthread_threadid_np(NULL, &(new_helper->tid));//OSX only

    getlogin_r(new_helper->user_name, 32);

    if(new_helper->prov_level == File_only | new_helper->prov_level == File_and_print){
        new_helper->prov_file_handle = fopen(new_helper->prov_file_path, "a");
    }
    return new_helper;
}

void prov_helper_teardown(prov_helper* helper){
    if(helper){// not null
        if(helper->prov_level == File_only | helper->prov_level ==File_and_print){//no file
            fflush(helper->prov_file_handle);
            fclose(helper->prov_file_handle);
        }
        free(helper);
    }
}

void file_ds_created(H5VL_provenance_info_t* info){
    assert(info);
    info->ds_created++;
}

void file_ds_accessed(H5VL_provenance_info_t* info){
    assert(info);
    info->ds_accessed++;
}

void ptr_cnt_increment(prov_helper* helper){
    assert(helper);
    //mutex lock
    helper->ptr_cnt++;
    //mutex unlock
}

void ptr_cnt_decrement(prov_helper* helper){
    assert(helper);
    assert(helper->ptr_cnt);
    //mutex lock
    helper->ptr_cnt--;
    //mutex unlock
    if(helper->ptr_cnt == 0)
        prov_helper_teardown(helper);
}


void get_time_str(char *str_out){
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(str_out, "%d/%d/%d %d:%d:%d", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

unsigned long get_time_usec() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return 1000000 * tp.tv_sec + tp.tv_usec;
}

void dataset_get_wrapper(void *dset, hid_t driver_id, H5VL_dataset_get_t get_type,
        hid_t dxpl_id, void **req, ...){
    va_list args;
    va_start(args, req);
    H5VLdataset_get(dset, driver_id, get_type, dxpl_id, req, args);
}

int prov_write(prov_helper* helper_in, const char* msg, unsigned long duration){
    char time[64];
    get_time_str(time);
    char pline[512];

    /* Trimming long VOL function names */
    char* base = "H5VL_provenance_";
    int base_len = strlen(base);
    int msg_len = strlen(msg);
    int trim_offset = 0;
    if(msg_len > base_len) {//strlen(H5VL_provenance_) == 16.

        int i = 0;
        for(; i < base_len; i++){
            if(base[i] != msg[i])
                break;
        }
        if(i == base_len) {// do trimming
            trim_offset = base_len;
        }
    }

    sprintf(pline, "[%s][User:%s][PID:%d][TID:%llu][Func:%s][%luus]\n", time, helper_in->user_name, helper_in->pid, helper_in->tid, msg + base_len, duration);

    switch(helper_in->prov_level){
        case File_only:
            fputs(pline, helper_in->prov_file_handle);
            break;

        case File_and_print:
            fputs(pline, helper_in->prov_file_handle);
            printf("%s", pline);
            break;

        case Print_only:
            printf("%s", pline);
            break;

        default:
            break;
    }

    if(helper_in->prov_level == (File_only | File_and_print)){
        fputs(pline, helper_in->prov_file_handle);
    }

    return 0;
}

//herr_t H5VL_prov_init(hid_t vipl_id){
//    herr_t ret_value = 1;
//
//    if (H5VLis_connector_registered("provenance") != 1)
//        prov_connector_id_global = H5VLregister_connector(&H5VL_provenance_cls, vipl_id);
//    else
//        prov_connector_id_global = H5VLget_connector_id("provenance");
//
//    if (prov_connector_id_global <= 0) {
//        ret_value = -1;
//        fprintf(stderr, "  [ASYNC VOL ERROR] with H5VLasync_init\n");
//        goto done;
//    }
//
//
//
//#ifdef ENABLE_DBG_MSG
//    fprintf(stderr,"  [ASYNC VOL DBG] Success with async vol registration\n");
//#endif
//
//done:
//    return ret_value;
//
//}


/*-------------------------------------------------------------------------
 * Function:    H5VL__provenance_new_obj
 *
 * Purpose:     Create a new pass through object for an underlying object
 *
 * Return:      Success:    Pointer to the new pass through object
 *              Failure:    NULL
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
static H5VL_provenance_t *
H5VL_provenance_new_obj(void *under_obj, hid_t under_vol_id, prov_helper* helper)
{

    H5VL_provenance_t *new_obj;

    new_obj = (H5VL_provenance_t *)calloc(1, sizeof(H5VL_provenance_t));
    new_obj->under_object = under_obj;
    new_obj->under_vol_id = under_vol_id;

    new_obj->prov_helper = helper;
    ptr_cnt_increment(helper);

    H5Iinc_ref(new_obj->under_vol_id);


    return new_obj;
} /* end H5VL__provenance_new_obj() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__provenance_free_obj
 *
 * Purpose:     Release a pass through object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Monday, December 3, 2018
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_free_obj(H5VL_provenance_t *obj)
{
    ptr_cnt_decrement(obj->prov_helper);
    obj->prov_helper = NULL;
    H5Idec_ref(obj->under_vol_id);
    free(obj);

    return 0;
} /* end H5VL__provenance_free_obj() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_register
 *
 * Purpose:     Register the pass-through VOL connector and retrieve an ID
 *              for it.
 *
 * Return:      Success:    The ID for the pass-through VOL connector
 *              Failure:    -1
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 28, 2018
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5VL_provenance_register(void)
{
    /* Clear the error stack */
    H5Eclear2(H5E_DEFAULT);

    /* Singleton register the pass-through VOL connector ID */
    if(H5I_VOL != H5Iget_type(prov_connector_id_global))
        prov_connector_id_global = H5VLregister_connector(&H5VL_provenance_cls, H5P_DEFAULT);

    return prov_connector_id_global;
} /* end H5VL_provenance_register() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_init
 *
 * Purpose:     Initialize this VOL connector, performing any necessary
 *              operations for the connector that will apply to all containers
 *              accessed with the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_init(hid_t vipl_id)
{

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INIT\n");
#endif

    /* Shut compiler up about unused parameter */
    vipl_id = vipl_id;

    return 0;
} /* end H5VL_provenance_init() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_term
 *
 * Purpose:     Terminate this VOL connector, performing any necessary
 *              operations for the connector that release connector-wide
 *              resources (usually created / initialized with the 'init'
 *              callback).
 *
 * Return:      Success:    0
 *              Failure:    (Can't fail)
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_term(void)
{

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL TERM\n");
#endif

    /* Reset VOL ID */
    prov_connector_id_global = H5I_INVALID_HID;

    return 0;
} /* end H5VL_provenance_term() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_info_copy
 *
 * Purpose:     Duplicate the connector's info object.
 *
 * Returns:     Success:    New connector info object
 *              Failure:    NULL
 *
 *---------------------------------------------------------------------------
 */
static void *
H5VL_provenance_info_copy(const void *_info)
{
    const H5VL_provenance_info_t *info = (const H5VL_provenance_info_t *)_info;
    H5VL_provenance_info_t *new_info;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INFO Copy\n");
#endif

    /* Allocate new VOL info struct for the pass through connector */
    new_info = (H5VL_provenance_info_t *)calloc(1, sizeof(H5VL_provenance_info_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_info->under_vol_id = info->under_vol_id;

    if(info->prov_file_path)
        new_info->prov_file_path = strdup(info->prov_file_path);
    if(info->prov_line_format)
        new_info->prov_line_format = strdup(info->prov_line_format);

    new_info->prov_level = info->prov_level;

    H5Iinc_ref(new_info->under_vol_id);
    if(info->under_vol_info)
        H5VLcopy_connector_info(new_info->under_vol_id, &(new_info->under_vol_info), info->under_vol_info);

    return new_info;
} /* end H5VL_provenance_info_copy() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_info_cmp
 *
 * Purpose:     Compare two of the connector's info objects, setting *cmp_value,
 *              following the same rules as strcmp().
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_info_cmp(int *cmp_value, const void *_info1, const void *_info2)
{
    const H5VL_provenance_info_t *info1 = (const H5VL_provenance_info_t *)_info1;
    const H5VL_provenance_info_t *info2 = (const H5VL_provenance_info_t *)_info2;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INFO Compare\n");
#endif

    /* Sanity checks */
    assert(info1);
    assert(info2);

    /* Initialize comparison value */
    *cmp_value = 0;
    
    /* Compare under VOL connector classes */
    H5VLcmp_connector_cls(cmp_value, info1->under_vol_id, info2->under_vol_id);
    if(*cmp_value != 0){
        return 0;
    }

    /* Compare under VOL connector info objects */
    H5VLcmp_connector_info(cmp_value, info1->under_vol_id, info1->under_vol_info, info2->under_vol_info);
    if(*cmp_value != 0){
        return 0;
    }

    *cmp_value = strcmp(info1->prov_file_path, info2->prov_file_path);
    if(*cmp_value != 0){
        return 0;
    }

    *cmp_value = strcmp(info1->prov_line_format, info2->prov_line_format);
    if(*cmp_value != 0){
        return 0;
    }
    *cmp_value = info1->prov_level - info2->prov_level;
    if(*cmp_value != 0){
        return 0;
    }

    return 0;
} /* end H5VL_provenance_info_cmp() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_info_free
 *
 * Purpose:     Release an info object for the connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_info_free(void *_info)
{
    H5VL_provenance_info_t *info = (H5VL_provenance_info_t *)_info;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INFO Free\n");
#endif

    /* Release underlying VOL ID and info */
    if(info->under_vol_info)
        H5VLfree_connector_info(info->under_vol_id, info->under_vol_info);

    H5Idec_ref(info->under_vol_id);

    if(info->prov_file_path)
        free(info->prov_file_path);

    if(info->prov_line_format)
        free(info->prov_line_format);
    /* Free pass through info object itself */
    free(info);

    return 0;
} /* end H5VL_provenance_info_free() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_info_to_str
 *
 * Purpose:     Serialize an info object for this connector into a string
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_info_to_str(const void *_info, char **str)
{
    const H5VL_provenance_info_t *info = (const H5VL_provenance_info_t *)_info;
    H5VL_class_value_t under_value = (H5VL_class_value_t)-1;
    char *under_vol_string = NULL;
    size_t under_vol_str_len = 0;
    size_t path_len = 0;
    size_t format_len = 0;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INFO To String\n");
#endif

    /* Get value and string for underlying VOL connector */
    H5VLget_value(info->under_vol_id, &under_value);
    H5VLconnector_info_to_str(info->under_vol_info, info->under_vol_id, &under_vol_string);

    /* Determine length of underlying VOL info string */
    if(under_vol_string)
        under_vol_str_len = strlen(under_vol_string);


    if(info->prov_file_path)
        path_len = strlen(info->prov_file_path);

    if(info->prov_line_format)
        format_len = strlen(info->prov_line_format);

    /* Allocate space for our info */
    *str = (char *)H5allocate_memory(64 + under_vol_str_len + path_len + format_len, (hbool_t)0);
    assert(*str);

    /* Encode our info
     * Normally we'd use snprintf() here for a little extra safety, but that
     * call had problems on Windows until recently. So, to be as platform-independent
     * as we can, we're using sprintf() instead.
     */
    sprintf(*str, "under_vol=%u;under_info={%s};path=%s;level=%d;format=%s",
            (unsigned)under_value, (under_vol_string ? under_vol_string : ""), info->prov_file_path, info->prov_level, info->prov_line_format);

    return 0;
} /* end H5VL_provenance_info_to_str() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_str_to_info
 *
 * Purpose:     Deserialize a string into an info object for this connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
herr_t provenance_file_setup(char* str_in, char* file_path_out, int* level_out, char* format_out){
    //acceptable format: path=$path_str;level=$level_int;format=$format_str
    char tmp_str[100] = {'\0'};
    memcpy(tmp_str, str_in, strlen(str_in)+1);
    //int ret = sscanf(tmp_str, "};path=%s level=%d format=%s", file_path_out, level_out, format_out);
    char* toklist[4] = {NULL};
    int i = 0;
        char *p;
        p = strtok(tmp_str, ";");
        while(p != NULL) {
            //printf("%s\n", p);
            toklist[i] = strdup(p);
            //printf("toklist[%d] = %s\n", i, toklist[i]);
            p = strtok(NULL, ";");
            i++;
        }

        sscanf(toklist[1], "path=%s", file_path_out);

        sscanf(toklist[2], "level=%d", level_out);

        sscanf(toklist[3], "format=%s", format_out);

        for(int i = 0; i<3; i++){
            if(toklist[i]){
                free(toklist[i]);
            }
        }

    //printf("Parsing result: sample_str = %s,\n path = %s,\n level = %d,\n format =%s.\n", str_in, file_path_out, *level_out, format_out);

    return 0;
}

static herr_t
H5VL_provenance_str_to_info(const char *str, void **_info)
{
    H5VL_provenance_info_t *info;
    unsigned under_vol_value;
    const char *under_vol_info_start, *under_vol_info_end;
    hid_t under_vol_id;
    void *under_vol_info = NULL;

    char* prov_file_path;//assign from parsed str
    Prov_level level;
    char* prov_format;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL INFO String To Info\n");
#endif

    /* Retrieve the underlying VOL connector value and info */
    sscanf(str, "under_vol=%u;", &under_vol_value);
    under_vol_id = H5VLregister_connector_by_value((H5VL_class_value_t)under_vol_value, H5P_DEFAULT);
    under_vol_info_start = strchr(str, '{');
    under_vol_info_end = strrchr(str, '}');
    assert(under_vol_info_end > under_vol_info_start);
    char *under_vol_info_str = NULL;
    if(under_vol_info_end != (under_vol_info_start + 1)) {
        under_vol_info_str = (char *)malloc((size_t)(under_vol_info_end - under_vol_info_start));
        memcpy(under_vol_info_str, under_vol_info_start + 1, (size_t)((under_vol_info_end - under_vol_info_start) - 1));
        *(under_vol_info_str + (under_vol_info_end - under_vol_info_start)) = '\0';

        H5VLconnector_str_to_info(under_vol_info_str, under_vol_id, &under_vol_info);//generate under_vol_info obj.

    } /* end else */

    /* Allocate new pass-through VOL connector info and set its fields */
    info = (H5VL_provenance_info_t *)calloc(1, sizeof(H5VL_provenance_info_t));
    info->under_vol_id = under_vol_id;
    info->under_vol_info = under_vol_info;

    info->prov_file_path = calloc(64, sizeof(char));
    info->prov_line_format = calloc(64, sizeof(char));

    if(provenance_file_setup(under_vol_info_end, info->prov_file_path, &(info->prov_level), info->prov_line_format) != 0){
        free(info->prov_file_path);
        free(info->prov_line_format);
        info->prov_line_format = NULL;
        info->prov_file_path = NULL;
        info->prov_level = -1;
    }
    //info->prov_level = tmp;
    //printf("After provenance_file_setup(): sample_str = %s,\n path = %s,\n level = %d,\n format =%s.\n", under_vol_info_end, info->prov_file_path, info->prov_level, info->prov_line_format);
    /* Set return value */
    *_info = info;

    if(under_vol_info_str)
        free(under_vol_info_str);
    return 0;
} /* end H5VL_provenance_str_to_info() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_get_object
 *
 * Purpose:     Retrieve the 'data' for a VOL object.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static void *
H5VL_provenance_get_object(const void *obj)
{
    const H5VL_provenance_t *o = (const H5VL_provenance_t *)obj;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL Get object\n");
#endif
    if(!o)
        printf("H5VL_provenance_get_object() get a NULL obj as a parameter.");
    void* ret = H5VLget_object(o->under_object, o->under_vol_id);

    return ret;

} /* end H5VL_provenance_get_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_get_wrap_ctx
 *
 * Purpose:     Retrieve a "wrapper context" for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_get_wrap_ctx(const void *obj, void **wrap_ctx)
{
    const H5VL_provenance_t *o = (const H5VL_provenance_t *)obj;
    H5VL_provenance_wrap_ctx_t *new_wrap_ctx;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL WRAP CTX Get\n");
#endif

    /* Allocate new VOL object wrapping context for the pass through connector */
    new_wrap_ctx = (H5VL_provenance_wrap_ctx_t *)calloc(1, sizeof(H5VL_provenance_wrap_ctx_t));

    /* Increment reference count on underlying VOL ID, and copy the VOL info */
    new_wrap_ctx->under_vol_id = o->under_vol_id;
    H5Iinc_ref(new_wrap_ctx->under_vol_id);
    H5VLget_wrap_ctx(o->under_object, o->under_vol_id, &new_wrap_ctx->under_wrap_ctx);

    /* Set wrap context to return */
    *wrap_ctx = new_wrap_ctx;

    return 0;
} /* end H5VL_provenance_get_wrap_ctx() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_wrap_object
 *
 * Purpose:     Use a "wrapper context" to wrap a data object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static void *
H5VL_provenance_wrap_object(void *obj, void *_wrap_ctx)
{
    H5VL_provenance_wrap_ctx_t *wrap_ctx = (H5VL_provenance_wrap_ctx_t *)_wrap_ctx;
    H5VL_provenance_t *new_obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL WRAP Object\n");
#endif

    /* Wrap the object with the underlying VOL */
    under = H5VLwrap_object(obj, wrap_ctx->under_vol_id, wrap_ctx->under_wrap_ctx);
    if(under)
        new_obj = H5VL_provenance_new_obj(under, wrap_ctx->under_vol_id, ((H5VL_provenance_t *)obj)->prov_helper);
    else
        new_obj = NULL;

    return new_obj;
} /* end H5VL_provenance_wrap_object() */


/*---------------------------------------------------------------------------
 * Function:    H5VL_provenance_free_wrap_ctx
 *
 * Purpose:     Release a "wrapper context" for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *---------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_free_wrap_ctx(void *_wrap_ctx)
{
    H5VL_provenance_wrap_ctx_t *wrap_ctx = (H5VL_provenance_wrap_ctx_t *)_wrap_ctx;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL WRAP CTX Free\n");
#endif

    /* Release underlying VOL ID and wrap context */
    if(wrap_ctx->under_wrap_ctx)
        H5VLfree_wrap_ctx(wrap_ctx->under_wrap_ctx, wrap_ctx->under_vol_id);
    H5Idec_ref(wrap_ctx->under_vol_id);

    /* Free pass through wrap context object itself */
    free(wrap_ctx);

    return 0;
} /* end H5VL_provenance_free_wrap_ctx() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_create
 *
 * Purpose:     Creates an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_attr_create(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *attr;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Create\n");
#endif

    under = H5VLattr_create(o->under_object, loc_params, o->under_vol_id, name, acpl_id, aapl_id, dxpl_id, req);
    if(under) {
        attr = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        attr = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void*)attr;
} /* end H5VL_provenance_attr_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_open
 *
 * Purpose:     Opens an attribute on an object.
 *
 * Return:      Success:    Pointer to attribute object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_attr_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t aapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *attr;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Open\n");
#endif

    under = H5VLattr_open(o->under_object, loc_params, o->under_vol_id, name, aapl_id, dxpl_id, req);
    if(under) {
        attr = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        attr = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void *)attr;
} /* end H5VL_provenance_attr_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_read
 *
 * Purpose:     Reads data from attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_read(void *attr, hid_t mem_type_id, void *buf,
    hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)attr;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Read\n");
#endif

    ret_value = H5VLattr_read(o->under_object, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_attr_read() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_write
 *
 * Purpose:     Writes data to attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_write(void *attr, hid_t mem_type_id, const void *buf,
    hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)attr;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Write\n");
#endif

    ret_value = H5VLattr_write(o->under_object, o->under_vol_id, mem_type_id, buf, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_attr_write() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_get
 *
 * Purpose:     Gets information about an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_get(void *obj, H5VL_attr_get_t get_type, hid_t dxpl_id,
    void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Get\n");
#endif

    ret_value = H5VLattr_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_attr_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_specific
 *
 * Purpose:     Specific operation on attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_specific(void *obj, const H5VL_loc_params_t *loc_params,
    H5VL_attr_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Specific\n");
#endif

    ret_value = H5VLattr_specific(o->under_object, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_attr_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_optional
 *
 * Purpose:     Perform a connector-specific operation on an attribute
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Optional\n");
#endif

    ret_value = H5VLattr_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_attr_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_attr_close
 *
 * Purpose:     Closes an attribute.
 *
 * Return:      Success:    0
 *              Failure:    -1, attr not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_attr_close(void *attr, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)attr;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL ATTRIBUTE Close\n");
#endif

    ret_value = H5VLattr_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    /* Release our wrapper, if underlying attribute was closed */
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);


    return ret_value;
} /* end H5VL_provenance_attr_close() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_dataset_create(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) 
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *dset;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Create\n");
#endif
    hid_t dt_id = -1;
    hid_t ds_id = -1;
    under = H5VLdataset_create(o->under_object, loc_params, o->under_vol_id, name, dcpl_id,  dapl_id, dxpl_id, req);
    if(under) {
        dset = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);
        dset->prov_info = new_dataset_info();
        //dset->shared_file_info = o->shared_file_info;
        ((H5VL_prov_dataset_info_t*)dset->prov_info)->dset_name = strdup(name);

        file_ds_created(o->shared_file_info);

        H5Pget(dcpl_id, H5VL_PROP_DSET_TYPE_ID, &dt_id);

        ((H5VL_prov_dataset_info_t*)dset->prov_info)->dt_class = H5Tget_class(dt_id);

        H5Pget(dcpl_id, H5VL_PROP_DSET_SPACE_ID, &ds_id);

        ((H5VL_prov_dataset_info_t*)dset->prov_info)->ds_class = H5Sget_simple_extent_type(ds_id);

        if(((H5VL_prov_dataset_info_t*)dset->prov_info)->ds_class == H5S_SIMPLE){

            ((H5VL_prov_dataset_info_t*)dset->prov_info)->dimension_cnt = H5Sget_simple_extent_ndims(ds_id);

            H5Sget_simple_extent_dims(ds_id, ((H5VL_prov_dataset_info_t*)dset->prov_info)->dimensions, NULL);

            ((H5VL_prov_dataset_info_t*)dset->prov_info)->dset_space_size = H5Sget_simple_extent_npoints(ds_id);

        }

        ((H5VL_prov_dataset_info_t*)dset->prov_info)->layout = H5Pget_layout(dcpl_id);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        dset = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);

    if(dt_id != -1)
        //H5Tclose(dt_id);

    if(ds_id != -1)
        //H5Sclose(ds_id);

    return (void *)dset;
} /* end H5VL_provenance_dataset_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t dapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *dset;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;


#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Open\n");
#endif

    hid_t dcpl_id = -1;
    hid_t dt_id = -1;
    hid_t ds_id = -1;
    under = H5VLdataset_open(o->under_object, loc_params, o->under_vol_id, name, dapl_id, dxpl_id, req);
    if (under) {
        dset = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);
        dset->prov_info = new_dataset_info();
        file_ds_accessed(o->shared_file_info);
        /* Check for async request */
        if (req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

        dataset_get_wrapper(dset->under_object, dset->under_vol_id, H5VL_DATASET_GET_DCPL, dxpl_id, req, &dcpl_id);

        ((H5VL_prov_dataset_info_t*) dset->prov_info)->dset_name = strdup(name);

        dataset_get_wrapper(dset->under_object, dset->under_vol_id, H5VL_DATASET_GET_TYPE, dxpl_id, req, &dt_id);

        ((H5VL_prov_dataset_info_t*) dset->prov_info)->dt_class = H5Tget_class(dt_id);
        ((H5VL_prov_dataset_info_t*) dset->prov_info)->dset_type_size = H5Tget_size(dt_id);
        ((H5VL_prov_dataset_info_t*) dset->prov_info)->dset_name = strdup(name);

        dataset_get_wrapper(dset->under_object, dset->under_vol_id, H5VL_DATASET_GET_SPACE, dxpl_id, req, &ds_id);

        ((H5VL_prov_dataset_info_t*) dset->prov_info)->ds_class = H5Sget_simple_extent_type(ds_id);

        if (((H5VL_prov_dataset_info_t*) dset->prov_info)->ds_class == H5S_SIMPLE) {
            ((H5VL_prov_dataset_info_t*) dset->prov_info)->dimension_cnt = H5Sget_simple_extent_ndims(ds_id);
            H5Sget_simple_extent_dims(ds_id, ((H5VL_prov_dataset_info_t*) dset->prov_info)->dimensions, NULL);
            ((H5VL_prov_dataset_info_t*) dset->prov_info)->dset_space_size = H5Sget_simple_extent_npoints(ds_id);
        }

        ((H5VL_prov_dataset_info_t*) dset->prov_info)->layout = H5Pget_layout(dcpl_id);

    } /* end if */
    else
        dset = NULL;

    prov_write(dset->prov_helper, __func__, get_time_usec() - start);

    if(dt_id != -1)
        H5Tclose(dt_id);

    if(ds_id != -1)
        H5Sclose(ds_id);


    return (void *)dset;
} /* end H5VL_provenance_dataset_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, void *buf, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Read\n");
#endif

    ret_value = H5VLdataset_read(o->under_object, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    hid_t dset_type;
    hid_t dset_space;
    hsize_t r_size = 0;
    unsigned long time = get_time_usec() - start;
    if(H5S_ALL == mem_space_id){
        r_size = ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_type_size * ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_space_size;;
    }else{
        r_size = ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_type_size * H5Sget_select_npoints(mem_space_id);
    }

    ((H5VL_prov_dataset_info_t*)(o->prov_info))->total_bytes_read += r_size;
    ((H5VL_prov_dataset_info_t*)(o->prov_info))->dataset_read_cnt++;
    ((H5VL_prov_dataset_info_t*)(o->prov_info))->total_read_time += time;

    printf("read size = %llu\n", r_size);

    prov_write(o->prov_helper, __func__, time);
    return ret_value;
} /* end H5VL_provenance_dataset_read() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, const void *buf, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Write\n");
#endif

    ret_value = H5VLdataset_write(o->under_object, o->under_vol_id, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    hid_t dset_type;
    hid_t dset_space;
    hsize_t w_size = 0;

    if(H5S_ALL == mem_space_id){
        w_size = ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_type_size * ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_space_size;;
    }else{
        w_size = ((H5VL_prov_dataset_info_t*)o->prov_info)->dset_type_size * H5Sget_select_npoints(mem_space_id);
    }
    unsigned long time = get_time_usec() - start;
    prov_write(o->prov_helper, __func__, time);
    ((H5VL_prov_dataset_info_t*)o->prov_info)->total_bytes_written += w_size;
    ((H5VL_prov_dataset_info_t*)(o->prov_info))->dataset_write_cnt++;
    ((H5VL_prov_dataset_info_t*)(o->prov_info))->total_write_time += time;
    printf("write size = %llu\n", w_size);

    return ret_value;
} /* end H5VL_provenance_dataset_write() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_get(void *dset, H5VL_dataset_get_t get_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Get\n");
#endif

    ret_value = H5VLdataset_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_dataset_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL H5Dspecific\n");
#endif

    ret_value = H5VLdataset_specific(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    if(specific_type == H5VL_DATASET_SET_EXTENT){//user changes the dimensions
        //TODO: update dimensions data for stats
    }

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_dataset_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_optional
 *
 * Purpose:     Perform a connector-specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Optional\n");
#endif

    ret_value = H5VLdataset_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_dataset_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_dataset_close(void *dset, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATASET Close\n");
#endif



    ret_value = H5VLdataset_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    /* Release our wrapper, if underlying dataset was closed */

    if(ret_value >= 0){
        dataset_stats_prov_write(o->prov_info);//output stats


        dataset_release_info(o->prov_info);
        H5VL_provenance_free_obj(o);
        prov_write(o->prov_helper, __func__, get_time_usec() - start);
    }


    return ret_value;
} /* end H5VL_provenance_dataset_close() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_commit
 *
 * Purpose:     Commits a datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_datatype_commit(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id,
    hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *dt;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Commit\n");
#endif

    under = H5VLdatatype_commit(o->under_object, loc_params, o->under_vol_id, name, type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
    if(under) {
        dt = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);
        dt->prov_info = new_datatype_info();

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        dt = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void *)dt;
} /* end H5VL_provenance_datatype_commit() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_open
 *
 * Purpose:     Opens a named datatype inside a container.
 *
 * Return:      Success:    Pointer to datatype object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_datatype_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *dt;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Open\n");
#endif

    under = H5VLdatatype_open(o->under_object, loc_params, o->under_vol_id, name, tapl_id, dxpl_id, req);
    if(under) {
        dt = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

        dt->prov_info = new_datatype_info();
    } /* end if */
    else
        dt = NULL;

    prov_write(dt->prov_helper, __func__, get_time_usec() - start);
    return (void *)dt;
} /* end H5VL_provenance_datatype_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_get
 *
 * Purpose:     Get information about a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_datatype_get(void *dt, H5VL_datatype_get_t get_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dt;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Get\n");
#endif

    ret_value = H5VLdatatype_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_datatype_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_specific
 *
 * Purpose:     Specific operations for datatypes
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_datatype_specific(void *obj, H5VL_datatype_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Specific\n");
#endif

    ret_value = H5VLdatatype_specific(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_datatype_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_optional
 *
 * Purpose:     Perform a connector-specific operation on a datatype
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_datatype_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Optional\n");
#endif

    ret_value = H5VLdatatype_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_datatype_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_datatype_close
 *
 * Purpose:     Closes a datatype.
 *
 * Return:      Success:    0
 *              Failure:    -1, datatype not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_datatype_close(void *dt, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)dt;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL DATATYPE Close\n");
#endif

    assert(o->under_object);

    ret_value = H5VLdatatype_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    /* Release our wrapper, if underlying datatype was closed */
    datatype_stats_prov_write(o->prov_info);
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);


    return ret_value;
} /* end H5VL_provenance_datatype_close() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_file_create(const char *name, unsigned flags, hid_t fcpl_id,
    hid_t fapl_id, hid_t dxpl_id, void **req)
{

    H5VL_provenance_info_t *info;
    H5VL_provenance_t *file;
    hid_t under_fapl_id;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL FILE Create\n");
#endif


printf("%s\n", __func__);
    /* Get copy of our VOL info from FAPL */

    H5Pget_vol_info(fapl_id, (void **)&info);

    printf("Verifying info content: prov_file_path = [%s], prov_level = [%d], format = [%s]\n", info->prov_file_path, info->prov_level, info->prov_line_format);

    /* Copy the FAPL */
    under_fapl_id = H5Pcopy(fapl_id);

    /* Set the VOL ID and info for the underlying FAPL */
    H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

    unsigned long start = get_time_usec();

    /* Open the file with the underlying VOL connector */
    under = H5VLfile_create(name, flags, fcpl_id, under_fapl_id, dxpl_id, req);

    prov_helper* prov_helper = prov_helper_init(info->prov_file_path, info->prov_level, info->prov_line_format);
    if(under) {

        file = H5VL_provenance_new_obj(under, info->under_vol_id, prov_helper);
        file->prov_info = new_file_info();
        file->shared_file_info = calloc(1, sizeof(H5VL_provenance_info_t));

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, info->under_vol_id, prov_helper);
    } /* end if */
    else
        file = NULL;

    /* Close underlying FAPL */
    H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    H5VL_provenance_info_free(info);

    prov_write(file->prov_helper, __func__, get_time_usec() - start);

    return (void *)file;
} /* end H5VL_provenance_file_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_file_open(const char *name, unsigned flags, hid_t fapl_id,
    hid_t dxpl_id, void **req)
{

    H5VL_provenance_info_t *info;
    H5VL_provenance_t *file;
    hid_t under_fapl_id;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL FILE Open\n");
#endif

    /* Get copy of our VOL info from FAPL */
    H5Pget_vol_info(fapl_id, (void **)&info);

    /* Copy the FAPL */
    under_fapl_id = H5Pcopy(fapl_id);

    /* Set the VOL ID and info for the underlying FAPL */
    H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);


    unsigned long start = get_time_usec();
    /* Open the file with the underlying VOL connector */
    under = H5VLfile_open(name, flags, under_fapl_id, dxpl_id, req);
    prov_helper* ph = prov_helper_init(info->prov_file_path, info->prov_level, info->prov_line_format);
    if(under) {

        file = H5VL_provenance_new_obj(under, info->under_vol_id, ph);
        file->prov_helper = ph;

        file->prov_info = new_file_info();
        file->shared_file_info = calloc(1, sizeof(H5VL_provenance_info_t));

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, info->under_vol_id, file->prov_helper);
    } /* end if */
    else
        file = NULL;

    /* Close underlying FAPL */
    H5Pclose(under_fapl_id);

    /* Release copy of our VOL info */
    H5VL_provenance_info_free(info);

    prov_write(file->prov_helper, __func__, get_time_usec() - start);
    return (void *)file;
} /* end H5VL_provenance_file_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id,
    void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)file;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL FILE Get\n");
#endif

    ret_value = H5VLfile_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_file_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_specific_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              file specific callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_file_specific_reissue(void *obj, hid_t connector_id,
    H5VL_file_specific_t specific_type, hid_t dxpl_id, void **req, ...)
{
    va_list arguments;
    herr_t ret_value;
    va_start(arguments, req);
    ret_value = H5VLfile_specific(obj, connector_id, specific_type, dxpl_id, req, arguments);
    va_end(arguments);

    return ret_value;
} /* end H5VL_provenance_file_specific_reissue() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_file_specific(void *file, H5VL_file_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)file;
    hid_t under_vol_id = -1;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL FILE Specific\n");
#endif

    /* Unpack arguments to get at the child file pointer when mounting a file */
    if(specific_type == H5VL_FILE_MOUNT) {
        H5I_type_t loc_type;
        const char *name;
        H5VL_provenance_t *child_file;
        hid_t plist_id;

        /* Retrieve parameters for 'mount' operation, so we can unwrap the child file */
        loc_type = (H5I_type_t)va_arg(arguments, int); /* enum work-around */
        name = va_arg(arguments, const char *);
        child_file = (H5VL_provenance_t *)va_arg(arguments, void *);
        plist_id = va_arg(arguments, hid_t);

        /* Keep the correct underlying VOL ID for possible async request token */
        under_vol_id = o->under_vol_id;

        /* Re-issue 'file specific' call, using the unwrapped pieces */
        ret_value = H5VL_provenance_file_specific_reissue(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, (int)loc_type, name, child_file->under_object, plist_id);
    } /* end if */
    else if(specific_type == H5VL_FILE_IS_ACCESSIBLE) {
        H5VL_provenance_info_t *info;
        hid_t fapl_id, under_fapl_id;
        const char *name;
        htri_t *ret;

        /* Get the arguments for the 'is accessible' check */
        fapl_id = va_arg(arguments, hid_t);
        name    = va_arg(arguments, const char *);
        ret     = va_arg(arguments, htri_t *);

        /* Get copy of our VOL info from FAPL */
        H5Pget_vol_info(fapl_id, (void **)&info);

        /* Copy the FAPL */
        under_fapl_id = H5Pcopy(fapl_id);

        /* Set the VOL ID and info for the underlying FAPL */
        H5Pset_vol(under_fapl_id, info->under_vol_id, info->under_vol_info);

        /* Keep the correct underlying VOL ID for possible async request token */
        under_vol_id = info->under_vol_id;

        /* Re-issue 'file specific' call */
        ret_value = H5VL_provenance_file_specific_reissue(NULL, info->under_vol_id, specific_type, dxpl_id, req, under_fapl_id, name, ret);

        /* Close underlying FAPL */
        H5Pclose(under_fapl_id);

        /* Release copy of our VOL info */
        H5VL_provenance_info_free(info);
    } /* end else-if */
    else {
        va_list my_arguments;

        /* Make a copy of the argument list for later, if reopening */
        if(specific_type == H5VL_FILE_REOPEN)
            va_copy(my_arguments, arguments);

        /* Keep the correct underlying VOL ID for possible async request token */
        under_vol_id = o->under_vol_id;

        ret_value = H5VLfile_specific(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, arguments);

        /* Wrap file struct pointer, if we reopened one */
        if(specific_type == H5VL_FILE_REOPEN) {
            if(ret_value >= 0) {
                void      **ret = va_arg(my_arguments, void **);

                if(ret && *ret)
                    *ret = H5VL_provenance_new_obj(*ret, o->under_vol_id, o->prov_helper);
            } /* end if */

            /* Finish use of copied vararg list */
            va_end(my_arguments);
        } /* end if */
    } /* end else */

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_file_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_file_optional(void *file, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)file;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL File Optional\n");
#endif

    ret_value = H5VLfile_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_file_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_file_close(void *file, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)file;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL FILE Close\n");
#endif

    ret_value = H5VLfile_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    /* Release our wrapper, if underlying file was closed */
    prov_write(o->prov_helper, __func__, get_time_usec() - start);

    file_stats_prov_write(o->shared_file_info);
    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);


    return ret_value;
} /* end H5VL_provenance_file_close() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_create
 *
 * Purpose:     Creates a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_group_create(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *group;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL GROUP Create\n");
#endif

    under = H5VLgroup_create(o->under_object, loc_params, o->under_vol_id, name, gcpl_id,  gapl_id, dxpl_id, req);
    if(under) {
        group = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);
        group->prov_info = new_group_info();
        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        group = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void *)group;
} /* end H5VL_provenance_group_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_open
 *
 * Purpose:     Opens a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_group_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *group;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL GROUP Open\n");
#endif

    under = H5VLgroup_open(o->under_object, loc_params, o->under_vol_id, name, gapl_id, dxpl_id, req);
    if(under) {
        group = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);
        group->prov_info = new_group_info();
        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        group = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void *)group;
} /* end H5VL_provenance_group_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_get
 *
 * Purpose:     Get info about a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_group_get(void *obj, H5VL_group_get_t get_type, hid_t dxpl_id,
    void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL GROUP Get\n");
#endif

    ret_value = H5VLgroup_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_group_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_specific
 *
 * Purpose:     Specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_group_specific(void *obj, H5VL_group_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL GROUP Specific\n");
#endif

    ret_value = H5VLgroup_specific(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_group_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_optional
 *
 * Purpose:     Perform a connector-specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_group_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL GROUP Optional\n");
#endif

    ret_value = H5VLgroup_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_group_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:      Success:    0
 *              Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_group_close(void *grp, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)grp;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL H5Gclose\n");
#endif

    ret_value = H5VLgroup_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    /* Release our wrapper, if underlying file was closed */
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    group_stats_prov_write(o->prov_info);
    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);


    return ret_value;
} /* end H5VL_provenance_group_close() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_create
 *
 * Purpose:     Creates a hard / soft / UD / external link.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_link_create(H5VL_link_create_type_t create_type, void *obj, const H5VL_loc_params_t *loc_params, hid_t lcpl_id, hid_t lapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    hid_t under_vol_id = -1;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Create\n");
#endif

    /* Try to retrieve the "under" VOL id */
    if(o)
        under_vol_id = o->under_vol_id;

    /* Fix up the link target object for hard link creation */
    if(H5VL_LINK_CREATE_HARD == create_type) {
        void         *cur_obj;

        /* Retrieve the object for the link target */
        H5Pget(lcpl_id, H5VL_PROP_LINK_TARGET, &cur_obj);

        /* If it's a non-NULL pointer, find the 'under object' and re-set the property */
        if(cur_obj) {
            /* Check if we still need the "under" VOL ID */
            if(under_vol_id < 0)
                under_vol_id = ((H5VL_provenance_t *)cur_obj)->under_vol_id;

            /* Set the object for the link target */
            H5Pset(lcpl_id, H5VL_PROP_LINK_TARGET, &(((H5VL_provenance_t *)cur_obj)->under_object));
        } /* end if */
    } /* end if */

    ret_value = H5VLlink_create(create_type, (o ? o->under_object : NULL), loc_params, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_copy
 *
 * Purpose:     Renames an object within an HDF5 container and copies it to a new
 *              group.  The original name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_link_copy(void *src_obj, const H5VL_loc_params_t *loc_params1,
    void *dst_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o_src = (H5VL_provenance_t *)src_obj;
    H5VL_provenance_t *o_dst = (H5VL_provenance_t *)dst_obj;
    hid_t under_vol_id = -1;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Copy\n");
#endif

    /* Retrieve the "under" VOL id */
    if(o_src)
        under_vol_id = o_src->under_vol_id;
    else if(o_dst)
        under_vol_id = o_dst->under_vol_id;
    assert(under_vol_id > 0);

    ret_value = H5VLlink_copy((o_src ? o_src->under_object : NULL), loc_params1, (o_dst ? o_dst->under_object : NULL), loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, under_vol_id, o_dst->prov_helper);
            
    prov_write(o_dst->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_move
 *
 * Purpose:     Moves a link within an HDF5 file to a new group.  The original
 *              name SRC is unlinked from the group graph
 *              and then inserted with the new name DST (which can specify a
 *              new path for the object) as an atomic operation. The names
 *              are interpreted relative to SRC_LOC_ID and
 *              DST_LOC_ID, which are either file IDs or group ID.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_link_move(void *src_obj, const H5VL_loc_params_t *loc_params1,
    void *dst_obj, const H5VL_loc_params_t *loc_params2, hid_t lcpl_id,
    hid_t lapl_id, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o_src = (H5VL_provenance_t *)src_obj;
    H5VL_provenance_t *o_dst = (H5VL_provenance_t *)dst_obj;
    hid_t under_vol_id = -1;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Move\n");
#endif

    /* Retrieve the "under" VOL id */
    if(o_src)
        under_vol_id = o_src->under_vol_id;
    else if(o_dst)
        under_vol_id = o_dst->under_vol_id;
    assert(under_vol_id > 0);

    ret_value = H5VLlink_move((o_src ? o_src->under_object : NULL), loc_params1, (o_dst ? o_dst->under_object : NULL), loc_params2, under_vol_id, lcpl_id, lapl_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, under_vol_id, o_dst->prov_helper);

    prov_write(o_dst->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_move() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_get
 *
 * Purpose:     Get info about a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_link_get(void *obj, const H5VL_loc_params_t *loc_params,
    H5VL_link_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Get\n");
#endif

    ret_value = H5VLlink_get(o->under_object, loc_params, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
            
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_specific
 *
 * Purpose:     Specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_link_specific(void *obj, const H5VL_loc_params_t *loc_params,
    H5VL_link_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Specific\n");
#endif

    ret_value = H5VLlink_specific(o->under_object, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_link_optional
 *
 * Purpose:     Perform a connector-specific operation on a link
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5VL_provenance_link_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL LINK Optional\n");
#endif

    ret_value = H5VLlink_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_link_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_object_open
 *
 * Purpose:     Opens an object inside a container.
 *
 * Return:      Success:    Pointer to object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
static void *
H5VL_provenance_object_open(void *obj, const H5VL_loc_params_t *loc_params,
    H5I_type_t *opened_type, hid_t dxpl_id, void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *new_obj;
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    void *under;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL OBJECT Open\n");
#endif

    under = H5VLobject_open(o->under_object, loc_params, o->under_vol_id, opened_type, dxpl_id, req);
    if(under) {
        new_obj = H5VL_provenance_new_obj(under, o->under_vol_id, o->prov_helper);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);
    } /* end if */
    else
        new_obj = NULL;

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return (void *)new_obj;
} /* end H5VL_provenance_object_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_object_copy
 *
 * Purpose:     Copies an object inside a container.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_object_copy(void *src_obj, const H5VL_loc_params_t *src_loc_params,
    const char *src_name, void *dst_obj, const H5VL_loc_params_t *dst_loc_params,
    const char *dst_name, hid_t ocpypl_id, hid_t lcpl_id, hid_t dxpl_id,
    void **req)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o_src = (H5VL_provenance_t *)src_obj;
    H5VL_provenance_t *o_dst = (H5VL_provenance_t *)dst_obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL OBJECT Copy\n");
#endif

    ret_value = H5VLobject_copy(o_src->under_object, src_loc_params, src_name, o_dst->under_object, dst_loc_params, dst_name, o_src->under_vol_id, ocpypl_id, lcpl_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o_src->under_vol_id, o_dst->prov_helper);

    prov_write(o_dst->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_object_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_object_get
 *
 * Purpose:     Get info about an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_object_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL OBJECT Get\n");
#endif

    ret_value = H5VLobject_get(o->under_object, loc_params, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_object_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_object_specific
 *
 * Purpose:     Specific operation on an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_object_specific(void *obj, const H5VL_loc_params_t *loc_params,
    H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL OBJECT Specific\n");
#endif

    ret_value = H5VLobject_specific(o->under_object, loc_params, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_object_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_object_optional
 *
 * Purpose:     Perform a connector-specific operation for an object
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_object_optional(void *obj, hid_t dxpl_id, void **req,
    va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL OBJECT Optional\n");
#endif

    ret_value = H5VLobject_optional(o->under_object, o->under_vol_id, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_provenance_new_obj(*req, o->under_vol_id, o->prov_helper);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_object_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_wait
 *
 * Purpose:     Wait (with a timeout) for an async operation to complete
 *
 * Note:        Releases the request if the operation has completed and the
 *              connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_wait(void *obj, uint64_t timeout,
    H5ES_status_t *status)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Wait\n");
#endif

    ret_value = H5VLrequest_wait(o->under_object, o->under_vol_id, timeout, status);

    if(ret_value >= 0 && *status != H5ES_STATUS_IN_PROGRESS)
        H5VL_provenance_free_obj(o);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_request_wait() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_notify
 *
 * Purpose:     Registers a user callback to be invoked when an asynchronous
 *              operation completes
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_notify(void *obj, H5VL_request_notify_t cb, void *ctx)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Wait\n");
#endif

    ret_value = H5VLrequest_notify(o->under_object, o->under_vol_id, cb, ctx);

    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_request_notify() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_cancel
 *
 * Purpose:     Cancels an asynchronous operation
 *
 * Note:        Releases the request, if connector callback succeeds
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_cancel(void *obj)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Cancel\n");
#endif

    ret_value = H5VLrequest_cancel(o->under_object, o->under_vol_id);

    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_request_cancel() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_specific_reissue
 *
 * Purpose:     Re-wrap vararg arguments into a va_list and reissue the
 *              request specific callback to the underlying VOL connector.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_specific_reissue(void *obj, hid_t connector_id,
    H5VL_request_specific_t specific_type, ...)
{
    va_list arguments;
    herr_t ret_value;

    va_start(arguments, specific_type);
    ret_value = H5VLrequest_specific(obj, connector_id, specific_type, arguments);
    va_end(arguments);

    return ret_value;
} /* end H5VL_provenance_request_specific_reissue() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_specific
 *
 * Purpose:     Specific operation on a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_specific(void *obj, H5VL_request_specific_t specific_type,
    va_list arguments)
{

    herr_t ret_value = -1;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Specific\n");
#endif

    if(H5VL_REQUEST_WAITANY == specific_type ||
            H5VL_REQUEST_WAITSOME == specific_type ||
            H5VL_REQUEST_WAITALL == specific_type) {
        va_list tmp_arguments;
        size_t req_count;

        /* Sanity check */
        assert(obj == NULL);

        /* Get enough info to call the underlying connector */
        va_copy(tmp_arguments, arguments);
        req_count = va_arg(tmp_arguments, size_t);

        /* Can only use a request to invoke the underlying VOL connector when there's >0 requests */
        if(req_count > 0) {
            void **req_array;
            void **under_req_array;
            uint64_t timeout;
            H5VL_provenance_t *o;
            size_t u;               /* Local index variable */

            /* Get the request array */
            req_array = va_arg(tmp_arguments, void **);

            /* Get a request to use for determining the underlying VOL connector */
            o = (H5VL_provenance_t *)req_array[0];

            /* Create array of underlying VOL requests */
            under_req_array = (void **)malloc(req_count * sizeof(void **));
            for(u = 0; u < req_count; u++)
                under_req_array[u] = ((H5VL_provenance_t *)req_array[u])->under_object;

            /* Remove the timeout value from the vararg list (it's used in all the calls below) */
            timeout = va_arg(tmp_arguments, uint64_t);

            /* Release requests that have completed */
            if(H5VL_REQUEST_WAITANY == specific_type) {
                size_t *index;          /* Pointer to the index of completed request */
                H5ES_status_t *status;  /* Pointer to the request's status */

                /* Retrieve the remaining arguments */
                index = va_arg(tmp_arguments, size_t *);
                assert(*index <= req_count);
                status = va_arg(tmp_arguments, H5ES_status_t *);

                /* Reissue the WAITANY 'request specific' call */
                ret_value = H5VL_provenance_request_specific_reissue(o->under_object, o->under_vol_id, specific_type, req_count, under_req_array, timeout, index, status);

                /* Release the completed request, if it completed */
                if(ret_value >= 0 && *status != H5ES_STATUS_IN_PROGRESS) {
                    H5VL_provenance_t *tmp_o;

                    tmp_o = (H5VL_provenance_t *)req_array[*index];
                    H5VL_provenance_free_obj(tmp_o);
                } /* end if */
            } /* end if */
            else if(H5VL_REQUEST_WAITSOME == specific_type) {
                size_t *outcount;               /* # of completed requests */
                unsigned *array_of_indices;     /* Array of indices for completed requests */
                H5ES_status_t *array_of_statuses; /* Array of statuses for completed requests */

                /* Retrieve the remaining arguments */
                outcount = va_arg(tmp_arguments, size_t *);
                assert(*outcount <= req_count);
                array_of_indices = va_arg(tmp_arguments, unsigned *);
                array_of_statuses = va_arg(tmp_arguments, H5ES_status_t *);

                /* Reissue the WAITSOME 'request specific' call */
                ret_value = H5VL_provenance_request_specific_reissue(o->under_object, o->under_vol_id, specific_type, req_count, under_req_array, timeout, outcount, array_of_indices, array_of_statuses);

                /* If any requests completed, release them */
                if(ret_value >= 0 && *outcount > 0) {
                    unsigned *idx_array;    /* Array of indices of completed requests */

                    /* Retrieve the array of completed request indices */
                    idx_array = va_arg(tmp_arguments, unsigned *);

                    /* Release the completed requests */
                    for(u = 0; u < *outcount; u++) {
                        H5VL_provenance_t *tmp_o;

                        tmp_o = (H5VL_provenance_t *)req_array[idx_array[u]];
                        H5VL_provenance_free_obj(tmp_o);
                    } /* end for */
                } /* end if */
            } /* end else-if */
            else {      /* H5VL_REQUEST_WAITALL == specific_type */
                H5ES_status_t *array_of_statuses; /* Array of statuses for completed requests */

                /* Retrieve the remaining arguments */
                array_of_statuses = va_arg(tmp_arguments, H5ES_status_t *);

                /* Reissue the WAITALL 'request specific' call */
                ret_value = H5VL_provenance_request_specific_reissue(o->under_object, o->under_vol_id, specific_type, req_count, under_req_array, timeout, array_of_statuses);

                /* Release the completed requests */
                if(ret_value >= 0) {
                    for(u = 0; u < req_count; u++) {
                        if(array_of_statuses[u] != H5ES_STATUS_IN_PROGRESS) {
                            H5VL_provenance_t *tmp_o;

                            tmp_o = (H5VL_provenance_t *)req_array[u];
                            H5VL_provenance_free_obj(tmp_o);
                        } /* end if */
                    } /* end for */
                } /* end if */
            } /* end else */

            /* Release array of requests for underlying connector */
            free(under_req_array);
        } /* end if */

        /* Finish use of copied vararg list */
        va_end(tmp_arguments);
    } /* end if */
    else
        assert(0 && "Unknown 'specific' operation");

    return ret_value;
} /* end H5VL_provenance_request_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_optional
 *
 * Purpose:     Perform a connector-specific operation for a request
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_optional(void *obj, va_list arguments)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Optional\n");
#endif

    ret_value = H5VLrequest_optional(o->under_object, o->under_vol_id, arguments);

    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    return ret_value;
} /* end H5VL_provenance_request_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_provenance_request_free
 *
 * Purpose:     Releases a request, allowing the operation to complete without
 *              application tracking
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5VL_provenance_request_free(void *obj)
{
    unsigned long start = get_time_usec();
    H5VL_provenance_t *o = (H5VL_provenance_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PROVNC_LOGGING
    printf("------- PASS THROUGH VOL REQUEST Free\n");
#endif
    prov_write(o->prov_helper, __func__, get_time_usec() - start);
    ret_value = H5VLrequest_free(o->under_object, o->under_vol_id);

    if(ret_value >= 0)
        H5VL_provenance_free_obj(o);

    return ret_value;
} /* end H5VL_provenance_request_free() */

