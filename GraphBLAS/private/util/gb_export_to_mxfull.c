//------------------------------------------------------------------------------
// gb_export_to_mxfull: export a dense array to a MATLAB dense matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// The input (void *) X is exported to a MATLAB dense mxArray S.

// The input array must be deep.  The output is a standard
// MATLAB dense matrix as an mxArray.  No typecasting is done.

#include "gb_matlab.h"

mxArray *gb_export_to_mxfull    // return exported MATLAB dense matrix F
(
    void **X_handle,            // pointer to array to export
    const GrB_Index nrows,      // dimensions of F
    const GrB_Index ncols,
    GrB_Type type               // type of the array
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    CHECK_ERROR (X_handle == NULL || *X_handle == NULL || type == NULL,
        "internal error") ;

    //--------------------------------------------------------------------------
    // allocate an empty dense matrix of the right type, then set content
    //--------------------------------------------------------------------------

    mxArray *F ;
    void *X = (*X_handle) ;

    if (type == GrB_BOOL)
    {
        F = mxCreateLogicalMatrix (0, 0) ;
        mxSetData (F, X) ;
    }
    else if (type == GrB_FP32)
    {
        F = mxCreateNumericMatrix (0, 0, mxSINGLE_CLASS, mxREAL) ;
        mxSetSingles (F, X) ;
    }
    else if (type == GrB_FP64)
    {
        F = mxCreateNumericMatrix (0, 0, mxDOUBLE_CLASS, mxREAL) ;
        mxSetDoubles (F, X) ;
    }
    else if (type == GrB_INT8)
    {
        F = mxCreateNumericMatrix (0, 0, mxINT8_CLASS, mxREAL) ;
        mxSetInt8s (F, X) ;
    }
    else if (type == GrB_INT16)
    {
        F = mxCreateNumericMatrix (0, 0, mxINT16_CLASS, mxREAL) ;
        mxSetInt16s (F, X) ;
    }
    else if (type == GrB_INT32)
    {
        F = mxCreateNumericMatrix (0, 0, mxINT32_CLASS, mxREAL) ;
        mxSetInt32s (F, X) ;
    }
    else if (type == GrB_INT64)
    {
        F = mxCreateNumericMatrix (0, 0, mxINT64_CLASS, mxREAL) ;
        mxSetInt64s (F, X) ;
    }
    else if (type == GrB_UINT8)
    {
        F = mxCreateNumericMatrix (0, 0, mxUINT8_CLASS, mxREAL) ;
        mxSetUint8s (F, X) ;
    }
    else if (type == GrB_UINT16)
    {
        F = mxCreateNumericMatrix (0, 0, mxUINT16_CLASS, mxREAL) ;
        mxSetUint16s (F, X) ;
    }
    else if (type == GrB_UINT32)
    {
        F = mxCreateNumericMatrix (0, 0, mxUINT32_CLASS, mxREAL) ;
        mxSetUint32s (F, X) ;
    }
    else if (type == GrB_UINT64)
    {
        F = mxCreateNumericMatrix (0, 0, mxUINT64_CLASS, mxREAL) ;
        mxSetUint64s (F, X) ;
    }
    #ifdef GB_COMPLEX_TYPE
    else if (type == gb_complex_type)
    {
        F = mxCreateNumericMatrix (0, 0, mxDOUBLE_CLASS, mxCOMPLEX) ;
        mxSetComplexDouble (F, X) ;
    }
    #endif
    else
    {
        ERROR ("unknown type") ;
    }

    // set the size
    mxSetM (F, nrows) ;
    mxSetN (F, ncols) ;

    // tell the caller that X is exported
    (*X_handle) = NULL ;

    //--------------------------------------------------------------------------
    // return the new MATLAB dense matrix
    //--------------------------------------------------------------------------

    return (F) ;
}

