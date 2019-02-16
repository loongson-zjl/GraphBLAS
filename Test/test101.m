% function test101
%TEST101 test import/export

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

rng ('default') ;

fprintf ('test101: GB_mex_export\n') ;

for n = 1:5
    for m = 1:5
        A = sprand (m, n, 0.1) ;
        for f = 0:3
            for hyper = 0:1
                for csc = 0:1
                    C = GB_mex_export (A, f, hyper, csc) ;
                    assert (isequal (A, C)) ;
                end
            end
        end
    end
end

fprintf ('\ntest101: all tests passed\n') ;