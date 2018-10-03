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

/*-------------------------------------------------------------------------
 *
 * Created:	H5Ddeprec.c
 *		April 5 2007
 *		Quincey Koziol <koziol@hdfgroup.org>
 *
 * Purpose:	Deprecated functions from the H5D interface.  These
 *              functions are here for compatibility purposes and may be
 *              removed in the future.  Applications should switch to the
 *              newer APIs.
 *
 *-------------------------------------------------------------------------
 */

/****************/
/* Module Setup */
/****************/

#include "H5Dmodule.h"          /* This source code file is part of the H5D module */


/***********/
/* Headers */
/***********/
#include "H5private.h"		/* Generic Functions			*/
#include "H5CXprivate.h"        /* API Contexts                         */
#include "H5Dpkg.h"		/* Datasets 				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Iprivate.h"		/* IDs			  		*/
#include "H5VLprivate.h"	/* VOL plugins				*/


/****************/
/* Local Macros */
/****************/


/******************/
/* Local Typedefs */
/******************/


/********************/
/* Package Typedefs */
/********************/


/********************/
/* Local Prototypes */
/********************/


/*********************/
/* Package Variables */
/*********************/


/*****************************/
/* Library Private Variables */
/*****************************/


/*******************/
/* Local Variables */
/*******************/


#ifndef H5_NO_DEPRECATED_SYMBOLS

/*-------------------------------------------------------------------------
 * Function:    H5Dcreate1
 *
 * Purpose:     Creates a new dataset named NAME at LOC_ID, opens the
 *              dataset for access, and associates with that dataset constant
 *              and initial persistent properties including the type of each
 *              datapoint as stored in the file (TYPE_ID), the size of the
 *              dataset (SPACE_ID), and other initial miscellaneous
 *              properties (DCPL_ID).
 *
 *              All arguments are copied into the dataset, so the caller is
 *              allowed to derive new types, data spaces, and creation
 *              parameters from the old ones and reuse them in calls to
 *              create other datasets.
 *
 * Return:      Success:    The object ID of the new dataset. At this
 *                          point, the dataset is ready to receive its
 *                          raw data. Attempting to read raw data from
 *                          the dataset will probably return the fill
 *                          value. The dataset should be closed when
 *                          the caller is no longer interested in it.
 *
 *              Failure:    H5I_INVALID_HID
 *
 * Programmer:	Robb Matzke
 *		Wednesday, December  3, 1997
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dcreate1(hid_t loc_id, const char *name, hid_t type_id, hid_t space_id,
	  hid_t dcpl_id)
{
    void           *dset = NULL;                        /* dset token from VOL plugin */
    H5VL_object_t  *vol_obj = NULL;                     /* object token of loc_id */
    H5VL_loc_params_t loc_params;
    H5P_genplist_t *plist;                              /* Property list pointer */
    hid_t           lcpl_id = H5P_LINK_CREATE_DEFAULT;
    hid_t           dapl_id = H5P_DEFAULT;              /* DAPL used by library */
    hid_t           ret_value = H5I_INVALID_HID;        /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE5("i", "i*siii", loc_id, name, type_id, space_id, dcpl_id);

    /* Check arguments */
    if(!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if(!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    if(H5P_DEFAULT == dcpl_id)
        dcpl_id = H5P_DATASET_CREATE_DEFAULT;
    else
        if(TRUE != H5P_isa_class(dcpl_id, H5P_DATASET_CREATE))
            HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "not dataset create property list ID")

    /* Verify access property list and set up collective metadata if appropriate */
    if(H5CX_set_apl(&dapl_id, H5P_CLS_DACC, loc_id, TRUE) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, H5I_INVALID_HID, "can't set access property list info")

    /* Get the plist structure */
    if(NULL == (plist = (H5P_genplist_t *)H5I_object(dcpl_id)))
        HGOTO_ERROR(H5E_ATOM, H5E_BADATOM, H5I_INVALID_HID, "can't find object for ID")

    /* set creation properties */
    if(H5P_set(plist, H5VL_PROP_DSET_TYPE_ID, &type_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5I_INVALID_HID, "can't set property value for datatype id")
    if(H5P_set(plist, H5VL_PROP_DSET_SPACE_ID, &space_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5I_INVALID_HID, "can't set property value for space id")
    if(H5P_set(plist, H5VL_PROP_DSET_LCPL_ID, &lcpl_id) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, H5I_INVALID_HID, "can't set property value for lcpl id")

    /* Set location parameters */
    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the location object */
    if(NULL == (vol_obj = H5VL_get_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Create the dataset through the VOL */
    if(NULL == (dset = H5VL_dataset_create(vol_obj->data, loc_params, vol_obj->driver->cls, name, dcpl_id, 
                                           H5P_DATASET_ACCESS_DEFAULT, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTINIT, H5I_INVALID_HID, "unable to create dataset")

    /* Get an atom for the dataset */
    if((ret_value = H5VL_register_id(H5I_DATASET, dset, vol_obj->driver, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "unable to register dataset")

done:
    if (H5I_INVALID_HID == ret_value)
        if(dset && H5VL_dataset_close(dset, vol_obj->driver->cls, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dcreate1() */


/*-------------------------------------------------------------------------
 * Function:    H5Dopen1
 *
 * Purpose:     Finds a dataset named NAME at LOC_ID, opens it, and returns
 *              its ID. The dataset should be close when the caller is no
 *              longer interested in it.
 *
 * Note:        Deprecated in favor of H5Dopen2
 *
 * Return:      Success:    A new dataset ID
 *              Failure:    H5I_INVALID_HID
 *
 * Programmer:	Robb Matzke
 *		Thursday, December  4, 1997
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Dopen1(hid_t loc_id, const char *name)
{
    void         *dset = NULL;      /* dset token from VOL plugin */
    H5VL_object_t *vol_obj = NULL;      /* object token of loc_id */
    H5VL_loc_params_t loc_params;
    hid_t        ret_value = H5I_INVALID_HID;         /* Return value */

    FUNC_ENTER_API(H5I_INVALID_HID)
    H5TRACE2("i", "i*s", loc_id, name);

    /* Check args */
    if (!name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be NULL")
    if (!*name)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, H5I_INVALID_HID, "name parameter cannot be an empty string")

    /* Set location parameters */
    loc_params.type = H5VL_OBJECT_BY_SELF;
    loc_params.obj_type = H5I_get_type(loc_id);

    /* get the location object */
    if(NULL == (vol_obj = H5VL_get_object(loc_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, H5I_INVALID_HID, "invalid location identifier")

    /* Open the dataset */
    if(NULL == (dset = H5VL_dataset_open(vol_obj->data, loc_params, vol_obj->driver->cls, name, 
                                         H5P_DATASET_ACCESS_DEFAULT, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTOPENOBJ, H5I_INVALID_HID, "unable to open dataset")

    /* Get an atom for the dataset */
    if((ret_value = H5VL_register_id(H5I_DATASET, dset, vol_obj->driver, TRUE)) < 0)
        HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, H5I_INVALID_HID, "can't register dataset atom")

done:
    if(H5I_INVALID_HID == ret_value)
        if(dset && H5VL_dataset_close(dset, vol_obj->driver->cls, H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL) < 0)
            HDONE_ERROR(H5E_DATASET, H5E_CLOSEERROR, H5I_INVALID_HID, "unable to release dataset")

    FUNC_LEAVE_API(ret_value)
} /* end H5Dopen1() */


/*-------------------------------------------------------------------------
 * Function:    H5Dextend
 *
 * Purpose:     This function makes sure that the dataset is at least of size
 *              SIZE. The dimensionality of SIZE is the same as the data
 *              space of the dataset being changed.
 *
 * Note:        Deprecated in favor of H5Dset_extent
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:	Robb Matzke
 *		Friday, January 30, 1998
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Dextend(hid_t dset_id, const hsize_t size[])
{
    H5VL_object_t  *vol_obj = NULL;             /* Dataset structure */
    hid_t           sid = H5I_INVALID_HID;      /* Dataspace ID */
    H5S_t          *ds = NULL;                  /* Dataspace struct */
    int             ndims;                      /* Dataset/space rank */
    hsize_t         dset_dims[H5S_MAX_RANK];    /* Current dataset dimensions */
    int             i;                          /* Local index variable */
    herr_t          ret_value = SUCCEED;        /* Return value */

    FUNC_ENTER_API(FAIL)
    H5TRACE2("e", "i*h", dset_id, size);

    /* Check args */
    if(NULL == (vol_obj = (H5VL_object_t *)H5I_object_verify(dset_id, H5I_DATASET)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid dataset identifier")
    if(!size)
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no size specified")

    /* Get the dataspace pointer for the dataset */
    if(H5VL_dataset_get(vol_obj->data, vol_obj->driver->cls, H5VL_DATASET_GET_SPACE, 
                        H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, &sid) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "unable to get dataspace")
    if(H5I_INVALID_HID == sid)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "received an invalid dataspace from the dataset")
    if(NULL == (ds = (H5S_t *)H5I_object_verify(sid, H5I_DATASPACE)))
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "couldn't get dataspace structure from ID")

    /* Get the dataset's current extent */
    if(H5S_get_simple_extent_dims(ds, dset_dims, NULL) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTGET, FAIL, "can't get dataset dimensions")

    /* Get the dataset dimensions */
    ndims = H5S_GET_EXTENT_NDIMS(ds);

    /* Make certain that the dataset dimensions don't decrease in any dimension.
     *
     * (Shrinking dimensions is possible with H5Dset_extent, but not H5Dextend)
     *
     * XXX (VOL_MERGE): I feel like we should fail here instead of just silently
     *                  not doing what we're supposed to do.
     */
    for(i = 0; i < ndims; i++)
        if(size[i] > dset_dims[i])
            dset_dims[i] = size[i];

    /* Set up collective metadata if appropriate */
    if(H5CX_set_loc(dset_id) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "can't set collective metadata read info")

    /* Increase size */
    if ((ret_value = H5VL_dataset_specific(vol_obj->data, vol_obj->driver->cls, H5VL_DATASET_SET_EXTENT, 
                                          H5P_DATASET_XFER_DEFAULT, H5_REQUEST_NULL, dset_dims)) < 0)
        HGOTO_ERROR(H5E_DATASET, H5E_CANTSET, FAIL, "unable to extend dataset")

done:
    FUNC_LEAVE_API(ret_value)
} /* end H5Dextend() */
#endif /* H5_NO_DEPRECATED_SYMBOLS */

