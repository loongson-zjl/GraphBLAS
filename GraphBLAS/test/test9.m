clear all

rng ('default') ;
levels = 4 ;
nfeatures = 6 ;
nneurons = 16 ;

for level = 1:levels
    W {level} = sprand (nneurons, nneurons, 0.5) ;
    bias {level} = (rand (1,nneurons)) ;
end

Y0 = sprandn (nfeatures, nneurons, 0.5) ;

Y1 = dnn_matlab (W, bias, Y0) ;
Y2 = dnn_gb     (W, bias, Y0) ;
Y3 = dnn_gb_terse (W, bias, Y0) ;
Y4 = dnn_gb_terse (W, bias, Y0, 'gb') ;
% Y5 = dnn_gb_overload (W, bias, Y0) ;

err = norm (Y1-Y2,1)
err = norm (Y1-Y3,1)
err = norm (Y1-Y4,1)

