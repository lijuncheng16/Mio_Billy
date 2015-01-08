function [Raw,Data,name] = automap_get_data(paras)
% function used to get csv data from server and parse the csv files to Data

toggle_get_data = paras.toggle_get_data;

addpath(genpath('../package'))

tic
local_path = './CSV/';

if toggle_get_data
    
    delete('CSV\*.csv')
    cd ../package/ssh2_v2_m1_r6/ssh2_v2_m1_r6/
    ssh2_setup
    cd ../../../code/
    HOSTNAME = paras.HOSTNAME;
    USERNAME = paras.USERNAME;
    PASSWORD = paras.PASSWORD;
    
    remote_path = '/home/respawn/datastore2/jingkun/CSV';
    
    ssh2_conn = ssh2_config(HOSTNAME,USERNAME,PASSWORD);
    ssh2_conn = ssh2_command(ssh2_conn, ['ls ',remote_path],1);
    ssh2_conn = scp_get(ssh2_conn,ssh2_conn.command_result,local_path,remote_path);
    ssh2_conn = ssh2_close(ssh2_conn);
end

%% process the files for incorrct timestamps, find out start and end time
files = dir([local_path '/*.csv']);
num_files = length(files);
Data = cell(0);
name = cell(0);
T_start = cell(0);
T_end = cell(0);
count = 0;
for i = 1:num_files
    Temp = csvread([local_path '/' files(i).name]);
    
    if length(Temp) > 1000
        Idx = find(diff(Temp(:,1))<=0); % idx of werid timestamp (time reverse)
        
        % if exists, correct it until every timestamp is correct
        while ~isempty(Idx)
            
            Temp(Idx,:) = (Temp(Idx,:)+Temp(Idx+1,:))/2;
            Temp(Idx+1,:) = [];
            Idx = find(diff(Temp(:,1))<=0);
        end
        Temp(:,1) = Temp(:,1)/86400+datenum(1970,1,1); % convert unixt time to matlab time
        count = count+1;
        Data{count} = Temp;
        T_start{i} = Temp(1,1);
        T_end{i} = Temp(end,1);
        name{count} = files(count).name;
    end
end

Raw = Data;

%% data cleaning to removing data streams with constant values 
N = length(Raw);
Ix = [];
for i = 1:N
    temp = Data{i}(:,2);
    if sum(abs(diff(temp))) < .01
        Ix = [Ix i];
    end
end

Raw(:,Ix) = [];
Data(:,Ix) = [];
N = length(Raw);

for i = 1:N
    plot(Data{i}(:,1),Data{i}(:,2))
    datetick('x','mm/dd')
    title(i)
    pause
end


%% interpolation to have the same length
t_start = max(cell2mat(T_start));
t_end = min(cell2mat(T_end));
t = datenum(t_start):1/(60*24):datenum(t_end);


Data = zeros(N,length(t));
for i = 1:N
    Data(i,:) = interp1(Raw{i}(:,1),Raw{i}(:,2),t'); % interpolation
end


end