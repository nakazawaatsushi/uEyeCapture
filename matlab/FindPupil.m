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
figure, imshow(Id*1000);

%%
Thresh_BIN = 12000/65535;
G = fspecial('gaussian',10,0.2);
II = imfilter(1000*Id,G);
bw0 = im2bw(II, Thresh_BIN);
bw = abs(ones(size(bw0)) - bw0);

Thresh_AREA = 100;
% remove all object containing fewer than Thresh_AREA pixels
bw = bwareaopen(bw,Thresh_AREA);

% show images
figure();
imshow(10*bw);

%% fill a gap in the pen's cap
se = strel('disk',2);
bw = imclose(bw,se);

% fill any holes, so that regionprops can be used to estimate
% the area enclosed by each of the boundaries
bw = imfill(bw,'holes');

[B,L] = bwboundaries(bw,'noholes');   
stats = regionprops(L,'Area','Centroid');

% loop over the boundaries
for k = 1:length(B)
  % obtain (X,Y) boundary coordinates corresponding to label 'k'
  boundary = B{k};

  % compute a simple estimate of the object's perimeter
  delta_sq = diff(boundary).^2;
  perimeter = sum(sqrt(sum(delta_sq,2)));

  % obtain the area calculation corresponding to label 'k'
  area = stats(k).Area;

  % compute the roundness metric
  metric(k) = 4*pi*area/perimeter^2;
end 

% Find max roundness region
[val,idx] = max(metric);

fprintf('max roudness = %f\n', val);

pause(1);

if( val > Thresh_ROUND )
    % Find IRIS
    centroid = stats(idx).Centroid

    % We can use ransac at here
    PARAM = fit_ellipse(B{idx}(:,2),B{idx}(:,1));

    % set parameter and draw boundary
    iris_param(1) = PARAM.a;
    iris_param(2) = PARAM.b;
    iris_param(3) = -PARAM.phi;
    iris_param(4) = PARAM.X0_in;
    iris_param(5) = PARAM.Y0_in;

    % find CORNEA
    param2 = find_cornea5( I2, iris_param, 1000 );

    current_result = param2;
else
    % find CORNEA using last result
    param(3) = param2(3);
    param(4) = param2(4);
    param(5) = param2(5);
    param2 = find_cornea5( I2, iris_param, 1000 );
    current_result = param2;        
end











