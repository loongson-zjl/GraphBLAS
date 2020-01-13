//------------------------------------------------------------------------------
// GB_dense_accum_sparse: C += A where C is dense and A is sparse
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// C and A must have the same vector dimension and vector length.
// TODO: the transposed case, C+=A' could easily be done.
// The parallelism used is identical to GB_AxB_colscale.

#include "GB_dense.h"
#ifndef GBCOMPACT
#include "GB_binop__include.h"
#endif

#define GB_FREE_WORK \
    GB_ek_slice_free (&pstart_slice, &kfirst_slice, &klast_slice, ntasks) ;

GrB_Info GB_dense_accum_sparse      // C += A where C is dense and A is sparse 
(
    GrB_Matrix C,                   // input/output matrix
    const GrB_Matrix A,             // input matrix
    const GrB_BinaryOp accum,       // operator to apply
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GrB_Info info ;
    ASSERT_MATRIX_OK (C, "C for C+=A", GB0) ;
    ASSERT_MATRIX_OK (A, "A for C+=A", GB0) ;
    ASSERT (!GB_PENDING (A)) ; ASSERT (!GB_ZOMBIES (A)) ;
    ASSERT (!GB_PENDING (C)) ; ASSERT (!GB_ZOMBIES (C)) ;
    ASSERT_BINARYOP_OK (accum, "accum for C+=A", GB0) ;
    ASSERT (A->vlen == C->vlen) ;
    ASSERT (A->vdim == C->vdim) ;

    //--------------------------------------------------------------------------
    // get the operator
    //--------------------------------------------------------------------------

    if (accum->opcode == GB_FIRST_opcode)
    {
        // nothing to do
        return (GrB_SUCCESS) ;
    }

    // C = accum (C,A) will be computed
    ASSERT (GB_Type_compatible (C->type, accum->ztype)) ;
    ASSERT (GB_Type_compatible (C->type, accum->xtype)) ;
    ASSERT (GB_Type_compatible (A->type, accum->ytype)) ;

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    int64_t anz   = GB_NNZ (A) ;
    int64_t anvec = A->nvec ;
    GB_GET_NTHREADS_MAX (nthreads_max, chunk, Context) ;
    int nthreads = GB_nthreads (anz + anvec, chunk, nthreads_max) ;
    int ntasks = (nthreads == 1) ? 1 : (32 * nthreads) ;
    ntasks = GB_IMIN (ntasks, anz) ;
    ntasks = GB_IMAX (ntasks, 1) ;

    //--------------------------------------------------------------------------
    // slice the entries for each task
    //--------------------------------------------------------------------------

    // Task tid does entries pstart_slice [tid] to pstart_slice [tid+1]-1 and
    // vectors kfirst_slice [tid] to klast_slice [tid].  The first and last
    // vectors may be shared with prior slices and subsequent slices.

    int64_t *pstart_slice = NULL, *kfirst_slice = NULL, *klast_slice = NULL ;
    if (!GB_ek_slice (&pstart_slice, &kfirst_slice, &klast_slice, A, ntasks))
    {
        // out of memory
        return (GB_OUT_OF_MEMORY) ;
    }

    //--------------------------------------------------------------------------
    // define the worker for the switch factory
    //--------------------------------------------------------------------------

    bool done = false ;

    #define GB_Cdense_accumA(accum,xyname) GB_Cdense_accumA_ ## accum ## xyname

    #define GB_BINOP_WORKER(accum,xyname)                                   \
    {                                                                       \
        info = GB_Cdense_accumA(accum,xyname) (C, A,                        \
            kfirst_slice, klast_slice, pstart_slice, ntasks, nthreads) ;    \
        done = (info != GrB_NO_VALUE) ;                                     \
    }                                                                       \
    break ;

    //--------------------------------------------------------------------------
    // launch the switch factory
    //--------------------------------------------------------------------------

    #ifndef GBCOMPACT

        GB_Opcode opcode ;
        GB_Type_code xycode, zcode ;
        if (GB_binop_builtin (C->type, false, A->type, false, accum, false,
            &opcode, &xycode, &zcode))
        { 
            // accumulate sparse matrix into dense matrix with built-in operator
            #include "GB_binop_factory.c"
        }

    #endif

    //--------------------------------------------------------------------------
    // C += A, sparse accum into dense, with typecasting or user-defined op
    //--------------------------------------------------------------------------

    if (!done)
    {

        //----------------------------------------------------------------------
        // get operators, functions, workspace, contents of A and C
        //----------------------------------------------------------------------

        GB_BURBLE_MATRIX (A, "generic ") ;

        GxB_binary_function fadd = accum->function ;

        size_t csize = C->type->size ;
        size_t asize = A->type->size ;
        size_t ysize = accum->ytype->size ;

        GB_cast_function cast_A ;

        // A is typecasted to y
        cast_A = GB_cast_factory (A->type->code, accum->ytype->code) ;

        //----------------------------------------------------------------------
        // C += A via function pointers, and typecasting
        //----------------------------------------------------------------------

        // aij = A(i,j), located in Ax [pA].  Note that GB_GETB is used,
        // since A appears as the 2nd input to z = fadd (x,y)
        // TODO rename this function C+=B and use A throughout, not B.
        #define GB_GETB(aij,Ax,pA)                                          \
            GB_void aij [GB_VLA(ysize)] ;                                   \
            cast_A (aij, Ax +((pA)*asize), asize)

        // C(i,j) = C(i,j) + A(i,j)
        #define GB_BINOP(cout_ij, cin_aij, aij)                             \
            GB_BINARYOP (cout_ij, cin_aij, aij)

        // address of Cx [p]
        #define GB_CX(p) Cx +((p)*csize)

        #define GB_ATYPE GB_void
        #define GB_CTYPE GB_void

        // no vectorization
        #define GB_PRAGMA_VECTORIZE

        #define GB_BINARYOP(z,x,y) fadd (z,x,y)
        #include "GB_dense_accum_sparse_template.c"
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    GB_FREE_WORK ;
    ASSERT_MATRIX_OK (C, "C+=A output", GB0) ;
    return (GrB_SUCCESS) ;
}

