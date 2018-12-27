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
 * Purpose:     Object callbacks for the native VOL connector
 *
 */

#define H5O_FRIEND              /* Suppress error about including H5Opkg    */
#define H5R_FRIEND              /* Suppress error about including H5Rpkg    */

#include "H5private.h"          /* Generic Functions                        */
#include "H5Eprivate.h"         /* Error handling                           */
#include "H5Fprivate.h"         /* Files                                    */
#include "H5Gprivate.h"         /* Groups                                   */
#include "H5Iprivate.h"         /* IDs                                      */
#include "H5Opkg.h"             /* Object headers                           */
#include "H5Pprivate.h"         /* Property lists                           */
#include "H5Rpkg.h"             /* References                               */
#include "H5VLprivate.h"        /* Virtual Object Layer                     */

#include "H5VLnative_private.h" /* Native VOL connector                     */


/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_open
 *
 * Purpose:     Handles the object open callback
 *
 * Return:      Success:    object pointer
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL__native_object_open(void *obj, const H5VL_loc_params_t *loc_params, H5I_type_t *opened_type, 
    hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t   loc;
    void       *ret_value = NULL;

    FUNC_ENTER_PACKAGE

    if(H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a file or file object")

    switch(loc_params->type) {
        case H5VL_OBJECT_BY_NAME:
            {
                /* Open the object */
                if(NULL == (ret_value = H5O_open_name(&loc, loc_params->loc_data.loc_by_name.name, opened_type)))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by name")
                break;
            }

        case H5VL_OBJECT_BY_IDX:
            {
                /* Open the object */
                if(NULL == (ret_value = H5O_open_by_idx(&loc, loc_params->loc_data.loc_by_idx.name, loc_params->loc_data.loc_by_idx.idx_type,
                        loc_params->loc_data.loc_by_idx.order, loc_params->loc_data.loc_by_idx.n, opened_type)))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by index")
                break;
            }

        case H5VL_OBJECT_BY_ADDR:
            {
                /* Open the object */
                if(NULL == (ret_value = H5O_open_by_addr(&loc, loc_params->loc_data.loc_by_addr.addr, opened_type)))
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTOPENOBJ, NULL, "unable to open object by address")
                break;
            }

        case H5VL_OBJECT_BY_REF:
            {
                hid_t temp_id = H5I_INVALID_HID;
                H5F_t *file = NULL;

                /* Get the file pointer from the entry */
                file = loc.oloc->file;

                /* Create reference */
                if((temp_id = H5R__dereference(file, loc_params->loc_data.loc_by_ref.lapl_id, 
                                              loc_params->loc_data.loc_by_ref.ref_type, 
                                              loc_params->loc_data.loc_by_ref._ref)) < 0)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_CANTOPENOBJ, NULL, "unable to dereference object")

                *opened_type = H5I_get_type(temp_id);
                if(NULL == (ret_value = H5I_remove(temp_id)))
                    HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, NULL, "unable to open object")
                break;
            }

        case H5VL_OBJECT_BY_SELF:
        default:
            HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, NULL, "unknown open parameters")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_copy
 *
 * Purpose:     Handles the object copy callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL__native_object_copy(void *src_obj, const H5VL_loc_params_t *loc_params1, const char *src_name, 
    void *dst_obj, const H5VL_loc_params_t *loc_params2, const char *dst_name, 
    hid_t ocpypl_id, hid_t lcpl_id, hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req)
{
    H5G_loc_t   src_loc;                /* Source object group location */
    H5G_loc_t   dst_loc;                /* Destination group location */
    herr_t      ret_value = FAIL;
    
    FUNC_ENTER_PACKAGE

    /* get location for objects */
    if(H5G_loc_real(src_obj, loc_params1->obj_type, &src_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")
    if(H5G_loc_real(dst_obj, loc_params2->obj_type, &dst_loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    /* Copy the object */
    if((ret_value = H5O_copy(&src_loc, src_name, &dst_loc, dst_name, ocpypl_id, lcpl_id)) < 0)
        HGOTO_ERROR(H5E_SYM, H5E_CANTCOPY, FAIL, "unable to copy object")    

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_copy() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_get
 *
 * Purpose:     Handles the object get callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_get(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_get_t get_type, 
    hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    herr_t      ret_value = SUCCEED;    /* Return value */
    H5G_loc_t   loc;                    /* Location of group */

    FUNC_ENTER_PACKAGE

    if(H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch(get_type) {
        /* H5Rget_region */
        case H5VL_REF_GET_REGION:
            {
                hid_t       *ret                    =  HDva_arg(arguments, hid_t *);
                H5R_type_t  H5_ATTR_UNUSED ref_type =  (H5R_type_t)HDva_arg(arguments, int); /* enum work-around */
                void        *ref                    =  HDva_arg(arguments, void *);
                H5S_t       *space = NULL;    /* Dataspace object */

                /* Get the dataspace with the correct region selected */
                if((space = H5R__get_region(loc.oloc->file, ref)) == NULL)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to retrieve region")

                /* Atomize */
                if((*ret = H5I_register(H5I_DATASPACE, space, TRUE)) < 0)
                    HGOTO_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL, "unable to register dataspace atom")

                break;
            }

        /* H5Rget_obj_type1/2 */
        case H5VL_REF_GET_TYPE:
            {
                H5O_type_t  *obj_type  =  HDva_arg(arguments, H5O_type_t *);
                H5R_type_t  ref_type   =  (H5R_type_t)HDva_arg(arguments, int); /* enum work-around */
                void        *ref       =  HDva_arg(arguments, void *);

                /* Get the object information */
                if(H5R__get_obj_type(loc.oloc->file, ref_type, ref, obj_type) < 0)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to determine object type")
                break;
            }

        /* H5Rget_name */
        case H5VL_REF_GET_NAME:
            {
                ssize_t     *ret       = HDva_arg(arguments, ssize_t *);
                char        *name      = HDva_arg(arguments, char *);
                size_t      size       = HDva_arg(arguments, size_t);
                H5R_type_t  ref_type   = (H5R_type_t)HDva_arg(arguments, int); /* enum work-around */
                void        *ref       = HDva_arg(arguments, void *);

                /* Get name */
                if((*ret = H5R__get_name(loc.oloc->file, ref_type, ref, name, size)) < 0)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_CANTGET, FAIL, "unable to determine object path")
                break;
            }

        /* H5Iget_name */
        case H5VL_ID_GET_NAME:
            {
                ssize_t *ret = HDva_arg(arguments, ssize_t *);
                char *name = HDva_arg(arguments, char *);
                size_t size = HDva_arg(arguments, size_t);

                /* Retrieve object's name */
                if((*ret = H5G_get_name(&loc, name, size, NULL)) < 0)
                    HGOTO_ERROR(H5E_ATOM, H5E_CANTGET, FAIL, "can't retrieve object name")

                break;
            }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't get this type of information from object")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_specific
 *
 * Purpose:     Handles the object specific callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_specific(void *obj, const H5VL_loc_params_t *loc_params, H5VL_object_specific_t specific_type, 
    hid_t H5_ATTR_UNUSED dxpl_id, void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5G_loc_t    loc;
    herr_t       ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    if(H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch(specific_type) {
        /* H5Oincr_refcount / H5Odecr_refcount */
        case H5VL_OBJECT_CHANGE_REF_COUNT:
            {
                int update_ref  = HDva_arg(arguments, int);
                H5O_loc_t  *oloc = loc.oloc;

                if(H5O_link(oloc, update_ref) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_LINKCOUNT, FAIL, "modifying object link count failed")

                break;
            }

        /* H5Oexists_by_name */
        case H5VL_OBJECT_EXISTS:
            {
                htri_t *ret = HDva_arg(arguments, htri_t *);

                if(loc_params->type == H5VL_OBJECT_BY_NAME) {
                    /* Check if the object exists */
                    if((*ret = H5G_loc_exists(&loc, loc_params->loc_data.loc_by_name.name)) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "unable to determine if '%s' exists", loc_params->loc_data.loc_by_name.name)
                } /* end if */
                else
                    HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown object exists parameters")
                break;
            }

        case H5VL_OBJECT_VISIT:
            {
                H5_index_t idx_type     = (H5_index_t)HDva_arg(arguments, int); /* enum work-around */
                H5_iter_order_t order   = (H5_iter_order_t)HDva_arg(arguments, int); /* enum work-around */
                H5O_iterate_t op        = HDva_arg(arguments, H5O_iterate_t);
                void *op_data           = HDva_arg(arguments, void *);
                unsigned fields         = HDva_arg(arguments, unsigned);

                /* Call internal object visitation routine */
                if(loc_params->type == H5VL_OBJECT_BY_SELF) {
                    /* H5Ovisit */
                    if((ret_value = H5O__visit(&loc, ".", idx_type, order, op, op_data, fields)) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
                } /* end if */
                else if(loc_params->type == H5VL_OBJECT_BY_NAME) {
                    /* H5Ovisit_by_name */
                    if((ret_value = H5O__visit(&loc, loc_params->loc_data.loc_by_name.name, idx_type, order, op, op_data, fields)) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_BADITER, FAIL, "object visitation failed")
                } /* end else-if */
                else
                    HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown object visit params");
                break;
            }

        case H5VL_OBJECT_FLUSH:
            {
                hid_t   oid     = HDva_arg(arguments, hid_t);

                /* Flush the object's metadata */
                if(H5O_flush(loc.oloc, oid) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTFLUSH, FAIL, "unable to flush object")

                break;
            }

        case H5VL_OBJECT_REFRESH:
            {
                hid_t                   oid         = HDva_arg(arguments, hid_t);
                H5O_loc_t              *oloc        = loc.oloc;

                /* Refresh the metadata */
                if(H5O_refresh_metadata(oid, *oloc) < 0)
                    HGOTO_ERROR(H5E_OHDR, H5E_CANTLOAD, FAIL, "unable to refresh object")

                break;
            }

        case H5VL_REF_CREATE:
            {
                void        *ref      = HDva_arg(arguments, void *);
                const char  *name     = HDva_arg(arguments, char *);
                H5R_type_t  ref_type  = (H5R_type_t)HDva_arg(arguments, int); /* enum work-around */
                hid_t       space_id  = HDva_arg(arguments, hid_t);
                H5S_t       *space = NULL;   /* Pointer to dataspace containing region */
                
                if(space_id != (-1) && (NULL == (space = (H5S_t *)H5I_object_verify(space_id, H5I_DATASPACE))))
                    HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataspace")

                /* Create reference */
                if(H5R__create(ref, &loc, name, ref_type, space) < 0)
                    HGOTO_ERROR(H5E_REFERENCE, H5E_CANTCREATE, FAIL, "unable to create reference")

                break;
            }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't recognize this operation type")
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL__native_object_optional
 *
 * Purpose:     Handles the object optional callback
 *
 * Return:      SUCCEED/FAIL
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5VL__native_object_optional(void *obj, hid_t H5_ATTR_UNUSED dxpl_id,
    void H5_ATTR_UNUSED **req, va_list arguments)
{
    H5VL_native_object_optional_t optional_type = HDva_arg(arguments, H5VL_native_object_optional_t);
    H5VL_loc_params_t *loc_params = HDva_arg(arguments, H5VL_loc_params_t *);
    H5G_loc_t   loc;                    /* Location of group */
    herr_t       ret_value = SUCCEED;    /* Return value */

    FUNC_ENTER_PACKAGE

    if(H5G_loc_real(obj, loc_params->obj_type, &loc) < 0)
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a file or file object")

    switch(optional_type) {
        /* H5Oget_info / H5Oget_info_by_name / H5Oget_info_by_idx */
        case H5VL_NATIVE_OBJECT_GET_INFO:
            {
                H5O_info_t  *obj_info = HDva_arg(arguments, H5O_info_t *);
                unsigned fields         = HDva_arg(arguments, unsigned);

                if(loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oget_info */
                    /* Retrieve the object's information */
                    if(H5G_loc_info(&loc, ".", obj_info, fields) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end if */
                else if(loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oget_info_by_name */
                    /* Retrieve the object's information */
                    if(H5G_loc_info(&loc, loc_params->loc_data.loc_by_name.name, obj_info, fields) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end else-if */
                else if(loc_params->type == H5VL_OBJECT_BY_IDX) { /* H5Oget_info_by_idx */
                    H5G_loc_t   obj_loc;                /* Location used to open group */
                    H5G_name_t  obj_path;               /* Opened object group hier. path */
                    H5O_loc_t   obj_oloc;               /* Opened object object location */

                    /* Set up opened group location to fill in */
                    obj_loc.oloc = &obj_oloc;
                    obj_loc.path = &obj_path;
                    H5G_loc_reset(&obj_loc);

                    /* Find the object's location, according to the order in the index */
                    if(H5G_loc_find_by_idx(&loc, loc_params->loc_data.loc_by_idx.name, 
                                           loc_params->loc_data.loc_by_idx.idx_type, 
                                           loc_params->loc_data.loc_by_idx.order, 
                                           loc_params->loc_data.loc_by_idx.n, &obj_loc/*out*/) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "group not found")

                    /* Retrieve the object's information */
                    if(H5O_get_info(obj_loc.oloc, obj_info, fields) < 0) {
                        H5G_loc_free(&obj_loc);
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTGET, FAIL, "can't retrieve object info")
                    } /* end if */

                    /* Release the object location */
                    if(H5G_loc_free(&obj_loc) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_CANTRELEASE, FAIL, "can't free location")
                } /* end else-if */
                else
                    HGOTO_ERROR(H5E_OHDR, H5E_UNSUPPORTED, FAIL, "unknown get info parameters")
                break;
            }

        /* H5Oget_comment / H5Oget_comment_by_name */
        case H5VL_NATIVE_OBJECT_GET_COMMENT:
            {
                char     *comment =  HDva_arg(arguments, char *);
                size_t   bufsize  =  HDva_arg(arguments, size_t);
                ssize_t  *ret     =  HDva_arg(arguments, ssize_t *);

                /* Retrieve the object's comment */
                if(loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oget_comment */
                    if((*ret = H5G_loc_get_comment(&loc, ".", comment/*out*/, bufsize)) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end if */
                else if(loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oget_comment_by_name */
                    if((*ret = H5G_loc_get_comment(&loc, loc_params->loc_data.loc_by_name.name, comment/*out*/, bufsize)) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end else-if */
                else
                    HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown set_coment parameters")
                break;
            }

        /* H5Oset_comment */
        case H5VL_NATIVE_OBJECT_SET_COMMENT:
            {
                const char    *comment  = HDva_arg(arguments, char *);

                if(loc_params->type == H5VL_OBJECT_BY_SELF) { /* H5Oset_comment */
                    /* (Re)set the object's comment */
                    if(H5G_loc_set_comment(&loc, ".", comment) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end if */
                else if(loc_params->type == H5VL_OBJECT_BY_NAME) { /* H5Oset_comment_by_name */
                    /* (Re)set the object's comment */
                    if(H5G_loc_set_comment(&loc, loc_params->loc_data.loc_by_name.name, comment) < 0)
                        HGOTO_ERROR(H5E_OHDR, H5E_NOTFOUND, FAIL, "object not found")
                } /* end else-if */
                else
                    HGOTO_ERROR(H5E_VOL, H5E_UNSUPPORTED, FAIL, "unknown set_coment parameters")
                break;
            }

        default:
            HGOTO_ERROR(H5E_VOL, H5E_CANTGET, FAIL, "can't perform this operation on object");       
    } /* end switch */

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5VL__native_object_optional() */

