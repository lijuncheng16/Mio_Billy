function info = automap_agent(Data,method,paras)
% The automapping agents takes in the path of a folder which contains all
% the csv files from datastreams and outputs info as a strcuture to
% indicate the meta data information regarding those datastreams.
%
% v0.2: We assume the simplest case, where each data stream is recorded
% during the same period of time and contains the same length of data. The
% output info will contain the group information bases on the correlations
% among those different datastreams.
%
% example: path='/Users/jingkungao/Dropbox/Research/Project/Sensor Andrew/mine/automapping/Example_data';
%          info = automap_agent(path);
%
% Author: Jingkun Gao(jingkung@andrew.cmu.edu)

addpath(genpath('../package'))

% calculate distances using linear correlation
[Cor,p] = corr(Data');
Cor2 = Cor./(p+eps);
Cor2 = Cor2/max(Cor2(:));

%%% calculate distances using MMD
%
% tic
% mult = 1;
% paras.co = DMMD_online_initialization(mult);
% Matrix_D_MMD = get_matrix(Y,@DMMD_online_estimation,paras);
% toc % 721s
%%
% group different datastreams using clustering algorithms
cutoff = paras.cutoff; % a positive scalar for inconsistent or distance measure
T = clusterdata(Cor,cutoff);

[info.linear_corr_agglomerative_cluster(:,2),ix] = sort(T);
info.linear_corr_agglomerative_cluster = num2cell(info.linear_corr_agglomerative_cluster);
info.linear_corr_agglomerative_cluster(:,1) = name(ix);

end
