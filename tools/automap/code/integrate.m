function integrate(info,paras)

toggle = paras.data_availability;

% get all the event nodes and registry ids
event_node_registy_ids = info.linear_corr_agglomerative_cluster(:,1);

% loop to separate event nodes and registry ids
n = length(event_node_registy_ids);
node = cell(n,1);
registry = cell(n,1);
for i=1:n
    temp = strsplit(event_node_registy_ids{i},'_');
    node{i} = temp{1};
    temp = strsplit(temp{2},'.');
    registry{i} = temp{2};
end

% the same event node(device) can have multiple registry ids(transducers),
% we find out the unique nodes and query cmd-line tool for meta info
[uq_node,ix_node] = unique(node);

if ispc
    % prepare the command in windows
    path_win = 'C:\cygwin64\bin\bash --login -c "/home/jingkungao/code_repo/mio/tools/cmd-line/CLI_strophe/bin/'
elseif isunix
    path_unix = '~mio/tools/cmd-line/CLI_strophe/bin/'
else
    disp('wrong os')
end

command_query = 'mio_meta_query -event '
command_post = ' -u testuser@sensor.andrew.cmu.edu -p testuser"'

% run command to query meta info for all nodes (we don't store any meta
% info in the node cuurently)
if toggle
    n_uq = length(uq_node);
    for i=1:n_uq
        command_eventnode = [uq_node{i} '_meta'];
        final_command = [path_win command_query command_eventnode command_post];
        [~,meta_info{i}] = system(final_command);
    end
end

% an example that contain meta info
command_eventnode = 'metaexample';
final_command = [path_win command_query command_eventnode command_post];
[~,example_info] = system(final_command);

% extract the location information from meta info (here we use 'Floor')

for i=1:n_uq
    temp = regexp(meta_info{i},'Floor');
    loca_info{i} = meta_info{i}(temp+7:temp+11);
end

% check if the node_registry_ids in the same group are in the same
% location. Do nothing if true. If not, vote to find out the most likely
% location and change the info through cmd-line tools
command_add_geoloc = 'mio_meta_geoloc_add -event '
[uq_gp,ix_gp] = unique(unique(cell2mat(info.linear_corr_agglomerative_cluster(:,2))));
n_gp = length(uq_gp);
if toggle
    for i=1:n_gp
        % for each group, find out the nodes and query meta data info
        if i==n_gp
            temp_info = loca_info(ix_gp(i):end,:);
            temp_nodes = node(ix_gp(i):end,:)
        else
            temp_info = loca_info(ix_gp(i):ix_gp(i+1),:);
            temp_nodes = node(ix_gp(i):ix_gp(i+1),:);
        end
        % query and check the nodes in the same group to see if they have the same
        % location info
        if length(temp_info) < 2; continue; end
        j = 1;
        check_result = true;
        while j < length(temp_info)
            str1 = temp_info{j};
            str2 = temp_info{j+1};
            str1(ismember(' ')) = [];
            str2(ismember(' ')) = [];
            check_result = check_result && strcmp(str1,str2);
            if ~check_result
                break
            end
            j = j+1;
        end
        % if check_result is true, do nothing
        % otherwise, vote to find out the most likely location(mll)
        [~,ix_info] = unique(temp_info);
        [~,ix_mll] = max(diff(ix_info));
        most_likely_location = temp_info{ix_mll};
        
        % use cmd-line tool to update
        for j = 1:length(temp_info)
            command_eventnode = [temp_nodes{j} '_meta'];
            command_loc_update = [' -floor ' most_likely_location];
            final_command = [path_win command_add_geoloc command_eventnode command_loc_update command_post];
            [~,respons] = system(final_command);
        end
        
    end
end
% an example that update the meta info
command_eventnode = 'metaexample';
command_loc_update = [' -floor 5'];
final_command = [path_win command_add_geoloc command_eventnode command_loc_update command_post];
[~,respons] = system(final_command);

end
