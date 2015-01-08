clear;clc; close all
addpath(genpath('../package'))

%% get data from server
paras.toggle_get_data = true;
paras.HOSTNAME = 'sensor2.andrew.cmu.edu';
paras.USERNAME = '*****';
paras.PASSWORD = '*****';
[Raw,Data,name] = automap_get_data(paras);

%% run automapping agent to get group information

paras.cutoff = 0.7;
info = automap_agent(Data,[],paras);

%% integrate to mio, update meta info based on automapping results
paras.data_availability = false;
% a toy example to update meta data info for node metaexample
integrate(info, paras);

