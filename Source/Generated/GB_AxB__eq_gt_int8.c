
//------------------------------------------------------------------------------
// GB_AxB:  hard-coded C=A*B and C<M>=A*B
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// Unless this file is Generator/GB_AxB.c, do not edit it (auto-generated)

#include "GB.h"
#ifndef GBCOMPACT
#include "GB_AxB__semirings.h"

// The C=A*B semiring is defined by the following types and operators:

// A*B function (Gustavon):  GB_AgusB__eq_gt_int8
// A'*B function (dot):      GB_AdotB__eq_gt_int8
// A*B function (heap):      GB_AheapB__eq_gt_int8
// Z type:   bool (the type of C)
// X type:   int8_t (the type of x for z=mult(x,y))
// Y type:   int8_t (the type of y for z=mult(x,y))
// Identity: true (where cij = (cij == identity) does not change cij)
// Multiply: z = x > y
// Add:      cij = (cij == z)
// Terminal: ;

#define GB_XTYPE \
    int8_t

#define GB_YTYPE \
    int8_t

#define GB_DOT_TERMINAL(cij) \
    ;

// aik = Ax [pA]
#define GB_GETA(aik,Ax,pA,asize) \
    int8_t aik = Ax [pA] ;

// bkj = Bx [pB]
#define GB_GETB(bkj,Bx,pB,bsize) \
    int8_t bkj = Bx [pB] ;

//------------------------------------------------------------------------------
// C<M>=A*B and C=A*B: gather/scatter saxpy-based method (Gustavson)
//------------------------------------------------------------------------------

#define GB_IDENTITY \
    true

// Sauna_Work [i] = identity
#define GB_CLEARW(Sauna_Work,i,identity,zsize)  \
    Sauna_Work [i] = identity ;

// Cx [p] = Sauna_Work [i]
#define GB_GATHERC(Cx,p,Sauna_Work,i,zsize)     \
    Cx [p] = Sauna_Work [i] ;

// mult-add operation (no mask)
#define GB_MULTADD_NOMASK                       \
    /* Sauna_Work [i] += A(i,k) * B(k,j) */     \
    bool t ;                                \
    t = aik > bkj ;                  \
    Sauna_Work [i] = (Sauna_Work [i] == t) ;

// mult-add operation (with mask)
#define GB_MULTADD_WITH_MASK                    \
    if (mark == hiwater)                        \
    {                                           \
        /* first time C(i,j) seen */            \
        /* Sauna_Work [i] = A(i,k) * B(k,j) */  \
        Sauna_Work [i] = aik > bkj ; \
        Sauna_Mark [i] = hiwater + 1 ;          \
    }                                           \
    else                                        \
    {                                           \
        /* C(i,j) seen before, update it */     \
        /* Sauna_Work [i] += A(i,k) * B(k,j) */ \
        GB_MULTADD_NOMASK ;                     \
    }

GrB_Info GB_AgusB__eq_gt_int8
(
    GrB_Matrix C,
    const GrB_Matrix M,
    const GrB_Matrix A, bool A_is_pattern,
    const GrB_Matrix B, bool B_is_pattern,
    GB_Sauna Sauna
)
{ 
    bool *restrict Sauna_Work = Sauna->Sauna_Work ;
    bool *restrict Cx = C->x ;
    GrB_Info info = GrB_SUCCESS ;
    #include "GB_AxB_Gustavson_meta.c"
    return (info) ;
}

//------------------------------------------------------------------------------
// C<M>=A'*B, C<!M>=A'*B or C=A'*B: dot product
//------------------------------------------------------------------------------

// t = aki*bkj
#define GB_DOT_MULT(aki,bkj)   \
    bool t ;               \
    t = aki > bkj ;

// cij += t
#define GB_DOT_ADD             \
    cij = (cij == t) ;

// cij = t
#define GB_DOT_COPY            \
    cij = t ;

// cij is not a pointer but a scalar; nothing to do
#define GB_DOT_REACQUIRE ;

// clear cij
#define GB_DOT_CLEAR           \
    cij = true ;

// save the value of C(i,j)
#define GB_DOT_SAVE            \
    Cx [cnz] = cij ;

GrB_Info GB_AdotB__eq_gt_int8
(
    GrB_Matrix *Chandle,
    const GrB_Matrix M, const bool Mask_comp,
    const GrB_Matrix A, bool A_is_pattern,
    const GrB_Matrix B, bool B_is_pattern
)
{ 
    GrB_Matrix C = (*Chandle) ;
    bool *restrict Cx = C->x ;
    bool cij ;
    GrB_Info info = GrB_SUCCESS ;
    #include "GB_AxB_dot_meta.c"
    return (info) ;
}

//------------------------------------------------------------------------------
// C<M>=A*B and C=A*B: heap saxpy-based method
//------------------------------------------------------------------------------

#include "GB_heap.h"

// cij = A(i,k) * B(k,j)
#define GB_CIJ_MULT(cij, aik, bkj)      \
    cij = aik > bkj ;

// C(i,j) += A(i,k) * B(k,j)
#define GB_CIJ_MULTADD(cij, aik, bkj)   \
    bool t ;                        \
    t = aik > bkj ;          \
    cij = (cij == t) ;

// cij is not a pointer but a scalar; nothing to do
#define GB_CIJ_REACQUIRE ;

// cij = identity
#define GB_CIJ_CLEAR                    \
    cij = true ;

// save the value of C(i,j)
#define GB_CIJ_SAVE                     \
    Cx [cnz] = cij ;

GrB_Info GB_AheapB__eq_gt_int8
(
    GrB_Matrix *Chandle,
    const GrB_Matrix M,
    const GrB_Matrix A, bool A_is_pattern,
    const GrB_Matrix B, bool B_is_pattern,
    int64_t *restrict List,
    GB_pointer_pair *restrict pA_pair,
    GB_Element *restrict Heap,
    const int64_t bjnz_max
)
{ 
    GrB_Matrix C = (*Chandle) ;
    bool *restrict Cx = C->x ;
    bool cij ;
    int64_t cvlen = C->vlen ;
    GB_CIJ_CLEAR ;
    GrB_Info info = GrB_SUCCESS ;
    #include "GB_AxB_heap_meta.c"
    return (info) ;
}

#endif

