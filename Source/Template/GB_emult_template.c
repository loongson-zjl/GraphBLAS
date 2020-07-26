//------------------------------------------------------------------------------
// GB_emult_template:  phase1 and phase2 for C=A.*B, C<M>=A.*B
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// Computes C=A.*B (no mask) or C<M>=A.*B (mask present and not complemented).
// Does not handle the case C<!M>=A.*B.  The complemented mask is handled in
// GB_mask instead.  If present, the mask M is assumed to be very sparse
// compared with A and B.

// phase1: does not compute C itself, but just counts the # of entries in each
// vector of C.  Fine tasks compute the # of entries in their slice of a
// single vector of C, and the results are cumsum'd.

// phase2: computes C, using the counts computed by phase1.

{

    // iB_first is unused if the operator is FIRST or PAIR
    #include "GB_unused.h"

    //--------------------------------------------------------------------------
    // get A, B, M, and C
    //--------------------------------------------------------------------------

    const int64_t *GB_RESTRICT Ap = A->p ;
    const int64_t *GB_RESTRICT Ah = A->h ;
    const int64_t *GB_RESTRICT Ai = A->i ;
    const int64_t vlen = A->vlen ;

    const int64_t *GB_RESTRICT Bp = B->p ;
    const int64_t *GB_RESTRICT Bh = B->h ;
    const int64_t *GB_RESTRICT Bi = B->i ;

    const int64_t *GB_RESTRICT Mp = NULL ;
    const int64_t *GB_RESTRICT Mh = NULL ;
    const int64_t *GB_RESTRICT Mi = NULL ;
    const GB_void *GB_RESTRICT Mx = NULL ;
    size_t msize = 0 ;
    if (M != NULL)
    { 
        Mp = M->p ;
        Mh = M->h ;
        Mi = M->i ;
        Mx = (GB_void *) (Mask_struct ? NULL : (M->x)) ;
        msize = M->type->size ;
    }

    #if defined ( GB_PHASE_2_OF_2 )
    const GB_ATYPE *GB_RESTRICT Ax = (GB_ATYPE *) A->x ;
    const GB_BTYPE *GB_RESTRICT Bx = (GB_BTYPE *) B->x ;
    const int64_t  *GB_RESTRICT Cp = C->p ;
    const int64_t  *GB_RESTRICT Ch = C->h ;
          int64_t  *GB_RESTRICT Ci = C->i ;
          GB_CTYPE *GB_RESTRICT Cx = (GB_CTYPE *) C->x ;
    #endif

    //--------------------------------------------------------------------------
    // phase1: count entries in each C(:,j); phase2: compute C
    //--------------------------------------------------------------------------

    int taskid ;
    #pragma omp parallel for num_threads(nthreads) schedule(dynamic,1)
    for (taskid = 0 ; taskid < ntasks ; taskid++)
    {

        //----------------------------------------------------------------------
        // get the task descriptor
        //----------------------------------------------------------------------

        int64_t kfirst = TaskList [taskid].kfirst ;
        int64_t klast  = TaskList [taskid].klast ;
        bool fine_task = (klast == -1) ;
        int64_t len ;
        if (fine_task)
        { 
            // a fine task operates on a slice of a single vector
            klast = kfirst ;
            len = TaskList [taskid].len ;
        }
        else
        { 
            // a coarse task operates on one or more whole vectors
            len = vlen ;
        }

        for (int64_t k = kfirst ; k <= klast ; k++)
        {

            //------------------------------------------------------------------
            // get j, the kth vector of C
            //------------------------------------------------------------------

            int64_t j = GBH (Ch, k) ;

            #if defined ( GB_PHASE_1_OF_2 )
            int64_t cjnz = 0 ;
            #else
            int64_t pC, pC_end ;
            if (fine_task)
            { 
                // A fine task computes a slice of C(:,j)
                pC     = TaskList [taskid  ].pC ;
                pC_end = TaskList [taskid+1].pC ;
                ASSERT (Cp [k] <= pC && pC <= pC_end && pC_end <= Cp [k+1]) ;
            }
            else
            { 
                // The vectors of C are never sliced for a coarse task.
                pC     = Cp [k] ;       // ok: C is sparse
                pC_end = Cp [k+1] ;     // ok: C is sparse
            }
            int64_t cjnz = pC_end - pC ;
            if (cjnz == 0) continue ;
            #endif

            //------------------------------------------------------------------
            // get A(:,j)
            //------------------------------------------------------------------

            int64_t pA = -1, pA_end = -1 ;
            if (fine_task)
            { 
                // A fine task operates on Ai,Ax [pA...pA_end-1], which is
                // A fine task operates on Ai,Ax [pA...pA_end-1], which is
                // a subset of the vector A(:,j)
                pA     = TaskList [taskid].pA ;
                pA_end = TaskList [taskid].pA_end ;
            }
            else
            {
                // A coarse task operates on the entire vector A (:,j)
                int64_t kA = (Ch == Ah) ? k :
                            ((C_to_A == NULL) ? j : C_to_A [k]) ;
                if (kA >= 0)
                { 
                    pA     = GBP (Ap, kA, vlen) ;
                    pA_end = GBP (Ap, kA+1, vlen) ;
                }
            }

            int64_t ajnz = pA_end - pA ;        // nnz in A(:,j) for this slice
            bool adense = (ajnz == len) ;
            int64_t pA_start = pA ;

            // get the first and last indices in A(:,j) for this vector
            int64_t iA_first = -1 ;
            if (ajnz > 0)
            { 
                iA_first = GBI (Ai, pA, vlen) ;
            }
            #if defined ( GB_PHASE_1_OF_2 ) || defined ( GB_DEBUG )
            int64_t iA_last = -1 ;
            if (ajnz > 0)
            { 
                iA_last  = GBI (Ai, pA_end-1, vlen) ;
            }
            #endif

            //------------------------------------------------------------------
            // get B(:,j)
            //------------------------------------------------------------------

            int64_t pB = -1, pB_end = -1 ;
            if (fine_task)
            { 
                // A fine task operates on Bi,Bx [pB...pB_end-1], which is
                // a subset of the vector B(:,j)
                pB     = TaskList [taskid].pB ;
                pB_end = TaskList [taskid].pB_end ;
            }
            else
            {
                // A coarse task operates on the entire vector B (:,j)
                int64_t kB = (Ch == Bh) ? k :
                            ((C_to_B == NULL) ? j : C_to_B [k]) ;
                if (kB >= 0)
                { 
                    pB     = GBP (Bp, kB, vlen) ;
                    pB_end = GBP (Bp, kB+1, vlen) ;
                }
            }

            int64_t bjnz = pB_end - pB ;        // nnz in B(:,j) for this slice
            bool bdense = (bjnz == len) ;
            int64_t pB_start = pB ;

            // get the first and last indices in B(:,j) for this vector
            int64_t iB_first = -1 ;
            if (bjnz > 0)
            { 
                iB_first = GBI (Bi, pB, vlen) ;
            }
            #if defined ( GB_PHASE_1_OF_2 ) || defined ( GB_DEBUG )
            int64_t iB_last = -1 ;
            if (bjnz > 0)
            { 
                iB_last  = GBI (Bi, pB_end-1, vlen) ;
            }
            #endif

            //------------------------------------------------------------------
            // phase1: count nnz (C (:,j)); phase2: compute C(:,j)
            //------------------------------------------------------------------

            #if defined ( GB_PHASE_1_OF_2 )

            if (ajnz == 0 || bjnz == 0)
            { 

                //--------------------------------------------------------------
                // A(:,j) and/or B(:,j) are empty
                //--------------------------------------------------------------

                ;

            }
            else if (iA_last < iB_first || iB_last < iA_first)
            { 

                //--------------------------------------------------------------
                // intersection of A(:,j) and B(:,j) is empty
                //--------------------------------------------------------------

                // the last entry of A(:,j) comes before the first entry
                // of B(:,j), or visa versa
                ;

            }
            else

            #endif

            if (M == NULL)
            {

                if (adense && bdense)
                {

                    //----------------------------------------------------------
                    // A(:,j) and B(:,j) dense: thus C(:,j) dense
                    //----------------------------------------------------------

                    ASSERT (ajnz == bjnz) ;
                    ASSERT (iA_first == iB_first) ;
                    ASSERT (iA_last  == iB_last ) ;
                    #if defined ( GB_PHASE_1_OF_2 )
                    cjnz = ajnz ;
                    #else
                    ASSERT (cjnz == ajnz) ;
                    GB_PRAGMA_SIMD_VECTORIZE
                    for (int64_t p = 0 ; p < ajnz ; p++)
                    { 
                        int64_t i = p + iA_first ;
                        Ci [pC + p] = i ;                   // ok: C is sparse
                        GB_GETA (aij, Ax, pA + p) ;
                        GB_GETB (bij, Bx, pB + p) ;
                        GB_BINOP (GB_CX (pC + p), aij, bij, i, j) ;
                    }
                    #endif

                }
                else if (adense)
                {

                    //----------------------------------------------------------
                    // A(:,j) is dense, B(:,j) is sparse: thus C(:,j) sparse
                    //----------------------------------------------------------

                    #if defined ( GB_PHASE_1_OF_2 )
                    cjnz = bjnz ;
                    #else
                    ASSERT (cjnz == bjnz) ;
                    GB_PRAGMA_SIMD_VECTORIZE
                    for (int64_t p = 0 ; p < bjnz ; p++)
                    { 
                        int64_t i = Bi [pB + p] ;           // ok: B is sparse
                        Ci [pC + p] = i ;                   // ok: C is sparse
                        GB_GETA (aij, Ax, pA + i - iA_first) ;
                        GB_GETB (bij, Bx, pB + p) ;
                        GB_BINOP (GB_CX (pC + p), aij, bij, i, j) ;
                    }
                    #endif

                }
                else if (bdense)
                {

                    //----------------------------------------------------------
                    // A(:,j) is sparse, B(:,j) is dense: thus C(:,j) sparse
                    //----------------------------------------------------------

                    #if defined ( GB_PHASE_1_OF_2 )
                    cjnz = ajnz ;
                    #else
                    ASSERT (cjnz == ajnz) ;
                    GB_PRAGMA_SIMD_VECTORIZE
                    for (int64_t p = 0 ; p < ajnz ; p++)
                    { 
                        int64_t i = Ai [pA + p] ;           // ok: A is sparse
                        Ci [pC + p] = i ;                   // ok: C is sparse
                        GB_GETA (aij, Ax, pA + p) ;
                        GB_GETB (bij, Bx, pB + i - iB_first) ;
                        GB_BINOP (GB_CX (pC + p), aij, bij, i, j) ;
                    }
                    #endif

                }
                else if (ajnz > 32 * bjnz)
                {

                    //----------------------------------------------------------
                    // A(:,j) is much denser than B(:,j)
                    //----------------------------------------------------------

                    for ( ; pB < pB_end ; pB++)
                    {
                        int64_t i = Bi [pB] ;               // ok: B is sparse
                        // find i in A(:,j)                 // ok: A is sparse
                        int64_t pright = pA_end - 1 ;
                        bool found ;
                        GB_BINARY_SEARCH (i, Ai, pA, pright, found) ;
                        if (found)
                        { 
                            #if defined ( GB_PHASE_1_OF_2 )
                            cjnz++ ;
                            #else
                            ASSERT (pC < pC_end) ;
                            Ci [pC] = i ;                   // ok: C is sparse
                            GB_GETA (aij, Ax, pA) ;
                            GB_GETB (bij, Bx, pB) ;
                            GB_BINOP (GB_CX (pC), aij, bij, i, j) ;
                            pC++ ;
                            #endif
                        }
                    }
                    #if defined ( GB_PHASE_2_OF_2 )
                    ASSERT (pC == pC_end) ;
                    #endif

                }
                else if (bjnz > 32 * ajnz)
                {

                    //----------------------------------------------------------
                    // B(:,j) is much denser than A(:,j)
                    //----------------------------------------------------------

                    for ( ; pA < pA_end ; pA++)
                    {
                        int64_t i = Ai [pA] ;               // ok: A is sparse
                        // find i in B(:,j)                 // ok: B is sparse
                        int64_t pright = pB_end - 1 ;
                        bool found ;
                        GB_BINARY_SEARCH (i, Bi, pB, pright, found) ;
                        if (found)
                        { 
                            #if defined ( GB_PHASE_1_OF_2 )
                            cjnz++ ;
                            #else
                            ASSERT (pC < pC_end) ;
                            Ci [pC] = i ;                   // ok: C is sparse
                            GB_GETA (aij, Ax, pA) ;
                            GB_GETB (bij, Bx, pB) ;
                            GB_BINOP (GB_CX (pC), aij, bij, i, j) ;
                            pC++ ;
                            #endif
                        }
                    }
                    #if defined ( GB_PHASE_2_OF_2 )
                    ASSERT (pC == pC_end) ;
                    #endif

                }
                else
                {

                    //----------------------------------------------------------
                    // A(:,j) and B(:,j) have about the same # of entries
                    //----------------------------------------------------------

                    // linear-time scan of A(:,j) and B(:,j)

                    while (pA < pA_end && pB < pB_end)
                    {
                        int64_t iA = Ai [pA] ;              // ok: A is sparse
                        int64_t iB = Bi [pB] ;              // ok: B is sparse
                        if (iA < iB)
                        { 
                            // A(i,j) exists but not B(i,j)
                            pA++ ;
                        }
                        else if (iB < iA)
                        { 
                            // B(i,j) exists but not A(i,j)
                            pB++ ;
                        }
                        else
                        { 
                            // both A(i,j) and B(i,j) exist
                            #if defined ( GB_PHASE_1_OF_2 )
                            cjnz++ ;
                            #else
                            ASSERT (pC < pC_end) ;
                            Ci [pC] = iB ;                  // ok: C is sparse
                            GB_GETA (aij, Ax, pA) ;
                            GB_GETB (bij, Bx, pB) ;
                            GB_BINOP (GB_CX (pC), aij, bij, iB, j) ;
                            pC++ ;
                            #endif
                            pA++ ;
                            pB++ ;
                        }
                    }

                    #if defined ( GB_PHASE_2_OF_2 )
                    ASSERT (pC == pC_end) ;
                    #endif
                }

            }
            else
            {

                //--------------------------------------------------------------
                // Mask is present
                //--------------------------------------------------------------

                int64_t pM = -1 ;
                int64_t pM_end = -1 ;
                if (fine_task)
                { 
                    // A fine task operates on Mi,Mx [pM...pM_end-1], which is
                    // a subset of the vector M(:,j)
                    pM     = TaskList [taskid].pM ;
                    pM_end = TaskList [taskid].pM_end ;
                }
                else
                {
                    int64_t kM = -1 ;
                    if (Ch == Mh)
                    { 
                        // Ch is the same as Mh (a shallow copy), or both NULL
                        kM = k ;
                    }
                    else
                    { 
                        kM = (C_to_M == NULL) ? j : C_to_M [k] ;
                    }
                    if (kM >= 0)
                    { 
                        pM     = GBP (Mp, kM, vlen) ;
                        pM_end = GBP (Mp, kM+1, vlen) ;
                    }
                }

                //--------------------------------------------------------------
                // C(:,j)<M(:,j) = A(:,j) .* B (:,j)
                //--------------------------------------------------------------

                for ( ; pM < pM_end ; pM++)
                {

                    //----------------------------------------------------------
                    // get M(i,j) for A(i,j) .* B (i,j)
                    //----------------------------------------------------------

                    int64_t i = GBI (Mi, pM, vlen) ;
                    bool mij = GB_mcast (Mx, pM, msize) ;
                    if (!mij) continue ;

                    //----------------------------------------------------------
                    // get A(i,j)
                    //----------------------------------------------------------

                    if (adense)
                    { 
                        // A(:,j) is dense; use direct lookup for A(i,j)
                        pA = pA_start + i - iA_first ;
                    }
                    else
                    { 
                        // A(:,j) is sparse; use binary search for A(i,j)
                        int64_t apright = pA_end - 1 ;
                        bool afound ;
                        GB_BINARY_SEARCH (i, Ai, pA, apright, afound) ;
                        if (!afound) continue ;
                    }
                    ASSERT (GBI (Ai, pA, vlen) == i) ;

                    //----------------------------------------------------------
                    // get B(i,j)
                    //----------------------------------------------------------

                    if (bdense)
                    { 
                        // B(:,j) is dense; use direct lookup for B(i,j)
                        pB = pB_start + i - iB_first ;
                    }
                    else
                    { 
                        // B(:,j) is sparse; use binary search for B(i,j)
                        int64_t bpright = pB_end - 1 ;
                        bool bfound ;
                        GB_BINARY_SEARCH (i, Bi, pB, bpright, bfound) ;
                        if (!bfound) continue ;
                    }
                    ASSERT (GBI (Bi, pB, vlen) == i) ;

                    //----------------------------------------------------------
                    // C(i,j) = A(i,j) .* B(i,j)
                    //----------------------------------------------------------

                    // C (i,j) = A (i,j) .* B (i,j)
                    #if defined ( GB_PHASE_1_OF_2 )
                    cjnz++ ;
                    #else
                    Ci [pC] = i ;                   // ok: C is sparse
                    GB_GETA (aij, Ax, pA) ;
                    GB_GETB (bij, Bx, pB) ;
                    GB_BINOP (GB_CX (pC), aij, bij, i, j) ;
                    pC++ ;
                    #endif
                }

                #if defined ( GB_PHASE_2_OF_2 )
                ASSERT (pC == pC_end) ;
                #endif
            }

            //------------------------------------------------------------------
            // final count of nnz (C (:,j))
            //------------------------------------------------------------------

            #if defined ( GB_PHASE_1_OF_2 )
            if (fine_task)
            { 
                TaskList [taskid].pC = cjnz ;
            }
            else
            { 
                Cp [k] = cjnz ;     // ok: C is sparse
            }
            #endif
        }
    }
}

