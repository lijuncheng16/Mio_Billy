function meta_data = get_meta_sql(name,meta_paras)

addpath(genpath('../package'))

% split file name(eventnode_data.registyid) to find out registry ids
N = length(name);
Registry = cell(N,1);
for i = 1:N
    Temp = strsplit(name{i}, '.');
    Registry{i} = Temp(2);
end

% initialize paras to store meta data
transducer_type = cell(N,1);
manufacture = cell(N,1);
transducer_name = cell(N,1);
unit = cell(N,1);

% initialize paras for sql connections
HOSTNAME = meta_paras.HOSTNAME;
USERNAME = meta_paras.USERNAME;
PASSWORD = meta_paras.PASSWORD;
DB = 'transducerRegistry';
query = 'select transducer_types.transducerTypeName,manufacturers.manufacturerName,transducers.transducerName,units.unitsName from transducer_types, manufacturers, units, transducer_instances, transducers where transducer_types.id = transducers.transducer_type_id and manufacturers.id = transducers.manufacturer_id and units.id = transducers.unit_id and transducers.id = transducer_instances.transducer_id and transducer_instances.id = ';

% open mysql connection
mysql('open', HOSTNAME, USERNAME, PASSWORD);
% select DB
mysql('use', DB);
% run query continuously to get meta data from registry ids
for i = 1:N
    temp = strcat(query, Registry{i}, ';');
    [transducer_type{i},manufacture{i},transducer_name{i},unit{i}] = mysql(temp{1});
end
% close mysql connection
mysql('close');

meta_data = struct('transducer_type', transducer_type, 'manufacture', manufacture, 'transducer_name', transducer_name, 'unit', unit);

end