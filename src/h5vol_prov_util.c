/*
 * h5vol_prov_util.c
 *
 *  Created on: Jan 9, 2019
 *      Author: tonglin
 */


/* Provenance functions */


#include "h5vol_prov_util.h"

//typedef enum ProvLevel {
//    Default, //no file write, only screen print
//    Print_only,
//    File_only,
//    File_and_print,
//    Level3,
//    Level4,
//    Disabled
//}Prov_level;
//
//typedef struct ProvenanceHelper{
//    char* prov_file_path;
//    FILE* prov_file_handle;
//    Prov_level prov_level;
//    char* prov_line_format;
//    char user_name[32];
//    int pid;
//    uint64_t tid;
//    char proc_name[64];
//}prov_helper;
//
//typedef enum ProvenanceOutputDST{
//    TEXT,
//    BINARY,
//    CSV
//}prov_out_dst;
//typedef struct ProvenanceFormat{
//    prov_out_dst dst_format;
//
//} prov_format;
//
//prov_helper* global_prov_helper;

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
