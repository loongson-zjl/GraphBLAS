//------------------------------------------------------------------------------
// GB_mex_about7: still more basic tests
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Test lots of random stuff.  The function otherwise serves no purpose.

#include "GB_mex.h"
#include "GB_mex_errors.h"

#define USAGE "GB_mex_about7"
#define FREE_ALL ;
#define GET_DEEP_COPY ;
#define FREE_DEEP_COPY ;

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    GrB_Info info ;
    GrB_Matrix A = NULL, B = NULL, Y_mangled = NULL, Y_save = NULL ;

    //--------------------------------------------------------------------------
    // startup GraphBLAS
    //--------------------------------------------------------------------------

    bool malloc_debug = GB_mx_get_global (true) ;
    int expected = GrB_SUCCESS ;

    //--------------------------------------------------------------------------
    // matrix check
    //--------------------------------------------------------------------------

    OK (GrB_Matrix_new (&A, GrB_FP64, 100, 100)) ;
    OK (GxB_Matrix_Option_set (A, GxB_SPARSITY_CONTROL, GxB_HYPERSPARSE)) ;
    OK (GrB_Matrix_setElement_FP64 (A, (double) 1.2, 0, 0)) ;
    OK (GrB_Matrix_wait (A, 1)) ;
    OK (GxB_Matrix_fprint (A, "A valid", 3, NULL)) ;

    printf ("\ninvalid A->p:\n") ;
    size_t save = A->p_size ;
    A->p_size = 3 ;
    expected = GrB_INVALID_OBJECT ;
    ERR (GxB_Matrix_fprint (A, "A with invalid A->p", 3, NULL)) ;
    A->p_size = save ;

    printf ("\ninvalid A->h:\n") ;
    save = A->h_size ;
    A->h_size = 3 ;
    ERR (GxB_Matrix_fprint (A, "A with invalid A->h", 3, NULL)) ;
    A->h_size = save ;

    printf ("\ninvalid A->Y:\n") ;
    A->Y->magic = GB_MAGIC2 ;
    ERR (GxB_Matrix_fprint (A, "A with invalid A->Y", 3, NULL)) ;
    A->Y->magic = GB_MAGIC ;

    OK (GrB_Matrix_new (&Y_mangled, GrB_FP64, 100, 100)) ;
    OK (GrB_Matrix_free (&(A->Y))) ;
    A->Y = Y_mangled ;
    ERR (GxB_Matrix_fprint (A, "A with invalid A->Y (wrong type)", 3, NULL)) ;

    OK (GrB_Matrix_free (&A)) ;

    OK (GrB_Matrix_new (&A, GrB_FP64, 100, 100)) ;
    OK (GxB_Matrix_Option_set (A, GxB_SPARSITY_CONTROL, GxB_SPARSE)) ;
    OK (GrB_Matrix_setElement_FP64 (A, (double) 1.2, 0, 0)) ;
    OK (GrB_Matrix_wait (A, 1)) ;
    OK (GxB_Matrix_fprint (A, "A valid (sparse)", 3, NULL)) ;

    A->Y = Y_mangled ;
    ERR (GxB_Matrix_fprint (A, "A with invalid A->Y (not hyper)", 3, NULL)) ;

    OK (GrB_Matrix_free (&Y_mangled)) ;
    OK (GrB_Matrix_free (&A)) ;

    OK (GrB_Matrix_new (&A, GrB_FP64, 100, 100)) ;
    OK (GxB_Matrix_Option_set (A, GxB_SPARSITY_CONTROL, GxB_HYPERSPARSE)) ;
    OK (GrB_Matrix_setElement_FP64 (A, (double) 1.2, 0, 0)) ;
    OK (GrB_Matrix_wait (A, 1)) ;
    OK (GxB_Matrix_fprint (A, "A valid (hypersparse)", 3, NULL)) ;

    OK (GrB_Matrix_new (&B, GrB_FP64, 100, 100)) ;
    OK (GxB_Matrix_Option_set (B, GxB_SPARSITY_CONTROL, GxB_HYPERSPARSE)) ;
    OK (GrB_Matrix_setElement_FP64 (B, (double) 1.2, 0, 0)) ;
    OK (GrB_Matrix_wait (B, 1)) ;
    OK (GrB_Matrix_free (&(B->Y))) ;
    B->Y = A->Y ;
    B->Y_shallow = true ;
    OK (GxB_Matrix_fprint (B, "B valid (shallow hypersparse)", 3, NULL)) ;
    CHECK (GB_aliased (A, B)) ;
    OK (GrB_Matrix_free (&B)) ;

    OK (GxB_Matrix_fprint (A, "A still valid (hypersparse)", 3, NULL)) ;

    A->Y->i [0] = 99 ;
    ERR (GxB_Matrix_fprint (A, "A->Y invalid (not found) ", 3, NULL)) ;
    A->Y->i [0] = 0 ;

    int64_t *Yx = A->Y->x ;
    Yx [0] = 99 ;
    ERR (GxB_Matrix_fprint (A, "A->Y invalid (wrong k) ", 3, NULL)) ;

    OK (GrB_Matrix_free (&A)) ;

    //--------------------------------------------------------------------------
    // axv2 and avx512f
    //--------------------------------------------------------------------------

    bool have_avx2 = GB_Global_cpu_features_avx2 ( ) ;
    bool have_avx512f = GB_Global_cpu_features_avx512f ( ) ;
    printf ("\navx2: %d avx512f: %d\n", have_avx2, have_avx512f) ;

    //--------------------------------------------------------------------------
    // compiler
    //--------------------------------------------------------------------------

    char *compiler ;
    int compiler_version [3] ;
    OK (GxB_Global_Option_get (GxB_COMPILER_NAME, &compiler)) ;
    OK (GxB_Global_Option_get (GxB_COMPILER_VERSION, compiler_version)) ;
    printf ("GraphBLAS compiled with:\n[%s] [v%d.%d.%d]\n", compiler,
        compiler_version [0], compiler_version [1], compiler_version [2]) ;

    //--------------------------------------------------------------------------
    // iterator errors
    //--------------------------------------------------------------------------

    GxB_Iterator iterator = NULL ;
    METHOD (GxB_Iterator_new (&iterator)) ;
    GxB_Iterator_free (&iterator) ;

    //--------------------------------------------------------------------------
    // wrapup
    //--------------------------------------------------------------------------

    GB_mx_put_global (true) ;   
    printf ("\nGB_mex_about7: all tests passed\n\n") ;
}

