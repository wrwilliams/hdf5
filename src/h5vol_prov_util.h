/*
 * h5vol_prov_util.h
 *
 *  Created on: Jan 9, 2019
 *      Author: tonglin
 */

#ifndef SRC_H5VOL_PROV_UTIL_H_
#define SRC_H5VOL_PROV_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include "hdf5.h"

#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <time.h>
//
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
//
//void prov_helper_init(prov_helper* helper, char* file_path, Prov_level prov_level, char* prov_line_format);
//
//void prov_helper_teardown(prov_helper* helper);
//
//void get_time_str(char *str_out);
//
//unsigned long get_time_usec() ;
//
//static int prov_write(prov_helper* helper_in, char* msg, unsigned long duration);
//
//
//#endif /* SRC_H5VOL_PROV_UTIL_H_ */
