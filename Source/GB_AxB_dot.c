//------------------------------------------------------------------------------
// GB_AxB_dot: compute C<M> = A'*B without forming A' via dot products
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// GB_AxB_dot computes the matrix multiplication C<M>=A'*B without forming
// A' explicitly.  It is useful when A is very tall and thin (n-by-1 in
// particular).  In that case A' is costly to transpose, but A'*B is very
// easy if B is also tall and thin (say also n-by-1).

// If M is NULL, the method computes C=A'*B by considering each entry C(i,j),
// taking O(m*n) time if C is m-by-n.  This is suitable only when C is small
// (such as a scalar, a small matrix, or a vector).  If M is present, the upper
// bound on the number of entries in C is the same as nnz(M), so that space is
// allocated for C, and C(i,j) is computed only where M(i,j)=1.  This function
// assumes the mask M is not complemented.

// Compare this function with GB_AxB_Gustavson, which computes C=A*B and
// C<M>=A*B.  The computation of C=A*B requires C->p and C->i to be constructed
// first, in a symbolic phase.  Otherwise they are very similar.  The dot
// product in this algorithm is very much like the merge-add in GB_add,
// except that the merge in GB_add produces a column (a(:,j)+b(:,j)),
// whereas the merge in this function produces a scalar (a(:,j)'*b(:,j)).

// parallel: this function will remain sequential.
// parallelism will be done in GB_AxB_parallel.

// Does not log an error; returns GrB_SUCCESS, GrB_OUT_OF_MEMORY, or GrB_PANIC.

#include "GB.h"
#ifndef GBCOMPACT
#include "GB_AxB__semirings.h"
#endif

GrB_Info GB_AxB_dot                 // C = A'*B using dot product method
(
    GrB_Matrix *Chandle,            // output matrix
    const GrB_Matrix M,             // mask matrix for C<M>=A'*B or C<!M>=A'*B
    const bool Mask_comp,           // if true, use !M
    const GrB_Matrix A,             // input matrix
    const GrB_Matrix B,             // input matrix
    const GrB_Semiring semiring,    // semiring that defines C=A*B
    const bool flipxy,              // if true, do z=fmult(b,a) vs fmult(a,b)
    bool *mask_applied              // if true, mask was applied
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_Context Context = NULL ;
    ASSERT (Chandle != NULL) ;
    ASSERT_OK_OR_NULL (GB_check (M, "M for dot A'*B", GB0)) ;
    ASSERT_OK (GB_check (A, "A for dot A'*B", GB0)) ;
    ASSERT_OK (GB_check (B, "B for dot A'*B", GB0)) ;
    ASSERT (!GB_PENDING (M)) ; ASSERT (!GB_ZOMBIES (M)) ;
    ASSERT (!GB_PENDING (A)) ; ASSERT (!GB_ZOMBIES (A)) ;
    ASSERT (!GB_PENDING (B)) ; ASSERT (!GB_ZOMBIES (B)) ;
    ASSERT_OK (GB_check (semiring, "semiring for numeric A'*B", GB0)) ;
    ASSERT (A->vlen == B->vlen) ;
    ASSERT (mask_applied != NULL) ;

    //--------------------------------------------------------------------------
    // get the semiring operators
    //--------------------------------------------------------------------------

    GrB_BinaryOp mult = semiring->multiply ;
    GrB_Monoid add = semiring->add ;
    ASSERT (mult->ztype == add->op->ztype) ;

    bool op_is_first  = mult->opcode == GB_FIRST_opcode ;
    bool op_is_second = mult->opcode == GB_SECOND_opcode ;
    bool A_is_pattern = false ;
    bool B_is_pattern = false ;

    if (flipxy)
    { 
        // z = fmult (b,a) will be computed
        A_is_pattern = op_is_first  ;
        B_is_pattern = op_is_second ;
        if (!A_is_pattern) ASSERT (GB_Type_compatible (A->type, mult->ytype)) ;
        if (!B_is_pattern) ASSERT (GB_Type_compatible (B->type, mult->xtype)) ;
    }
    else
    { 
        // z = fmult (a,b) will be computed
        A_is_pattern = op_is_second ;
        B_is_pattern = op_is_first  ;
        if (!A_is_pattern) ASSERT (GB_Type_compatible (A->type, mult->xtype)) ;
        if (!B_is_pattern) ASSERT (GB_Type_compatible (B->type, mult->ytype)) ;
    }

    (*Chandle) = NULL ;

    // the dot method handles any mask, complemented or not complemented

    //--------------------------------------------------------------------------
    // estimate nnz(C) and allocate C
    //--------------------------------------------------------------------------

    if (B->nvec_nonempty < 0)
    { 
        B->nvec_nonempty = GB_nvec_nonempty (B, NULL) ;
    }

    if (A->nvec_nonempty < 0)
    { 
        A->nvec_nonempty = GB_nvec_nonempty (A, NULL) ;
    }

    // GxB_fprint (A, 2, stderr) ;
    // GxB_fprint (B, 2, stderr) ;

    bool Adense = false, Bdense = false ;
    GrB_Index anzmax, bnzmax ;
    if (GB_Index_multiply (&anzmax, A->nvec_nonempty, A->vlen))
    {
        Adense = (anzmax == GB_NNZ (A)) ;
    }

    if (GB_Index_multiply (&bnzmax, B->nvec_nonempty, B->vlen))
    {
        Bdense = (bnzmax == GB_NNZ (B)) ;
    }

    // if (Adense) fprintf (stderr, "all vectors in A are dense\n") ;
    // if (Bdense) fprintf (stderr, "all vectors in B are dense\n") ;

    int64_t cnz_guess = 15 + GB_NNZ (A) + GB_NNZ (B) ;
    if (Adense || Bdense)
    {
        // this is exact
        cnz_guess = A->nvec_nonempty * B->nvec_nonempty ;
    }

    GrB_Info info ;
    GrB_Type ctype = add->op->ztype ;
    int64_t cvlen = A->vdim ;
    int64_t cvdim = B->vdim ;

    info = GB_AxB_alloc (Chandle, ctype, cvlen, cvdim, (Mask_comp ? NULL : M),
        A, B, true, cnz_guess) ;

    if (info != GrB_SUCCESS)
    { 
        // out of memory
        return (info) ;
    }

    GrB_Matrix C = (*Chandle) ;

    // fprintf (stderr, "C->nzmax " GBd "\n", C->nzmax) ;

    //--------------------------------------------------------------------------
    // C = A'*B, computing each entry with a dot product, via builtin semiring
    //--------------------------------------------------------------------------

    bool done = false ;

#ifndef GBCOMPACT

    //--------------------------------------------------------------------------
    // define the worker for the switch factory
    //--------------------------------------------------------------------------

    #define GB_AdotB(add,mult,xyname) GB_AdotB_ ## add ## mult ## xyname

    #define GB_AxB_WORKER(add,mult,xyname)                              \
    {                                                                   \
        info = GB_AdotB (add,mult,xyname) (Chandle, M, Mask_comp,       \
            A, A_is_pattern, B, B_is_pattern) ;                         \
        done = true ;                                                   \
    }                                                                   \
    break ;

    //--------------------------------------------------------------------------
    // launch the switch factory
    //--------------------------------------------------------------------------

    GB_Opcode mult_opcode, add_opcode ;
    GB_Type_code xycode, zcode ;

    if (GB_AxB_semiring_builtin (A, A_is_pattern, B, B_is_pattern, semiring,
        flipxy, &mult_opcode, &add_opcode, &xycode, &zcode))
    { 
        #include "GB_AxB_factory.c"
    }

    if (info != GrB_SUCCESS)
    { 
        // out of memory
        return (info) ;
    }

#endif

    //--------------------------------------------------------------------------
    // user semirings created at compile time
    //--------------------------------------------------------------------------

    if (semiring->object_kind == GB_USER_COMPILED)
    {

        // determine the required type of A and B for the user semiring
        GrB_Type atype_required, btype_required ;

        if (flipxy)
        { 
            // A is passed as y, and B as x, in z = mult(x,y)
            atype_required = mult->ytype ;
            btype_required = mult->xtype ;
        }
        else
        { 
            // A is passed as x, and B as y, in z = mult(x,y)
            atype_required = mult->xtype ;
            btype_required = mult->ytype ;
        }

        if (A->type == atype_required && B->type == btype_required)
        {
            info = GB_AxB_user (GxB_AxB_DOT, semiring, Chandle, M, A, B,
                flipxy, Mask_comp, NULL, NULL, NULL, 0, NULL) ;
            done = true ;
            if (info != GrB_SUCCESS)
            { 
                // out of memory or invalid semiring
                return (info) ;
            }
        }
    }

    //--------------------------------------------------------------------------
    // C = A'*B, computing each entry with a dot product, with typecasting
    //--------------------------------------------------------------------------

    if (!done)
    {

        //----------------------------------------------------------------------
        // get operators, functions, workspace, contents of A, B, C, and M
        //----------------------------------------------------------------------

        // fprintf (stderr, "generic\n") ;

        GxB_binary_function fmult = mult->function ;
        GxB_binary_function fadd  = add->op->function ;

        size_t csize = C->type->size ;
        size_t asize = A_is_pattern ? 0 : A->type->size ;
        size_t bsize = B_is_pattern ? 0 : B->type->size ;

        size_t xsize = mult->xtype->size ;
        size_t ysize = mult->ytype->size ;

        // scalar workspace: because of typecasting, the x/y types need not
        // be the same as the size of the A and B types.
        // flipxy false: aki = (xtype) A(k,i) and bkj = (ytype) B(k,j)
        // flipxy true:  aki = (ytype) A(k,i) and bkj = (xtype) B(k,j)
        size_t aki_size = flipxy ? ysize : xsize ;
        size_t bkj_size = flipxy ? xsize : ysize ;

        char aki [aki_size] ;
        char bkj [bkj_size] ;

        char t [csize] ;

        GB_void *restrict Cx = C->x ;
        GB_void *restrict cij = Cx ;        // advances through each entry of C

        GB_void *restrict identity = add->identity ;
        GB_void *restrict terminal = add->terminal ;

        GB_cast_function cast_A, cast_B ;
        if (flipxy)
        { 
            // A is typecasted to y, and B is typecasted to x
            cast_A = A_is_pattern ? NULL : 
                     GB_cast_factory (mult->ytype->code, A->type->code) ;
            cast_B = B_is_pattern ? NULL : 
                     GB_cast_factory (mult->xtype->code, B->type->code) ;
        }
        else
        { 
            // A is typecasted to x, and B is typecasted to y
            cast_A = A_is_pattern ? NULL :
                     GB_cast_factory (mult->xtype->code, A->type->code) ;
            cast_B = B_is_pattern ? NULL :
                     GB_cast_factory (mult->ytype->code, B->type->code) ;
        }

        //----------------------------------------------------------------------
        // C = A'*B via dot products, function pointers, and typecasting
        //----------------------------------------------------------------------

        // aki = A(k,i), located in Ax [pA]
        #define GB_GETA(aki,Ax,pA,asize)                                \
            if (!A_is_pattern) cast_A (aki, Ax +((pA)*asize), asize) ;

        // bkj = B(k,j), located in Bx [pB]
        #define GB_GETB(bkj,Bx,pB,bsize)                                \
            if (!B_is_pattern) cast_B (bkj, Bx +((pB)*bsize), bsize) ;

        // t = aki*bkj
        #define GB_DOT_MULT(aki,bkj)                                    \
            GB_MULTIPLY (t, aki, bkj) ;

        // cij += t
        #define GB_DOT_ADD                                              \
            fadd (cij, cij, t) ;

        // break if cij reaches the terminal value
        #define GB_DOT_TERMINAL(cij)                                    \
            if (terminal != NULL && memcmp (cij, terminal, csize) == 0) \
            {                                                           \
                break ;                                                 \
            }

        // cij = t
        #define GB_DOT_COPY    memcpy (cij, t, csize) ;

        // C->x has moved so the pointer to cij needs to be recomputed
        #define GB_DOT_REACQUIRE cij = Cx + cnz * csize ;

        // cij = identity
        #define GB_DOT_CLEAR   memcpy (cij, identity, csize) ;

        // save the value of C(i,j) by advancing cij pointer to next value
        #define GB_DOT_SAVE    cij += csize ;

        #define GB_XTYPE GB_void
        #define GB_YTYPE GB_void

        if (flipxy)
        { 
            #define GB_MULTIPLY(z,x,y) fmult (z,y,x)
            #include "GB_AxB_dot_meta.c"
            #undef GB_MULTIPLY
        }
        else
        { 
            #define GB_MULTIPLY(z,x,y) fmult (z,x,y)
            #include "GB_AxB_dot_meta.c"
            #undef GB_MULTIPLY
        }
    }

    //--------------------------------------------------------------------------
    // trim the size of C: this cannot fail
    //--------------------------------------------------------------------------

    info = GB_ix_realloc (C, GB_NNZ (C), true, NULL) ;
    ASSERT (info == GrB_SUCCESS) ;
    ASSERT_OK (GB_check (C, "dot: C = A'*B output", GB0)) ;
    // GxB_fprint (C, 2, stderr) ;
    ASSERT (*Chandle == C) ;
    (*mask_applied) = (M != NULL) ;
    return (GrB_SUCCESS) ;
}

