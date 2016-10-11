%%
%id = [9 10 14];     % face (oizumi) 0926_182221
id = [8 9 13];     % Face (nakazawa) 0926_182221
%id = [21 22 27];    % PAPER
%id = [7 8 12];      % PARM
%id = [3 4 9];       % HAIR


[fname,fpath] = uigetfile('*.*');

fname1 = sprintf('%s%04d.png',fpath,id(1));
fname2 = sprintf('%s%04d.png',fpath,id(2));
fname3 = sprintf('%s%04d.png',fpath,id(3));

%
TMP = imread(fname1);
I{1} = ConvertFromBayer(TMP);
TMP = imread(fname2);
I{2} = ConvertFromBayer(TMP);
TMP = imread(fname3);
I{3} = ConvertFromBayer(TMP);

%% difference between the first and second frames
Id = I{2} - I{1};

% show normal images
imshow(I{1}*16)
figure,imshow(I{2}*16)
figure,imshow(I{3}*16)

% visualize by color image
figure,imshow(Id*200);

%%
figure;
for i=1:100
    fprintf(1,'i = %d\n',i);
    I2 = uint16(double(I{1}) - i*0.01*double(Id));
    imshow(I2*16);
    pause;
end