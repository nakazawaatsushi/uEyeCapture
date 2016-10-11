%%
id = [11 14 17];     % 1007_165944


[fname,fpath] = uigetfile('*.*');

fname1 = sprintf('%s%04d.png',fpath,id(1));
fname2 = sprintf('%s%04d.png',fpath,id(2));
fname3 = sprintf('%s%04d.png',fpath,id(3));

%
TMP = imread(fname1);
if size(TMP,3) == 3
    I{1} = ConvertFromBayer(TMP);
    TMP = imread(fname2);
    I{2} = ConvertFromBayer(TMP);
    TMP = imread(fname3);
    I{3} = ConvertFromBayer(TMP);
else
    I{1} = TMP;
    I{2} = imread(fname2);
    I{3} = imread(fname3);
end

%% difference between the first and second frames
Id = rgb2gray(I{2}) - rgb2gray(I{1});

figure, imshow(Id*10000);

%%
Id = rgb2gray(I{3}) - rgb2gray(I{2});

imshow(Id*10000);

% show normal images
%imshow(I{1}*16)
%figure,imshow(I{2}*16)
%figure,imshow(I{3}*16)

% visualize by color image
%figure,imshow(Id*200);

%%
%figure;
%for i=1:100
%    fprintf(1,'i = %d\n',i);
%    I2 = uint16(double(I{1}) - i*0.01*double(Id));
%    imshow(I2*16);
%    pause;
%end