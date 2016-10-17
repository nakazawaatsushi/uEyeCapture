%%
Thresh_AREA = 2000;
Thresh_BIN = 10000/65535;
THRESH_AREA = 2000;

if exist('pupil','var') == 1
    clear pupil;
end

[fname,fpath] = uigetfile('*.*');

for FRM = 1:1000
    id = [FRM FRM+1];
    
    fname1 = sprintf('%s%04d.png',fpath,id(1));
    fname2 = sprintf('%s%04d.png',fpath,id(2));

    if exist(fname2,'file') == 0
        % if file does not found
        break;
    end
    
    TMP = imread(fname1);
    if size(TMP,3) == 3
        I{1} = ConvertFromBayer(TMP);
        TMP = imread(fname2);
        I{2} = ConvertFromBayer(TMP);
    else
        I{1} = TMP;
        I{2} = imread(fname2);
    end

    %% difference between the first and second frames and save
    Id = abs(rgb2gray(I{2}) - rgb2gray(I{1}));
    imwrite(Id, sprintf('%s%04d-%04d.png',fpath,id(1),id(2)));
    figure(2), imshow(Id*1000);

    %%

    G = fspecial('gaussian',10,0.3);
    II = imfilter(1000*Id,G);
    bw0 = im2bw(II, Thresh_BIN);
    bw = abs(ones(size(bw0)) - bw0);

    % remove all object containing fewer than Thresh_AREA pixels
    bw = bwareaopen(bw,THRESH_AREA);

    % show images
    figure(3);
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

      if area < THRESH_AREA
          metric(k) = 0.0;
      else
        % compute the roundness metric
        metric(k) = 4*pi*area/perimeter^2;
      end
    end 

    if length(B) > 2
        %% Find max roundness region
        [val,idx] = max(metric);

        fprintf('max roudness = %f\n', val);
        plot(stats(idx).Centroid(1), stats(idx).Centroid(2), 'g+');

        %% óÃàÊèÓïÒ
        boundary = B{idx};
        figure(3); imshow(10*bw);
        hold on, plot(boundary(:,2), boundary(:,1),'r.');
        
        saveas(gcf, sprintf('%s%04d-out.png',fpath,id(1)));
        
        %% óÃàÊèÓïÒÇï€ë∂
        if exist('pupil','var') == 0
            pupil{1}.frm = FRM;
            pupil{1}.boundary = boundary;
        else
            pupil{numel(pupil)+1}.frm = FRM;
            pupil{numel(pupil)}.boundary = boundary;
        end
        pause;
    end
end

save(sprintf('%spupil_boundary.mat',fpath),'pupil');
