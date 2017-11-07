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
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Monday, August  2, 1999
 *
 * Purpose:	The public header file for the ros3 driver.
 */
#ifndef H5FDros3_H
#define H5FDros3_H

#define H5FD_ROS3	(H5FD_ros3_init())

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 *
 * structure H5FD_s3_config_t
 *
 * H5FD_s3_config_t is a public structure that is used to pass S3 
 * configuration data to the appropriate S3 VFD via the FAPL.  A 
 * pointer to an instance of this structure is a parameter in the 
 * H5Pset_s3_config() and H5Pget_s3_config() API calls.
 *
 * The fields of the structure are discussed individually below:
 *  
 * version: Integer field containing the version number of this version
 *      of the H5FD_s3_config_t structure.  Any instance of
 *      H5FD_s3_config_t passed to the above calls must have a known
 *      version number, or an error will be flagged.
 *
 *      This field should be set to H5FD__CURR_ROS3_FAPL_T_VERSION.
 *
 * authenticate:
 *
 *     Flag TRUE or FALSE whether or not requests are to be authenticated
 *     with the AWS4 algorithm. 
 *     If TRUE, `aws_region`, `secret_id`, and `secret_key` must be populated. 
 *     If FALSE, those three components are unused and may be NULL.
 *
 * aws_region:
 *
 *     String identifier of the AWS "region" of the host, e.g. "us-east-1".
 *     TODO: listing or reference to listing.
 *
 * secret_id:
 *
 *     Access ID for the resource. (string)
 *
 * secret_key:
 *
 *     Access key for the id/resource/host/whatever. (string)
 *
 ****************************************************************************/

#define H5FD__CURR_ROS3_FAPL_T_VERSION     1

#define H5FD__ROS3_MAX_REGION_LEN         32
#define H5FD__ROS3_MAX_SECRET_ID_LEN     128
#define H5FD__ROS3_MAX_SECRET_KEY_LEN    128

typedef struct H5FD_ros3_fapl_t {
    int32_t version;
    hbool_t authenticate;
    char    aws_region[H5FD__ROS3_MAX_REGION_LEN + 1];
    char    secret_id[H5FD__ROS3_MAX_SECRET_ID_LEN + 1];
    char    secret_key[H5FD__ROS3_MAX_SECRET_KEY_LEN + 1];
} H5FD_ros3_fapl_t;


H5_DLL hid_t H5FD_ros3_init(void);
H5_DLL herr_t H5Pget_fapl_ros3(hid_t fapl_id, H5FD_ros3_fapl_t * fa_out);
H5_DLL herr_t H5Pset_fapl_ros3(hid_t fapl_id, H5FD_ros3_fapl_t * fa);

#ifdef __cplusplus
}
#endif

#endif /* ifndef H5FDros3_H */


