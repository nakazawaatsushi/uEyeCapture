%% Check image sequence which is the most bright and daplrk

clear;

[fname,fpath] = uigetfile('*.*');

for i=0:1000
    fname = sprintf('%s%04d.png',fpath,i);
    
    if exist(fname,'file')
        I = imread(fname);
        if size(I,3) == 1
            bright(i+1) = sum(sum(I));
        else
            bright(i+1) = sum(sum(sum(I)));
        end
        
        fprintf(1,'File %s: brightness %d\n', fname, bright(i+1));
    end
end
