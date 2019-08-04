//------------------------------------------------------------------------------
// gb_matlab.h: definitions for MATLAB interface for SuiteSparse:GraphBLAS
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// This MATLAB interface depends heavily on internal details of the
// SuiteSparse:GraphBLAS library.  Thus, GB.h is #include'd, not just
// GraphBLAS.h.

#ifndef GB_MATLAB_H
#define GB_MATLAB_H

#include "GB.h"
#include "mex.h"
#include <ctype.h>

//------------------------------------------------------------------------------
// error handling
//------------------------------------------------------------------------------

#define ERROR2(message, arg) \
    mexErrMsgIdAndTxt ("GraphBLAS:error", message, arg) ;
#define ERROR(message) mexErrMsgIdAndTxt ("GraphBLAS:error", message) ;
#define USAGE(message) mexErrMsgIdAndTxt ("GraphBLAS:usage", message) ;
#define CHECK_ERROR(error,message) if (error) ERROR (message) ;
#define OK(method) CHECK_ERROR ((method) != GrB_SUCCESS, GrB_error ( )) ;

//------------------------------------------------------------------------------
// basic macros
//------------------------------------------------------------------------------

// MATCH(s,t) compares two strings and returns true if equal
#define MATCH(s,t) (strcmp(s,t) == 0)

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

//------------------------------------------------------------------------------
// typedefs
//------------------------------------------------------------------------------

typedef enum            // output of gb.methods
{
    KIND_GB = 0,        // return a MATLAB struct containing a GrB_Matrix
    KIND_SPARSE = 1,    // return a MATLAB sparse matrix
    KIND_FULL = 2       // return a MATLAB dense matrix
}
kind_enum_t ;

//------------------------------------------------------------------------------
// gb_double_to_integer: convert a double to int64_t and check conversion
//------------------------------------------------------------------------------

static inline int64_t gb_double_to_integer (double x)
{
    int64_t i = (int64_t) x ;
    CHECK_ERROR (x != (double) i, "index must be integer") ;
    return (i) ;
}

//------------------------------------------------------------------------------
// function prototypes
//------------------------------------------------------------------------------

GrB_Type gb_mxarray_type        // return the GrB_Type of a MATLAB matrix
(
    const mxArray *X
) ;

GrB_Type gb_mxstring_to_type    // return the GrB_Type from a MATLAB string
(
    const mxArray *S        // MATLAB mxArray containing a string
) ;

void gb_mxstring_to_string  // copy a MATLAB string into a C string
(
    char *string,           // size at least maxlen+1
    const size_t maxlen,    // length of string
    const mxArray *S,       // MATLAB mxArray containing a string
    const char *name        // name of the mxArray
) ;

GrB_Matrix gb_get_shallow   // return a shallow copy of MATLAB sparse matrix
(
    const mxArray *X
) ;

GrB_Matrix gb_get_deep      // return a deep GrB_Matrix copy of a MATLAB X
(
    const mxArray *X,       // input MATLAB matrix (sparse or struct)
    GrB_Type type           // typecast X to this type (NULL if no typecast)
) ;

GrB_Type gb_type_to_mxstring    // return the MATLAB string from a GrB_Type
(
    const GrB_Type type
) ;

GrB_Matrix gb_typecast      // A = (type) S, where A is deep
(
    GrB_Type type,              // if NULL, copy but do not typecast
    GxB_Format_Value format,    // also convert to the requested format
    GrB_Matrix S                // may be shallow
) ;

void gb_usage       // check usage and make sure GxB_init has been called
(
    bool ok,                // if false, then usage is not correct
    const char *message     // error message if usage is not correct
) ;

void gb_find_dot            // find 1st and 2nd dot ('.') in a string
(
    int32_t position [2],   // positions of one or two dots
    const char *s           // null-terminated string to search
) ;

GrB_Type gb_string_to_type      // return the GrB_Type from a string
(
    const char *classname
) ;

GrB_BinaryOp gb_mxstring_to_binop       // return binary operator from a string
(
    const mxArray *mxstring,            // MATLAB string
    const GrB_Type default_type         // default type if not in the string
) ;

GrB_BinaryOp gb_string_to_binop         // return binary operator from a string
(
    char *opstring,                     // string defining the operator
    const GrB_Type default_type         // default type if not in the string
) ;

GrB_BinaryOp gb_string_and_type_to_binop    // return op from string and type
(
    const char *op_name,        // name of the operator, as a string
    const GrB_Type type         // type of the x,y inputs to the operator
) ;

GrB_Semiring gb_mxstring_to_semiring    // return semiring from a string
(
    const mxArray *mxstring,            // MATLAB string
    const GrB_Type default_type         // default type if not in the string
) ;

GrB_Semiring gb_string_to_semiring      // return a semiring from a string
(
    char *semiring_string,              // string defining the semiring
    const GrB_Type default_type         // default type if not in the string:
                                        // type of x,y inputs to mult operator
) ;

GrB_Semiring gb_semiring            // built-in semiring, or NULL if error
(
    const GrB_BinaryOp add,         // add operator
    const GrB_BinaryOp mult         // multiply operator
) ;

GrB_Descriptor gb_mxarray_to_descriptor     // return a new descriptor
(
    const mxArray *D_matlab,    // MATLAB struct
    kind_enum_t *kind           // gb, sparse, or full
) ;

mxArray *gb_export_to_mxstruct  // return exported MATLAB struct G
(
    GrB_Matrix *A_handle        // matrix to export; freed on output
) ;

mxArray *gb_export_to_mxsparse  // return exported MATLAB sparse matrix S
(
    GrB_Matrix *A_handle        // matrix to export; freed on output
) ;

mxArray *gb_export_to_mxfull    // return exported MATLAB dense matrix F
(
    void **X_handle,            // pointer to array to export
    const GrB_Index nrows,      // dimensions of F
    const GrB_Index ncols,
    GrB_Type type               // type of the array
) ;

mxArray *gb_export              // return the exported MATLAB matrix or struct
(
    GrB_Matrix *C_handle,       // GrB_Matrix to export and free
    kind_enum_t kind            // gb, sparse, or full
) ;

GrB_BinaryOp gb_string_to_selectop      // return select operator from a string
(
    char *opstring                      // string defining the operator
) ;

GrB_BinaryOp gb_mxstring_to_selectop    // return select operator from a string
(
    const mxArray *mxstring             // MATLAB string
) ;

bool gb_is_shallow              // true if any component of A is shallow
(
    GrB_Matrix A                // GrB_Matrix to query
) ;

bool gb_mxarray_is_scalar   // true if MATLAB array is a scalar
(
    const mxArray *S
) ;

bool gb_mxarray_is_empty    // true if MATLAB array is NULL, or 2D and 0-by-0
(
    const mxArray *S
) ;

void gb_mxfree              // mxFree wrapper
(
    void **p_handle         // handle to pointer to be freed
) ;

void gb_at_exit (void)  ;   // called when GraphBLAS is cleared by MATLAB

int64_t *gb_mxarray_to_list     // return List of integers
(
    const mxArray *mxList,      // list to extract
    bool *allocated,            // true if output list was allocated
    int64_t *len                // length of list
) ;

GrB_Index *gb_mxcell_to_index   // return index list I
(
    const mxArray *I_cell,      // MATLAB cell array
    const GrB_Index n,          // dimension of matrix being indexed
    bool *I_allocated,          // true if output array I is allocated
    GrB_Index *ni               // length (I)
) ;

void gb_matrix_assign_scalar
(
    GrB_Matrix C,
    const GrB_Matrix M,
    const GrB_BinaryOp accum,
    const GrB_Matrix A,
    const GrB_Index *I,
    const GrB_Index ni,
    const GrB_Index *J,
    const GrB_Index nj,
    const GrB_Descriptor desc
) ;

GrB_BinaryOp gb_first_binop         // return GrB_FIRST_[type] operator
(
    const GrB_Type type
) ;

#endif
