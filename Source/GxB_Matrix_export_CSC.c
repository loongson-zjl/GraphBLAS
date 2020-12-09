//------------------------------------------------------------------------------
// GxB_Matrix_export_CSC: export a matrix in CSC format
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB_export.h"

#define GB_FREE_ALL ;

GrB_Info GxB_Matrix_export_CSC  // export and free a CSC matrix
(
    GrB_Matrix *A,      // handle of matrix to export and free
    GrB_Type *type,     // type of matrix exported
    GrB_Index *nrows,   // number of rows of the matrix
    GrB_Index *ncols,   // number of columns of the matrix

    GrB_Index **Ap,     // column "pointers", Ap_size >= ncols+1
    GrB_Index **Ai,     // row indices, Ai_size >= nvals(A)
    void **Ax,          // values, Ax_size 1, or >= nvals(A)
    GrB_Index *Ap_size, // size of Ap
    GrB_Index *Ai_size, // size of Ai
    GrB_Index *Ax_size, // size of Ax

    bool *jumbled,      // if true, indices in each column may be unsorted
    const GrB_Descriptor desc
)
{ 

    //--------------------------------------------------------------------------
    // check inputs and get the descriptor
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Matrix_export_CSC (&A, &type, &nrows, &ncols,"
        "&Ap, &Ai, &Ax, &Ap_size, &Ai_size, &Ax_size, &jumbled, desc)") ;
    GB_BURBLE_START ("GxB_Matrix_export_CSC") ;
    GB_RETURN_IF_NULL (A) ;
    GB_RETURN_IF_NULL_OR_FAULTY (*A) ;
    GB_GET_DESCRIPTOR (info, desc, xx1, xx2, xx3, xx4, xx5, xx6) ;
    ASSERT_MATRIX_OK (*A, "A to export as CSC", GB0) ;

    //--------------------------------------------------------------------------
    // ensure the matrix is in CSC format
    //--------------------------------------------------------------------------

    if (!((*A)->is_csc))
    { 
        // A = A', done in-place, to put A in CSC format
        GBURBLE ("(transpose) ") ;
        GB_OK (GB_transpose (NULL, NULL, true, *A,
            NULL, NULL, NULL, false, Context)) ;
    }

    //--------------------------------------------------------------------------
    // finish any pending work
    //--------------------------------------------------------------------------

    if (jumbled == NULL)
    { 
        // the exported matrix cannot be jumbled
        GB_MATRIX_WAIT (*A) ;
    }
    else
    { 
        // the exported matrix is allowed to be jumbled
        GB_MATRIX_WAIT_IF_PENDING_OR_ZOMBIES (*A) ;
    }

    //--------------------------------------------------------------------------
    // ensure the matrix is sparse
    //--------------------------------------------------------------------------

    GB_OK (GB_convert_any_to_sparse (*A, Context)) ;

    //--------------------------------------------------------------------------
    // export the matrix
    //--------------------------------------------------------------------------

    ASSERT (GB_IS_SPARSE (*A)) ;
    ASSERT (((*A)->is_csc)) ;
    ASSERT (!GB_ZOMBIES (*A)) ;
    ASSERT (GB_IMPLIES (jumbled == NULL, !GB_JUMBLED (*A))) ;
    ASSERT (!GB_PENDING (*A)) ;

    int sparsity ;
    bool is_csc ;

    info = GB_export (A, type, nrows, ncols,
        Ap,   Ap_size,  // Ap
        NULL, NULL,     // Ah
        NULL, NULL,     // Ab
        Ai,   Ai_size,  // Ai
        Ax,   Ax_size,  // Ax
        NULL, jumbled, NULL,                // jumbled or not
        &sparsity, &is_csc, Context) ;      // sparse by col

    if (info == GrB_SUCCESS)
    {
        ASSERT (sparsity == GxB_SPARSE) ;
        ASSERT (is_csc) ;
    }
    GB_BURBLE_END ;
    return (info) ;
}

