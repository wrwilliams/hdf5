/*
 * h5vol_prov.c
 *
 *  Created on: Jul 9, 2018
 *      Author: tonglin
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hdf5.h"

#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
//#include <pthread.h>

#define LOG 502

static herr_t H5VL_prov_init(hid_t vipl_id);
static herr_t H5VL_prov_term(hid_t vtpl_id);

/* Datatype callbacks */
static void *H5VL_prov_datatype_commit(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req);
static void *H5VL_prov_datatype_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t tapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_prov_datatype_get(void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_prov_datatype_close(void *dt, hid_t dxpl_id, void **req);

/* Dataset callbacks */
static void *H5VL_prov_dataset_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
static void *H5VL_prov_dataset_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_prov_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                                    hid_t file_space_id, hid_t plist_id, void *buf, void **req);
static herr_t H5VL_prov_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                                     hid_t file_space_id, hid_t plist_id, const void *buf, void **req);
static herr_t H5VL_prov_dataset_close(void *dset, hid_t dxpl_id, void **req);

/* File callbacks */
static void *H5VL_prov_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req);
static void *H5VL_prov_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_prov_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
static herr_t H5VL_prov_file_close(void *file, hid_t dxpl_id, void **req);

/* Group callbacks */
static void *H5VL_prov_group_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req);
static herr_t H5VL_prov_group_close(void *grp, hid_t dxpl_id, void **req);

/* Link callbacks */

/* Object callbacks */
static void *H5VL_prov_object_open(void *obj, H5VL_loc_params_t loc_params, H5I_type_t *opened_type, hid_t dxpl_id, void **req);
static herr_t H5VL_prov_object_specific(void *obj, H5VL_loc_params_t loc_params, H5VL_object_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);

typedef enum ProvLevel {
    Default, //no file write, only screen print
    Print_only,
    File_only,
    File_and_print,
    Level3,
    Level4,
    Disabled
}Prov_level;

typedef struct ProvenanceHelper{
    char* prov_file_path;
    FILE* prov_file_handle;
    Prov_level prov_level;
    char* prov_line_format;
    char user_name[32];
    int pid;
    uint64_t tid;
    char proc_name[64];
}prov_helper;

prov_helper* global_prov_helper;

void prov_helper_init(prov_helper* helper, char* file_path, Prov_level prov_level, char* prov_line_format){
    printf("prov_helper_init 0.\n");
    helper->prov_file_path = file_path;
    helper->prov_level = prov_level;
    helper->prov_line_format = prov_line_format;
    helper->pid = getpid();
    //pthread_id_np_t   tid;

    //pid_t tid = gettid(); Linux
    pthread_threadid_np(NULL, &(helper->tid));//OSX only
    getlogin_r(helper->user_name, 32);

    if(helper->prov_level == File_only | helper->prov_level == File_and_print){
        helper->prov_file_handle = fopen(helper->prov_file_path, "a");
    }

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

static int prov_write(prov_helper* helper_in, char* msg, unsigned long duration){
    char time[64];
    get_time_str(time);
    char pline[512];
    sprintf(pline, "[%s][User:%s][PID:%d][TID:%ld][Func:%s][Dura:%lu]\n", time, helper_in->user_name, helper_in->pid, helper_in->tid, msg, duration);

    switch(helper_in->prov_level){
        case File_only:
            fputs(pline, helper_in->prov_file_handle);
            break;

        case File_and_print:
            fputs(pline, helper_in->prov_file_handle);
            printf(pline);
            break;

        case Print_only:
            printf(pline);
            break;

        default:
            break;
    }

    if(helper_in->prov_level == (File_only | File_and_print)){
        fputs(pline, helper_in->prov_file_handle);
    }

    return 0;
}
//prov_write(global_prov_helper, );

hid_t native_driver_id = -1;

static const H5VL_class_t H5VL_prov_g = {
    0,
    LOG,
    "log",                  /* name */
    H5VL_prov_init,                              /* initialize */
    H5VL_prov_term,                              /* terminate */
    sizeof(hid_t),
    NULL,
    NULL,
    {                                           /* attribute_cls */
        NULL, //H5VL_prov_attr_create,                /* create */
        NULL, //H5VL_prov_attr_open,                  /* open */
        NULL, //H5VL_prov_attr_read,                  /* read */
        NULL, //H5VL_prov_attr_write,                 /* write */
        NULL, //H5VL_prov_attr_get,                   /* get */
        NULL, //H5VL_prov_attr_specific,              /* specific */
        NULL, //H5VL_prov_attr_optional,              /* optional */
        NULL  //H5VL_prov_attr_close                  /* close */
    },
    {                                           /* dataset_cls */
        H5VL_prov_dataset_create,                    /* create */
        H5VL_prov_dataset_open,                      /* open */
        H5VL_prov_dataset_read,                      /* read */
        H5VL_prov_dataset_write,                     /* write */
        NULL, //H5VL_prov_dataset_get,               /* get */
        NULL, //H5VL_prov_dataset_specific,          /* specific */
        NULL, //H5VL_prov_dataset_optional,          /* optional */
        H5VL_prov_dataset_close                      /* close */
    },
    {                                               /* datatype_cls */
        H5VL_prov_datatype_commit,                   /* commit */
        H5VL_prov_datatype_open,                     /* open */
        H5VL_prov_datatype_get,                      /* get_size */
        NULL, //H5VL_prov_datatype_specific,         /* specific */
        NULL, //H5VL_prov_datatype_optional,         /* optional */
        H5VL_prov_datatype_close                     /* close */
    },
    {                                           /* file_cls */
        H5VL_prov_file_create,                      /* create */
        H5VL_prov_file_open,                        /* open */
        H5VL_prov_file_get,                         /* get */
        NULL, //H5VL_prov_file_specific,            /* specific */
        NULL, //H5VL_prov_file_optional,            /* optional */
        H5VL_prov_file_close                        /* close */
    },
    {                                           /* group_cls */
        H5VL_prov_group_create,                     /* create */
        NULL, //H5VL_prov_group_open,               /* open */
        NULL, //H5VL_prov_group_get,                /* get */
        NULL, //H5VL_prov_group_specific,           /* specific */
        NULL, //H5VL_prov_group_optional,           /* optional */
        H5VL_prov_group_close                       /* close */
    },
    {                                           /* link_cls */
        NULL, //H5VL_prov_link_create,                /* create */
        NULL, //H5VL_prov_link_copy,                  /* copy */
        NULL, //H5VL_prov_link_move,                  /* move */
        NULL, //H5VL_prov_link_get,                   /* get */
        NULL, //H5VL_prov_link_specific,              /* specific */
        NULL, //H5VL_prov_link_optional,              /* optional */
    },
    {                                           /* object_cls */
        H5VL_prov_object_open,                        /* open */
        NULL, //H5VL_prov_object_copy,                /* copy */
        NULL, //H5VL_prov_object_get,                 /* get */
        H5VL_prov_object_specific,                    /* specific */
        NULL, //H5VL_prov_object_optional,            /* optional */
    },
    {
        NULL,
        NULL,
        NULL
    },
    NULL
};

typedef struct H5VL_prov_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
} H5VL_prov_t;

typedef struct H5VL_prov_datatype_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
    char* dtype_name;
    int datatype_commit_cnt;
    int datatype_get_cnt;
} H5VL_prov_datatype_t;

typedef struct H5VL_prov_dataset_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
    char* dset_name;
    size_t dset_type_size;
    hsize_t dset_space_size;
    hsize_t tatal_bytes_read;
    hsize_t tatal_bytes_written;
    hsize_t tatal_read_ns;
    hsize_t tatal_write_ns;
    int dataset_read_cnt;
    int dataset_write_cnt;
} H5VL_prov_dataset_t;

typedef struct H5VL_prov_group_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
    int group_get_cnt;
    int group_specific_cnt;
} H5VL_prov_group_t;

typedef struct H5VL_prov_link_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
    int link_get_cnt;
    int link_specific_cnt;
} H5VL_prov_link_t;

typedef struct H5VL_prov_file_t {
    void   *under_object;
    char* func_name;
    int func_cnt;//stats
} H5VL_prov_file_t;

static int stat_write_dataset(H5VL_prov_dataset_t* ds){
    printf("=========================\n");
    printf("Dataset name: %s\n", ds->dset_name);
    printf("Dataset reads:%d\n", ds->dataset_read_cnt);
    printf("Dataset writes:%d\n", ds->dataset_write_cnt);
    printf("Dataset read %llu bytes in %llu usec\n", (unsigned long long)(ds->tatal_bytes_read), (unsigned long long)(ds->tatal_read_ns));
    printf("Dataset wrote %llu bytes in %llu usec\n", (unsigned long long)(ds->tatal_bytes_written), (unsigned long long)(ds->tatal_write_ns));
    printf("=========================\n");
    return 0;
}

static int stat_write_datatype(H5VL_prov_datatype_t* dt){
    printf("=========================\n");
    printf("Datatype name: %s\n", dt->dtype_name);
    printf("Datatype commits:%d\n", dt->datatype_commit_cnt);
    printf("Datatype gets:%d\n", dt->datatype_get_cnt);
    printf("=========================\n");
    return 0;
}

static herr_t
visit_cb(hid_t oid, const char *name,
         const H5O_info_t *oinfo, void *udata)
{
    ssize_t len;
    char n[25];

    if(H5Iget_type(oid) == H5I_GROUP) {
        len = H5VLget_driver_name(oid, n, 50);
        printf ("Visiting GROUP VOL name = %s  %d\n", n, len);
    }
    if(H5Iget_type(oid) == H5I_DATASET)
        printf("visiting dataset\n");
    if(H5Iget_type(oid) == H5I_DATATYPE)
        printf("visiting datatype\n");

    return 1;
} /* end h5_verify_cached_stabs_cb() */

int main(int argc, char **argv) {
        const char file_name[]="large_dataset.h5";
    const char group_name[]="/Group";
    const char dataset_name[]="Data";
    char fullpath[500];
    hid_t file_id;
    hid_t group_id;
    hid_t dataspaceId;
        hid_t datasetId;
        hid_t acc_tpl;
        hid_t under_fapl;
        hid_t vol_id, vol_id2;
        hid_t int_id;
        hid_t attr;
        hid_t space;
    const unsigned int nelem=60;
    int *data;
    unsigned int i;
    hsize_t dims[1];
        ssize_t len;
        char name[25];
        static hsize_t      ds_size[2] = {10, 20};

        global_prov_helper = malloc(sizeof(prov_helper));
        prov_helper_init(global_prov_helper, "./prov.txt", File_and_print, "");
        printf("prov_helper_init done\n");
        prov_write(global_prov_helper, __func__, 0);//main function reports
        printf("prov_write done\n");
        //prov_helper_teardown(GLOBAL_prov_helper);
        //printf("prov_helper_teardown done.");
        prov_write(global_prov_helper, __func__, 0);

        under_fapl = H5Pcreate (H5P_FILE_ACCESS);
        H5Pset_fapl_native(under_fapl);
        assert(H5VLis_registered("native") == 1);
        printf("2\n");
        vol_id = H5VLregister (&H5VL_prov_g);
        assert(vol_id > 0);
        assert(H5VLis_registered("log") == 1);
        printf("3\n");
        vol_id2 = H5VLget_driver_id("log");
        H5VLinitialize(vol_id2, H5P_DEFAULT);
        H5VLclose(vol_id2);
        printf("4\n");
        native_driver_id = H5VLget_driver_id("native");
        assert(native_driver_id > 0);

        acc_tpl = H5Pcreate (H5P_FILE_ACCESS);
        H5Pset_vol(acc_tpl, vol_id, &under_fapl);
        printf("5\n");
    file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, acc_tpl);
    printf("6\n");
        len = H5VLget_driver_name(file_id, name, 25);
        printf("7\n");
        printf ("FILE VOL name = %s  %d\n", name, len);

    group_id = H5Gcreate2(file_id, group_name, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        len = H5VLget_driver_name(group_id, name, 50);
        printf ("GROUP VOL name = %s  %d\n", name, len);

        int_id = H5Tcopy(H5T_NATIVE_INT);
        H5Tcommit2(file_id, "int", int_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        len = H5VLget_driver_name(int_id, name, 50);
        printf ("DT COMMIT name = %s  %d\n", name, len);
        H5Tclose(int_id);

        int_id = H5Topen2(file_id, "int", H5P_DEFAULT);
        len = H5VLget_driver_name(int_id, name, 50);
        printf ("DT OPEN name = %s  %d\n", name, len);
        H5Tclose(int_id);

        int_id = H5Oopen(file_id,"int",H5P_DEFAULT);
        len = H5VLget_driver_name(int_id, name, 50);
        printf ("DT OOPEN name = %s  %d\n", name, len);

        len = H5Fget_name(file_id, name, 50);
        printf("name = %d  %s\n", len, name);

    data = malloc (sizeof(int)*nelem);
    for(i=0;i<nelem;++i)
      data[i]=i;

    dims [0] = 60;
    dataspaceId = H5Screate_simple(1, dims, NULL);
        space = H5Screate_simple (2, ds_size, ds_size);

    sprintf(fullpath,"%s/%s",group_name,dataset_name);
    datasetId = H5Dcreate2(file_id,fullpath,H5T_NATIVE_INT,dataspaceId,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);

    H5Sclose(dataspaceId);

        len = H5VLget_driver_name(datasetId, name, 50);
        printf ("DSET name = %s  %d\n", name, len);

    H5Dwrite(datasetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);

    H5Dclose(datasetId);

        H5Ovisit(file_id, H5_INDEX_NAME, H5_ITER_NATIVE, visit_cb, NULL);

    free (data);

        H5Oclose(int_id);

        H5Sclose (space);
    H5Gclose(group_id);
#if 0

        attr = H5Acreate2(group_id, "attr1", int_id, space, H5P_DEFAULT, H5P_DEFAULT);
        int_id = H5Aget_type(attr);
        len = H5VLget_driver_name(int_id, name, 50);
        printf ("DT OPEN name = %s  %d\n", name, len);

        H5Aclose (attr);

        int_id = H5Oopen(file_id,"int",H5P_DEFAULT);
        len = H5VLget_driver_name(int_id, name, 50);
        printf ("DT OOPEN name = %s  %d\n", name, len);
        H5Oclose(int_id);


    H5Fclose(file_id);
    file_id = H5Fopen(file_name, H5F_ACC_RDWR, H5P_DEFAULT);//acc_tpl);
        H5Fflush(file_id, H5F_SCOPE_GLOBAL);
#endif

    H5Fclose(file_id);
        H5Pclose(acc_tpl);
        H5Pclose(under_fapl);

        H5VLclose(native_driver_id);
        H5VLterminate(vol_id, H5P_DEFAULT);
        H5VLunregister (vol_id);
        assert(H5VLis_registered("log") == 0);
        prov_helper_teardown(global_prov_helper);
        printf("prov_helper_teardown done.");
    return 0;
}

static herr_t H5VL_prov_init(hid_t vipl_id)
{
    prov_write(global_prov_helper, __func__, 0);

    printf("------- LOG INIT\n");
    return 0;
}

static herr_t H5VL_prov_term(hid_t vtpl_id)
{
    prov_write(global_prov_helper, __func__, 0);

    printf("------- LOG TERM\n");
    return 0;
}

static void *
H5VL_prov_file_create(const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req)
{
    hid_t under_fapl;
    H5VL_prov_t *file;
    unsigned long start = get_time_usec();
    file = (H5VL_prov_t *)calloc(1, sizeof(H5VL_prov_t));

    under_fapl = *((hid_t *)H5Pget_vol_info(fapl_id));
    file->under_object = H5VLfile_create(name, flags, fcpl_id, under_fapl, dxpl_id, req);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Fcreate\n");
    return (void *)file;
}

static void *
H5VL_prov_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req)
{
    hid_t under_fapl;
    H5VL_prov_t *file;
    unsigned long start = get_time_usec();
    file = (H5VL_prov_t *)calloc(1, sizeof(H5VL_prov_t));

    under_fapl = *((hid_t *)H5Pget_vol_info(fapl_id));
    file->under_object = H5VLfile_open(name, flags, under_fapl, dxpl_id, req);
    //prov_write(global_prov_helper, "file_open");
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Fopen\n");
    return (void *)file;
}

static herr_t
H5VL_prov_file_get(void *file, H5VL_file_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_prov_t *f = (H5VL_prov_t *)file;
    unsigned long start = get_time_usec();
    H5VLfile_get(f->under_object, native_driver_id, get_type, dxpl_id, req, arguments);
    //prov_write(global_prov_helper, "file_get");
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Fget %d\n", get_type);
    return 1;
}
static herr_t
H5VL_prov_file_close(void *file, hid_t dxpl_id, void **req)
{
    H5VL_prov_t *f = (H5VL_prov_t *)file;
    unsigned long start = get_time_usec();
    H5VLfile_close(f->under_object, native_driver_id, dxpl_id, req);
    free(f);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Fclose\n");
    return 1;
}

static void *
H5VL_prov_group_create(void *obj, H5VL_loc_params_t loc_params, const char *name,
                      hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    H5VL_prov_t *group;
    H5VL_prov_t *o = (H5VL_prov_t *)obj;
    unsigned long start = get_time_usec();
    group = (H5VL_prov_t *)calloc(1, sizeof(H5VL_prov_t));

    group->under_object = H5VLgroup_create(o->under_object, loc_params, native_driver_id, name, gcpl_id,  gapl_id, dxpl_id, req);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Gcreate\n");
    return (void *)group;
}

static herr_t
H5VL_prov_group_close(void *grp, hid_t dxpl_id, void **req)
{
    H5VL_prov_t *g = (H5VL_prov_t *)grp;
    unsigned long start = get_time_usec();
    H5VLgroup_close(g->under_object, native_driver_id, dxpl_id, req);
    free(g);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Gclose\n");
    return 1;
}

static void *
H5VL_prov_datatype_commit(void *obj, H5VL_loc_params_t loc_params, const char *name,
                         hid_t type_id, hid_t lcpl_id, hid_t tcpl_id, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    H5VL_prov_datatype_t *dt;
    H5VL_prov_datatype_t *o = (H5VL_prov_datatype_t *)obj;
    unsigned long start = get_time_usec();
    dt = (H5VL_prov_t *)calloc(1, sizeof(H5VL_prov_t));

    dt->under_object = H5VLdatatype_commit(o->under_object, loc_params, native_driver_id, name,
                                           type_id, lcpl_id, tcpl_id, tapl_id, dxpl_id, req);
    printf("------- LOG H5Tcommit: before +1: dt->datatype_commit_cnt = %d\n", dt->datatype_commit_cnt);
    dt->datatype_commit_cnt++;
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Tcommit\n");
    return dt;
}
static void *
H5VL_prov_datatype_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t tapl_id, hid_t dxpl_id, void **req)
{
    H5VL_prov_datatype_t *dt;//init dt
    H5VL_prov_datatype_t *o = (H5VL_prov_datatype_t *)obj;
    unsigned long start = get_time_usec();
    dt = (H5VL_prov_datatype_t *)calloc(1, sizeof(H5VL_prov_datatype_t));

    dt->under_object = H5VLdatatype_open(o->under_object, loc_params, native_driver_id, name, tapl_id, dxpl_id, req);
    dt->dtype_name = strdup(name);
    dt->datatype_commit_cnt = 0;
    dt->datatype_get_cnt = 0;
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Topen, will return dt = %p\n", (void *)dt);
    return (void *)dt;
}

static herr_t
H5VL_prov_datatype_get(void *dt, H5VL_datatype_get_t get_type, hid_t dxpl_id, void **req, va_list arguments)
{
    printf("------- H5VL_prov_datatype_get, get input dt = %p\n", (void *)dt);
    H5VL_prov_datatype_t *o = (H5VL_prov_datatype_t *)dt;//init dt?
    if(!dt){
        printf("------- H5VL_prov_datatype_get: input dt is null. wellcome a seg fault \n");
        printf("------- H5VL_prov_datatype_get: o->datatype_get_cnt = ", o->datatype_get_cnt);
    }
    herr_t ret_value;
    unsigned long start = get_time_usec();
    ret_value = H5VLdatatype_get(o->under_object, native_driver_id, get_type, dxpl_id, req, arguments);
    printf("------- LOG datatype get: before +1: o->datatype_get_cnt = %d\n", o->datatype_get_cnt);
    o->datatype_get_cnt++;
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG datatype get\n");
    return ret_value;
}

static herr_t
H5VL_prov_datatype_close(void *dt, hid_t dxpl_id, void **req)
{
    H5VL_prov_datatype_t *type = (H5VL_prov_datatype_t *)dt;
    if(!dt)
        printf("------- H5VL_prov_datatype_close: warning, dt is a null pointer \n");
    assert(type->under_object);
    unsigned long start = get_time_usec();
    stat_write_datatype(type);
    H5VLdatatype_close(type->under_object, native_driver_id, dxpl_id, req);
    printf("------- LOG H5Tclose 0\n");
    //stat_write_datatype(type);
    printf("------- LOG H5Tclose 1\n");
    free(type);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Tclose 2\n");
    return 1;
}

static void *
H5VL_prov_object_open(void *obj, H5VL_loc_params_t loc_params, H5I_type_t *opened_type, hid_t dxpl_id, void **req)
{
    H5VL_prov_t *new_obj;
    H5VL_prov_t *o = (H5VL_prov_t *)obj;
    unsigned long start = get_time_usec();
    new_obj = (H5VL_prov_t *)calloc(1, sizeof(H5VL_prov_t));

    new_obj->under_object = H5VLobject_open(o->under_object, loc_params, native_driver_id, opened_type, dxpl_id, req);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Oopen, return %p\n", (void *)new_obj);
    return (void *)new_obj;
}

static herr_t
H5VL_prov_object_specific(void *obj, H5VL_loc_params_t loc_params, H5VL_object_specific_t specific_type,
                         hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_prov_t *o = (H5VL_prov_t *)obj;
    unsigned long start = get_time_usec();
    H5VLobject_specific(o->under_object, loc_params, native_driver_id, specific_type, dxpl_id, req, arguments);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG Object specific\n");
    return 1;
}

//patch, not call from outside.
static void dataset_get_wrapper(void *dset, hid_t driver_id, H5VL_dataset_get_t get_type,
        hid_t dxpl_id, void **req, ...){
    va_list args;
    va_start(args, req);
    H5VLdataset_get(dset, driver_id, get_type, dxpl_id, req, args);
}

static void *
H5VL_prov_dataset_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_prov_dataset_t *dset;
    H5VL_prov_dataset_t *o = (H5VL_prov_dataset_t *)obj;
    unsigned long start = get_time_usec();
    dset = (H5VL_prov_dataset_t *)calloc(1, sizeof(H5VL_prov_dataset_t));

    dset->under_object = H5VLdataset_create(o->under_object, loc_params, native_driver_id, name, dcpl_id,  dapl_id, dxpl_id, req);

    dset->dset_name = strdup(name);
    dset->dataset_read_cnt = 0;
    dset->dataset_write_cnt = 0;

    hid_t dset_type;


    //H5VLdataset_get(dset->under_object,native_driver_id, H5VL_DATASET_GET_TYPE, dxpl_id, req, &dset_type);
    dataset_get_wrapper(dset->under_object,native_driver_id, H5VL_DATASET_GET_TYPE, dxpl_id, req, &dset_type);

    dset->dset_type_size = H5Tget_size(dset_type);

    H5Tclose(dset_type);

    hid_t dset_space;
    dataset_get_wrapper(dset->under_object,native_driver_id, H5VL_DATASET_GET_SPACE, dxpl_id, req, &dset_space);

    dset->dset_space_size = H5Sget_simple_extent_npoints(dset_space);
    printf("dset_space size = %llu\n", dset->dset_space_size);
    H5Sclose(dset_space);


    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Dcreate\n");
    return (void *)dset;
}



static void *
H5VL_prov_dataset_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_prov_dataset_t *dset;
    H5VL_prov_dataset_t *o = (H5VL_prov_dataset_t *)obj;
    unsigned long start = get_time_usec();
    dset = (H5VL_prov_dataset_t *)calloc(1, sizeof(H5VL_prov_dataset_t));

    dset->under_object = H5VLdataset_open(o->under_object, loc_params, native_driver_id, name, dapl_id, dxpl_id, req);

//    id_t           ret_value;
//    H5Tget_size(dset_type)
//    H5Tlose(dset_tyep)
//    &dset_type
//    hid_t dset_type;
    hid_t dset_type;
    dataset_get_wrapper(dset->under_object,native_driver_id, H5VL_DATASET_GET_TYPE, dxpl_id, req, &dset_type);

    dset->dset_type_size = H5Tget_size(dset_type);

    H5Tclose(dset_type);

    hid_t dset_space;
    dataset_get_wrapper(dset->under_object,native_driver_id, H5VL_DATASET_GET_SPACE, dxpl_id, req, &dset_space);

    dset->dset_space_size = H5Sget_simple_extent_npoints(dset_space);
    printf("dset_space size = %llu\n", dset->dset_space_size);
    H5Sclose(dset_space);



    dset->dset_name = strdup(name);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Dopen\n");
    return (void *)dset;
}

static herr_t
H5VL_prov_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                      hid_t file_space_id, hid_t plist_id, void *buf, void **req)
{
    H5VL_prov_dataset_t *d = (H5VL_prov_dataset_t *)dset;
    unsigned long start = get_time_usec();
    H5VLdataset_read(d->under_object, native_driver_id, mem_type_id, mem_space_id, file_space_id,
                     plist_id, buf, req);
    d->dataset_read_cnt++;
    printf("read mem_space_id ========================  %llx\n", mem_space_id);
    if(H5S_ALL == mem_space_id){
        d->tatal_bytes_read += d->dset_type_size * d->dset_space_size;
    }else
        d->tatal_bytes_read += d->dset_type_size * H5Sget_select_npoints(mem_space_id);
    unsigned long t = get_time_usec() - start;
    d->tatal_read_ns += t;
    prov_write(global_prov_helper, __func__, t);
    printf("------- LOG H5Dread\n");
    return 1;
}
static herr_t
H5VL_prov_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
                       hid_t file_space_id, hid_t plist_id, const void *buf, void **req)
{
    H5VL_prov_dataset_t *d = (H5VL_prov_dataset_t *)dset;
    unsigned long start = get_time_usec();
    H5VLdataset_write(d->under_object, native_driver_id, mem_type_id, mem_space_id, file_space_id,
                     plist_id, buf, req);
    d->dataset_write_cnt++;
    printf(" write mem_space_id ========================  %llx\n", mem_space_id);
    if(H5S_ALL == mem_space_id){
        d->tatal_bytes_written += d->dset_type_size * d->dset_space_size;
    }else
        d->tatal_bytes_written += d->dset_type_size * H5Sget_select_npoints(mem_space_id);
    unsigned long t = get_time_usec() - start;
    d->tatal_write_ns += t;
    prov_write(global_prov_helper, __func__, t);

    printf("------- LOG H5Dwrite\n");
    return 1;
}
static herr_t
H5VL_prov_dataset_close(void *dset, hid_t dxpl_id, void **req)
{
    H5VL_prov_dataset_t *d = (H5VL_prov_dataset_t *)dset;
    unsigned long start = get_time_usec();
    H5VLdataset_close(d->under_object, native_driver_id, dxpl_id, req);
    stat_write_dataset(d);
    free(d);
    prov_write(global_prov_helper, __func__, get_time_usec() - start);
    printf("------- LOG H5Dclose\n");
    return 1;
}

#if 0
static void *H5VL_prov_attr_create(void *obj, H5VL_loc_params_t loc_params, const char *attr_name, hid_t acpl_id, hid_t aapl_id, hid_t dxpl_id, void **req){
static herr_t H5VL_prov_attr_close(void *attr, hid_t dxpl_id, void **req){

/* Datatype callbacks */


/* Dataset callbacks */
static void *H5VL_prov_dataset_create(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req){
static herr_t H5VL_prov_dataset_close(void *dset, hid_t dxpl_id, void **req){

/* File callbacks */


static void *H5VL_prov_file_open(const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req){


/* Group callbacks */

static void *H5VL_prov_group_open(void *obj, H5VL_loc_params_t loc_params, const char *name, hid_t gapl_id, hid_t dxpl_id, void **req){
static herr_t H5VL_prov_group_get(void *obj, H5VL_group_get_t get_type, hid_t dxpl_id, void **req, va_list arguments){


/* Link callbacks */

/* Object callbacks */


#endif



