#!/bin/bash

# GBH Building
# Remove the nodes if they already exist
../mio_node_delete -event root -u jdm@localhost -p jdm
../mio_node_delete -event admin_favorites  -u jdm@localhost -p jdm
../mio_node_delete -event floor_1  -u jdm@localhost -p jdm
../mio_node_delete -event floor_2  -u jdm@localhost -p jdm
../mio_node_delete -event lobby  -u jdm@localhost -p jdm
../mio_node_delete -event it_office -u jdm@localhost -p jdm
../mio_node_delete -event door_lock -u jdm@localhost -p jdm
../mio_node_delete -event ac_unit1 -u jdm@localhost -p jdm
../mio_node_delete -event gbh_office -u jdm@localhost -p jdm
../mio_node_delete -event ges_torres -u jdm@localhost -p jdm
../mio_node_delete -event f376d570-3ceb-11e4-916c-0800200c9a66 -u jdm@localhost -p jdm
../mio_node_delete -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -u jdm@localhost -p jdm
../mio_node_delete -event 2db717e0-3d41-11e4-916c-0800200c9a66 -u jdm@localhost -p jdm
# Set admins metadata
# sudo ejabberdctl private_set admin localhost '<user xmlns="http://mortar.io/user"><name>Admin</name><email>jdm@localhost</email><group>admin</group></userjdm
# Create building
../mio_node_create -event root -title root -u jdm@localhost -p jdm
../mio_meta_add -event root -name "Building" -type location -u jdm@localhost -p jdm

# Create Admins Fav folder
../mio_node_create -event admin_favorites -title root -u jdm@localhost -p jdm
../mio_meta_add -event admin_favorites -name "Favorites" -type location -u jdm@localhost -p jdm

# Create building floors
../mio_node_create -event floor_1 -title "Floor 1" -u jdm@localhost -p jdm
../mio_meta_add -event floor_1 -name "Floor 1" -type location -u jdm@localhost -p jdm
../mio_node_create -event floor_2 -title "Floor 2" -u jdm@localhost -p jdm
../mio_meta_add -event floor_2 -name "Floor 2" -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child floor_1 -parent root -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child floor_2 -parent root -u jdm@localhost -p jdm

# Create rooms floor 1
../mio_node_create -event lobby -title "Lobby" -u jdm@localhost -p jdm
../mio_meta_add -event lobby -name "Lobby" -type location -u jdm@localhost -p jdm
../mio_node_create -event it_office -title "IT Office" -u jdm@localhost -p jdm
../mio_meta_add -event it_office -name "IT Office" -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child lobby -parent floor_1 -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child it_office -parent floor_1 -u jdm@localhost -p jdm
# Add device to lobby
../mio_node_create -event door_lock -title "Door Lock" -u jdm@localhost -p jdm
../mio_meta_add -event door_lock -name "Door Lock" -type device -info "LG Door Lock" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child door_lock -parent lobby -u jdm@localhost -p jdm
../mio_meta_transducer_add -event door_lock -name mode -type mode -unit enum -enum_names "close,open" -enum_values "0,1" -u jdm@localhost -p jdm
../mio_schedule_event_add -event door_lock -t_name mode -t_value 1 -time "2014-08-22T07:30:00.412338-0500" -info "Remove lock at work start" -r_freq "DAILY" -r_int 1 -r_count 10 -r_until "2014-08-22T07:30:00.412338-0500" -r_bymonth "0,1,2,3" -r_byday "MO,TU" -r_exdate "2014-08-22" -u jdm@localhost -p jdm
../mio_schedule_event_add -event door_lock -t_name mode -t_value 0 -time "2014-08-22T18:15:00.412338-0500" -info "Lock door after work close" -u jdm@localhost -p jdm
../mio_publish_data -event door_lock -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event door_lock -id mode -value 1 -rawvalue 1 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event door_lock -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm

# add device to it_office
../mio_node_create -event ac_unit1 -title "Air Unit" -u jdm@localhost -p jdm
../mio_meta_add -event ac_unit1 -name "Air Unit" -type device -info "LG Air Unit" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child ac_unit1 -parent it_office -u jdm@localhost -p jdm
../mio_meta_transducer_add -event ac_unit1 -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event ac_unit1 -name temperature -type temperature -unit celcius -min 15 -max 50 -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit1 -t_name mode -t_value 2 -time "2014-08-22T07:30:00.412338-0500" -info "Turn AC Unit on" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit1 -t_name temperature -t_value 22 -time "2014-08-22T07:31:00.412338-0500" -info "Set a cold temperature after turning on" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit1 -t_name temperature -t_value 26 -time "2014-08-22T12:30:00.412338-0500" -info "Set a warm temperature at lunch hours" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit1 -t_name temperature -t_value 22 -time "2014-08-22T14:00:00.412338-0500" -info "Set a cold temperature after lunch" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit1 -t_name mode -t_value 0 -time "2014-08-22T18:15:00.412338-0500" -info "Turn AC Unit off" -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit1 -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit1 -id temperature -value 22 -rawvalue 22 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event ac_unit1 -id mode -value 1 -rawvalue 1 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit1 -id temperature -value 40 -rawvalue 40 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event ac_unit1 -id mode -value 2 -rawvalue 2 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit1 -id temperature -value 20 -rawvalue 20 -u jdm@localhost -p jdm

# Create rooms floor 2
../mio_node_create -event gbh_office -title "GBH Office" -u jdm@localhost -p jdm
../mio_meta_add -event gbh_office -name "GBH Office" -type location -u jdm@localhost -p jdm
../mio_node_create -event ges_torres -title "GES Torres" -u jdm@localhost -p jdm
../mio_meta_add -event ges_torres -name "GES Torres" -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child gbh_office -parent floor_2 -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child ges_torres -parent floor_2 -u jdm@localhost -p jdm

# This is the script to create nodes with actual data.
../mio_node_create  -event f376d570-3ceb-11e4-916c-0800200c9a66 -title "FireFly Environmental" -u jdm@localhost -p jdm
../mio_meta_property_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name device_image -value http://wise.ece.cmu.edu/redmine/attachments/download/204 -u jdm@localhost -p jdm
../mio_meta_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name "FireFly Environmental" -type device -info "Test Info" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child f376d570-3ceb-11e4-916c-0800200c9a66 -parent floor_1 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name light -type light -unit lux -min 0 -max 1024 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name temperature -type temperature -unit celsius -min -50 -max 100 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name humidity -type humidity -unit kg/kg -min 0 -max 100 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name audio -type audio -unit SPL -min 0 -max 100 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name motion -type motion -unit intensity -min 0 -max 100 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event f376d570-3ceb-11e4-916c-0800200c9a66 -name acceleration -type acceleration -unit G -min -3 -max 3 -u jdm@localhost -p jdm
../mio_publish_data -event f376d570-3ceb-11e4-916c-0800200c9a66 -id light -value 1 -rawvalue 1 -u jdm@localhost -p jdm
../mio_publish_data -event f376d570-3ceb-11e4-916c-0800200c9a66 -id light -value 5 -rawvalue 1 -u jdm@localhost -p jdm
../mio_publish_data -event f376d570-3ceb-11e4-916c-0800200c9a66 -id light -value 2 -rawvalue 1 -u jdm@localhost -p jdm

../mio_node_create  -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -title "FireFly Environmental" -u jdm@localhost -p jdm
../mio_meta_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "FireFly Environmental" -type device -info "Test Info" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child 0729c422-2312-11e4-afd3-13c37e36d1a7 -parent root -u jdm@localhost -p jdm
../mio_meta_property_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name device_image -value http://wise.ece.cmu.edu/redmine/attachments/download/204 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Daylighting Enabled" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Lighting Level" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Lighting State" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Loadshed Allowed" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Loadshed Goal" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Permanently Disable Occupancy" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 0729c422-2312-11e4-afd3-13c37e36d1a7 -name "Total Power" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm

../mio_node_create  -event 2db717e0-3d41-11e4-916c-0800200c9a66 -title "FireFly Environmental 2" -u jdm@localhost -p jdm
../mio_meta_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "FireFly Environmental 2" -type device -info "Test Info 2" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child 2db717e0-3d41-11e4-916c-0800200c9a66 -parent root -u jdm@localhost -p jdm
../mio_meta_property_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name device_image -value http://wise.ece.cmu.edu/redmine/attachments/download/204 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Accelerometer X" -type test -unit test -min 0 -max 1000 -interface "Accelerometer X" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Accelerometer Y" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Accelerometer Z" -type test -unit test -min 0 -max 1000 -interface "Accelerometer Z" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Barometer" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Battery Level" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Humidity Sensor" -type test -unit test -min 0 -max 1000 -interface "Humidity Sensor" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Light Meter" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Microphone" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Motion Sensor" -type test -unit test -min 0 -max 1000 -interface "Motion Sensor" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Ping" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "RSSI" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Thermometer Analog" -type test -unit test -min 0 -max 1000 -interface "Thermometer Analog" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event 2db717e0-3d41-11e4-916c-0800200c9a66 -name "Thermometer Digital" -type test -unit test -min 0 -max 1000 -u jdm@localhost -p jdm