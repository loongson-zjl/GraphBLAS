//------------------------------------------------------------------------------
// GraphBLAS/Demo/Program/gauss_demo: Gaussian integers
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2022, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GraphBLAS.h"

//------------------------------------------------------------------------------
// the Gaussian integer: real and imaginary parts
//------------------------------------------------------------------------------

typedef struct
{
    int32_t real ;
    int32_t imag ;
}
gauss ;

// repeat the typedef as a string, to give to GraphBLAS
#define GAUSS_DEFN              \
"typedef struct "               \
"{ "                            \
   "int32_t real ; "            \
   "int32_t imag ; "            \
"} "                            \
"gauss ;"

//------------------------------------------------------------------------------
// addgauss: add two Gaussian integers
//------------------------------------------------------------------------------

void addgauss (gauss *z, const gauss *x, const gauss *y)
{
    z->real = x->real + y->real ;
    z->imag = x->imag + y->imag ;
}

#define ADDGAUSS_DEFN                                           \
"void addgauss (gauss *z, const gauss *x, const gauss *y)   \n" \
"{                                                          \n" \
"    z->real = x->real + y->real ;                          \n" \
"    z->imag = x->imag + y->imag ;                          \n" \
"}"

//------------------------------------------------------------------------------
// multgauss: multiply two Gaussian integers
//------------------------------------------------------------------------------

void multgauss (gauss *z, const gauss *x, const gauss *y)
{
    z->real = x->real * y->real - x->imag * y->imag ;
    z->imag = x->real * y->imag + x->imag * y->real ;
}

#define MULTGAUSS_DEFN                                          \
"void multgauss (gauss *z, const gauss *x, const gauss *y)  \n" \
"{                                                          \n" \
"    z->real = x->real * y->real - x->imag * y->imag ;      \n" \
"    z->imag = x->real * y->imag + x->imag * y->real ;      \n" \
"}"

//------------------------------------------------------------------------------
// realgauss: real part of a Gaussian integer
//------------------------------------------------------------------------------

void realgauss (int32_t *z, const gauss *x)
{
    (*z) = x->real ;
}

#define REALGAUSS_DEFN                                          \
"void realgauss (int32_t *z, const gauss *x)                \n" \
"{                                                          \n" \
"    (*z) = x->real ;                                       \n" \
"}"

//------------------------------------------------------------------------------
// ijgauss: Gaussian positional op
//------------------------------------------------------------------------------

void ijgauss (int64_t *z, const gauss *x, GrB_Index i, GrB_Index j, 
    const gauss *y)
{
    (*z) = x->real + y->real + i - j ;
//  printf ("i: %ld j: %ld x: (%d,%d), y: (%d,%d) result: %ld\n",
//      i, j, x->real, x->imag, y->real, y->imag, *z) ;
}

#define IJGAUSS_DEFN                                                        \
"void ijgauss (int64_t *z, const gauss *x, GrB_Index i, GrB_Index j,    \n" \
"    const gauss *y)                                                    \n" \
"{                                                                      \n" \
"    (*z) = x->real + y->real + i - j ;                                 \n" \
"}"

//------------------------------------------------------------------------------
// printgauss: print a Gauss matrix
//------------------------------------------------------------------------------

// This is a very slow way to print a large matrix, so using this approach is
// not recommended for large matrices.  However, it looks nice for this demo
// since the matrix is small.

#define TRY(method)             \
{                               \
    GrB_Info info = method ;    \
    if (info != GrB_SUCCESS)    \
    {                           \
        printf ("info: %d error! Line %d\n", info, __LINE__)  ; \
        fflush (stdout) ;       \
        abort ( ) ;             \
    }                           \
}

void printgauss (GrB_Matrix A, char *name)
{
    // print the matrix
    GrB_Index m, n ;
    TRY (GrB_Matrix_nrows (&m, A)) ;
    TRY (GrB_Matrix_ncols (&n, A)) ;
    printf ("\n%s\nsize: %d-by-%d\n", name, (int) m, (int) n) ;
    for (int i = 0 ; i < m ; i++)
    {
        printf ("row %2d: ", i) ;
        for (int j = 0 ; j < n ; j++)
        {
            gauss a ;
            GrB_Info info = GrB_Matrix_extractElement_UDT (&a, A, i, j) ;
            if (info == GrB_NO_VALUE)
            {
                printf ("      .     ") ;
            }
            else if (info == GrB_SUCCESS)
            {
                printf (" (%4d,%4d)", a.real, a.imag) ;
            }
            else TRY (GrB_PANIC) ;
        }
        printf ("\n") ;
    }
    printf ("\n") ;
}

//------------------------------------------------------------------------------
// gauss main program
//------------------------------------------------------------------------------

int main (void)
{
    // start GraphBLAS
    TRY (GrB_init (GrB_NONBLOCKING)) ;
    TRY (GxB_set (GxB_BURBLE, true)) ;
    printf ("Gauss demo.  Note that all transposes are array transposes,\n"
        "not matrix (conjugate) transposes.") ;

    // create the Gauss type
    GrB_Type Gauss ;
    TRY (GxB_Type_new (&Gauss, sizeof (gauss), "gauss", GAUSS_DEFN)) ;
    TRY (GxB_print (Gauss, 3)) ;

    // create the AddGauss operators
    GrB_BinaryOp AddGauss ; 
    TRY (GxB_BinaryOp_new (&AddGauss, (void *) addgauss, Gauss, Gauss, Gauss,
        "addgauss", ADDGAUSS_DEFN)) ;
    TRY (GxB_print (AddGauss, 3)) ;

    // create the AddMonoid
    gauss zero ;
    zero.real = 0 ;
    zero.imag = 0 ;
    GrB_Monoid AddMonoid ;
    TRY (GrB_Monoid_new_UDT (&AddMonoid, AddGauss, &zero)) ;
    TRY (GxB_print (AddMonoid, 3)) ;

    // create the MultGauss operator
    GrB_BinaryOp MultGauss ;
    TRY (GxB_BinaryOp_new (&MultGauss, (void *) multgauss,
        Gauss, Gauss, Gauss, "multgauss", MULTGAUSS_DEFN)) ;
    TRY (GxB_print (MultGauss, 3)) ;

    // create the GaussSemiring
    GrB_Semiring GaussSemiring ;
    TRY (GrB_Semiring_new (&GaussSemiring, AddMonoid, MultGauss)) ;
    TRY (GxB_print (GaussSemiring, 3)) ;

    // create a 4-by-4 Gauss matrix, each entry A(i,j) = (i+1,2-j),
    // except A(0,0) is missing
    GrB_Matrix A, D ;
    TRY (GrB_Matrix_new (&A, Gauss, 4, 4)) ;
    TRY (GrB_Matrix_new (&D, GrB_BOOL, 4, 4)) ;
    gauss a ;
    for (int i = 0 ; i < 4 ; i++)
    {
        TRY (GrB_Matrix_setElement (D, 1, i, i)) ;
        for (int j = 0 ; j < 4 ; j++)
        {
            if (i == 0 && j == 0) continue ;
            a.real = i+1 ;
            a.imag = 2-j ;
            TRY (GrB_Matrix_setElement_UDT (A, &a, i, j)) ;
        }
    }
    printgauss (A, "\n=============== Gauss A matrix:\n") ;

    // a = sum (A)
    TRY (GrB_Matrix_reduce_UDT (&a, NULL, AddMonoid, A, NULL)) ;
    printf ("\nsum (A) = (%d,%d)\n", a.real, a.imag) ;

    // A = A*A
    TRY (GrB_mxm (A, NULL, NULL, GaussSemiring, A, A, NULL)) ;
    printgauss (A, "\n=============== Gauss A = A^2 matrix:\n") ;

    // a = sum (A)
    TRY (GrB_Matrix_reduce_UDT (&a, NULL, AddMonoid, A, NULL)) ;
    printf ("\nsum (A^2) = (%d,%d)\n", a.real, a.imag) ;

    // C<D> = A*A' where A and D are sparse
    GrB_Matrix C ;
    TRY (GrB_Matrix_new (&C, Gauss, 4, 4)) ;
    printgauss (C, "\nGauss C empty matrix") ;
    TRY (GxB_set (A, GxB_SPARSITY_CONTROL, GxB_SPARSE)) ;
    TRY (GxB_set (D, GxB_SPARSITY_CONTROL, GxB_SPARSE)) ;
    TRY (GrB_mxm (C, D, NULL, GaussSemiring, A, A, GrB_DESC_T1)) ;
    printgauss (C, "\n=============== Gauss C = diag(AA') matrix:\n") ;

    // C = D*A
    GrB_free (&D) ;
    TRY (GrB_Matrix_new (&D, Gauss, 4, 4)) ;
    TRY (GxB_set (A, GxB_SPARSITY_CONTROL, GxB_SPARSE)) ;
    TRY (GxB_set (D, GxB_SPARSITY_CONTROL, GxB_SPARSE)) ;
    TRY (GrB_select (D, NULL, NULL, GrB_DIAG, A, 0, NULL)) ;
    printgauss (D, "\nGauss D matrix") ;
    TRY (GrB_mxm (C, NULL, NULL, GaussSemiring, D, A, NULL)) ;
    printgauss (C, "\n=============== Gauss C = D*A matrix:\n") ;

    // C = A*D
    TRY (GrB_mxm (C, NULL, NULL, GaussSemiring, A, D, NULL)) ;
    printgauss (C, "\n=============== Gauss C = A*D matrix:\n") ;

    // C = (1,2) then C += A*A' where C is full
    gauss ciso ;
    ciso.real = 1 ;
    ciso.imag = -2 ;
    TRY (GrB_Matrix_assign_UDT (C, NULL, NULL, &ciso,
        GrB_ALL, 4, GrB_ALL, 4, NULL)) ;
    printgauss (C, "\n=============== Gauss C = (1,-2) matrix:\n") ;
    printgauss (A, "\n=============== Gauss A matrix:\n") ;
    TRY (GrB_mxm (C, NULL, AddGauss, GaussSemiring, A, A, GrB_DESC_T1)) ;
    printgauss (C, "\n=============== Gauss C += A*A' matrix:\n") ;

    // C += B*A where B is full and A is sparse
    GrB_Matrix B ;
    TRY (GrB_Matrix_new (&B, Gauss, 4, 4)) ;
    TRY (GrB_Matrix_assign_UDT (B, NULL, NULL, &ciso,
        GrB_ALL, 4, GrB_ALL, 4, NULL)) ;
    printgauss (B, "\n=============== Gauss B = (1,-2) matrix:\n") ;
    TRY (GrB_mxm (C, NULL, AddGauss, GaussSemiring, B, A, NULL)) ;
    printgauss (C, "\n=============== Gauss C += B*A:\n") ;

    // C += A*B where B is full and A is sparse
    TRY (GrB_mxm (C, NULL, AddGauss, GaussSemiring, A, B, NULL)) ;
    printgauss (C, "\n=============== Gauss C += A*B:\n") ;

    // C = ciso+A
    TRY (GrB_apply (C, NULL, NULL, AddGauss, (void *) &ciso, A, NULL)) ;
    printgauss (C, "\n=============== Gauss C = (1,-2) + A:\n") ;

    // C = A*ciso
    TRY (GrB_apply (C, NULL, NULL, MultGauss, A, (void *) &ciso, NULL)) ;
    printgauss (C, "\n=============== Gauss C = A*(1,-2):\n") ;

    // C = A'*ciso
    TRY (GrB_apply (C, NULL, NULL, MultGauss, A, (void *) &ciso, GrB_DESC_T0)) ;
    printgauss (C, "\n=============== Gauss C = A'*(1,-2):\n") ;

    // C = ciso*A'
    TRY (GrB_apply (C, NULL, NULL, MultGauss, (void *) &ciso, A, GrB_DESC_T1)) ;
    printgauss (C, "\n=============== Gauss C = (1,-2)*A':\n") ;

    // create the RealGauss unary op
    GrB_UnaryOp RealGauss ;
    TRY (GxB_UnaryOp_new (&RealGauss, (void *) realgauss, GrB_INT32, Gauss,
        "realgauss", REALGAUSS_DEFN)) ;
    TRY (GxB_print (RealGauss, 3)) ;
    GrB_Matrix R ;
    TRY (GrB_Matrix_new (&R, GrB_INT32, 4, 4)) ;
    // R = RealGauss (C)
    TRY (GrB_apply (R, NULL, NULL, RealGauss, C, NULL)) ;
    GxB_print (R, 3) ;
    // R = RealGauss (C')
    printgauss (C, "\n=============== R = RealGauss (C')\n") ;
    TRY (GrB_apply (R, NULL, NULL, RealGauss, C, GrB_DESC_T0)) ;
    GxB_print (R, 2) ;
    GrB_free (&R) ;

    // create the IJGauss IndexUnaryOp
    GrB_IndexUnaryOp IJGauss ;
    TRY (GxB_IndexUnaryOp_new (&IJGauss, (void *) ijgauss, GrB_INT64, Gauss,
        Gauss, "ijgauss", IJGAUSS_DEFN)) ;
    TRY (GrB_Matrix_new (&R, GrB_INT64, 4, 4)) ;
    printgauss (C, "\n=============== C \n") ;
    TRY (GrB_Matrix_apply_IndexOp_UDT (R, NULL, NULL, IJGauss, C,
        (void *) &ciso, NULL)) ;
    printf ("\nR = ijgauss (C)\n") ;
    GxB_print (R, 3) ;
    GrB_Index I [100], J [100], rnvals = 100 ;
    double X [100] ;
    TRY (GrB_Matrix_extractTuples_FP64 (I, J, X, &rnvals, R)) ;
    for (int k = 0 ; k < rnvals ; k++)
    { 
        printf ("R (%ld,%ld) = %g\n", I [k], J [k], X [k]) ;
    }

    printgauss (C, "\n=============== C\n") ;
    TRY (GrB_transpose (C, NULL, NULL, C, NULL)) ;
    printgauss (C, "\n=============== C = C'\n") ;

    // free everything and finalize GraphBLAS
    GrB_free (&A) ;
    GrB_free (&B) ;
    GrB_free (&D) ;
    GrB_free (&C) ;
    GrB_free (&R) ;
    GrB_free (&Gauss) ;
    GrB_free (&AddGauss) ;
    GrB_free (&RealGauss) ;
    GrB_free (&IJGauss) ;
    GrB_free (&AddMonoid) ;
    GrB_free (&MultGauss) ;
    GrB_free (&GaussSemiring) ;
    GrB_finalize ( ) ;
}

