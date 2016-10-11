[fname,fpath] = uigetfile('*.*');
I = imread(strcat(fpath,fname));
[R{1},G{1},B{1}] = ConvertFromBayer(I);
[fname,fpath] = uigetfile('*.*');
I = imread(strcat(fpath,fname));
[R{2},G{2},B{2}] = ConvertFromBayer(I);
[fname,fpath] = uigetfile('*.*');
I = imread(strcat(fpath,fname));
[R{3},G{3},B{3}] = ConvertFromBayer(I);

[~,rect] = imcrop(R{1});

for i=1:3
    R{i} = imcrop(R{i},rect);
    G{i} = imcrop(G{i},rect);
    B{i} = imcrop(B{i},rect);
end    

%%
Irgb = zeros(size(R{1},1),size(R{1},2),3,'uint16');
Irgb(:,:,1) = R{1};
Irgb(:,:,2) = G{1};
Irgb(:,:,3) = B{1};
figure('Name','RGB1'), imshow(Irgb);

Irgb(:,:,1) = R{2};
Irgb(:,:,2) = G{2};
Irgb(:,:,3) = B{2};
figure('Name','RGB2'), imshow(Irgb);

Irgb(:,:,1) = R{3};
Irgb(:,:,2) = G{3};
Irgb(:,:,3) = B{3};
figure('Name','RGB3'), imshow(Irgb);

%%
R12 = R{2} - R{1};
G12 = G{2} - G{1};
B12 = B{2} - B{1};
RGB12 = zeros(size(R12,1),size(R12,2),3,'uint16');
RGB12(:,:,1) = R12; RGB13(:,:,2) = G12; RGB13(:,:,3) = B12; 
figure, 
subplot(1,4,1); imshow(R12);
subplot(1,4,2); imshow(G12);
subplot(1,4,3); imshow(B12);
subplot(1,4,4); imshow(RGB12);

R13 = R{3} - R{1};
G13 = G{3} - G{1};
B13 = B{3} - B{1};
RGB13 = zeros(size(R12,1),size(R12,2),3,'uint16');
RGB13(:,:,1) = R13; RGB13(:,:,2) = G13; RGB13(:,:,3) = B13; 
figure, 
subplot(1,4,1); imshow(R13);
subplot(1,4,2); imshow(G13);
subplot(1,4,3); imshow(B13);
subplot(1,4,4); imshow(RGB13);

%% plot
idx = find(R{2} < 65535);   % Eliminate overflowed pixels
idx = find(G{2}(idx) < 65535);
idx = find(B{2}(idx) < 65535);
figure, plot3(R12(1:1:size(idx,1)),G12(1:1:size(idx,1)),B12(1:1:size(idx,1)),'r.');

idx = find(R{3} < 65535);   % Eliminate overflowed pixels
idx = find(G{3}(idx) < 65535);
idx = find(B{3}(idx) < 65535);
hold on, plot3(R13(1:1:size(idx,1)),G13(1:1:size(idx,1)),B13(1:1:size(idx,1)),'g.');

