function [I_RGB] = ConvertFromBayer(I)

R = I(1:2:size(I,1),1:2:size(I,2));
%figure('Name','Red Channel'); imshow(Ir);
B = I(2:2:size(I,1),2:2:size(I,2));
%figure('Name','Blue Channel'); imshow(Ib);
G = uint16(0.5*(double(I(1:2:size(I,1),2:2:size(I,2))) + double(I(2:2:size(I,1),1:2:size(I,2)))));
%figure('Name','Green Channel'); imshow(Ig);

I_RGB = zeros(size(R,1),size(R,2),3,'uint16');

I_RGB(:,:,1) = R;
I_RGB(:,:,2) = G;
I_RGB(:,:,3) = B;

end
