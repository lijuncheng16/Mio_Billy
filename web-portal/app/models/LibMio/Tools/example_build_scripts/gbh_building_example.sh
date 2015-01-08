#!/bin/bash
# GBH Building

# Remove the nodes if they already exist
../mio_node_delete -event root -u jdm@localhost -p jdm
../mio_node_delete -event floor_1 -u jdm@localhost -p jdm
../mio_node_delete -event floor_2 -u jdm@localhost -p jdm
../mio_node_delete -event lobby -u jdm@localhost -p jdm
../mio_node_delete -event it_office -u jdm@localhost -p jdm
../mio_node_delete -event gbh_office -u jdm@localhost -p jdm
../mio_node_delete -event ges_torres -u jdm@localhost -p jdm
../mio_node_delete -event door_lock -u jdm@localhost -p jdm
../mio_node_delete -event ac_unit -u jdm@localhost -p jdm
../mio_node_delete -event jdm_favorites -u jdm@localhost -p jdm
../mio_node_delete -event jdm_lights -u jdm@localhost -p jdm
../mio_node_delete -event jdm_cameras -u jdm@localhost -p jdm

# Create building
../mio_node_create -event root -title GBH -u jdm@localhost -p jdm
../mio_meta_add -event root -name "GBH Building" -type location -u jdm@localhost -p jdm

# Create building floors
../mio_node_create -event floor_1 -title "Floor 1" -u jdm@localhost -p jdm
../mio_meta_add -event floor_1 -name floor_1 -type location -u jdm@localhost -p jdm
../mio_node_create -event floor_2 -title "Floor 2" -u jdm@localhost -p jdm
../mio_meta_add -event floor_2 -name floor_2 -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child floor_1 -parent root -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child floor_2 -parent root -u jdm@localhost -p jdm

# Create rooms floor 1
../mio_node_create -event lobby -title "Lobby" -u jdm@localhost -p jdm
../mio_meta_add -event lobby -name lobby -type location -u jdm@localhost -p jdm
../mio_node_create -event it_office -title "IT Office" -u jdm@localhost -p jdm
../mio_meta_add -event it_office -name it_office -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child lobby -parent floor_1 -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child it_office -parent floor_1 -u jdm@localhost -p jdm
# Add device to lobby
../mio_node_create -event door_lock -title "Door Lock" -u jdm@localhost -p jdm
../mio_meta_add -event door_lock -name door_lock -type device -info "LG Door Lock" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child door_lock -parent lobby -u jdm@localhost -p jdm
../mio_meta_transducer_add -event door_lock -name mode -type mode -unit enum -enum_names "close,open" -enum_values "0,1" -u jdm@localhost -p jdm
../mio_schedule_event_add -event door_lock -t_name mode -t_value 1 -time "2014-08-22T07:30:00.412338-0500" -info "Remove lock at work start" -r_freq "DAILY" -r_int 1 -r_count 10 -r_until "2014-08-22T07:30:00.412338-0500" -r_bymonth "0,1,2,3" -r_byday "MO,TU" -r_exdate "2014-08-22" -u jdm@localhost -p jdm
../mio_schedule_event_add -event door_lock -t_name mode -t_value 0 -time "2014-08-22T18:15:00.412338-0500" -info "Lock door after work close" -u jdm@localhost -p jdm
../mio_publish_data -event door_lock -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event door_lock -id mode -value 1 -rawvalue 1 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event door_lock -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm
../mio_meta_query -event door_lock -u jdm@localhost -p jdm
../mio_schedule_query -event door_lock -u jdm@localhost -p jdm

# add device to it_office
../mio_node_create -event ac_unit -title "Air Unit" -u jdm@localhost -p jdm
../mio_meta_add -event ac_unit -name ac_unit -type device -info "LG Air Unit" -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child ac_unit -parent it_office -u jdm@localhost -p jdm
../mio_meta_transducer_add -event ac_unit -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u jdm@localhost -p jdm
../mio_meta_transducer_add -event ac_unit -name temperature -type temperature -unit celcius -min 15 -max 50 -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit -t_name mode -t_value 2 -time "2014-08-22T07:30:00.412338-0500" -info "Turn AC Unit on" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit -t_name temperature -t_value 22 -time "2014-08-22T07:31:00.412338-0500" -info "Set a cold temperature after turning on" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit -t_name temperature -t_value 26 -time "2014-08-22T12:30:00.412338-0500" -info "Set a warm temperature at lunch hours" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit -t_name temperature -t_value 22 -time "2014-08-22T14:00:00.412338-0500" -info "Set a cold temperature after lunch" -u jdm@localhost -p jdm
../mio_schedule_event_add -event ac_unit -t_name mode -t_value 0 -time "2014-08-22T18:15:00.412338-0500" -info "Turn AC Unit off" -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit -id mode -value 0 -rawvalue 0 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit -id temperature -value 22 -rawvalue 22 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event ac_unit -id mode -value 1 -rawvalue 1 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit -id temperature -value 40 -rawvalue 40 -u jdm@localhost -p jdm
sleep 2
../mio_publish_data -event ac_unit -id mode -value 2 -rawvalue 2 -u jdm@localhost -p jdm
../mio_publish_data -event ac_unit -id temperature -value 20 -rawvalue 20 -u jdm@localhost -p jdm
../mio_meta_query -event ac_unit -u jdm@localhost -p jdm
../mio_schedule_query -event ac_unit -u jdm@localhost -p jdm

# Create rooms floor 2
../mio_node_create -event gbh_office -title "GBH Office" -u jdm@localhost -p jdm
../mio_meta_add -event gbh_office -name gbh_office -type location -u jdm@localhost -p jdm
../mio_node_create -event ges_torres -title "GES Torres" -u jdm@localhost -p jdm
../mio_meta_add -event ges_torres -name ges_torres -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child gbh_office -parent floor_2 -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child ges_torres -parent floor_2 -u jdm@localhost -p jdm

#create favorites
../mio_node_create -event jdm_favorites -title Favorites -u jdm@localhost -p jdm
../mio_meta_add -event jdm_favorites -name "Favorites" -type location -u jdm@localhost -p jdm
../mio_node_create -event jdm_lights -title Light -u jdm@localhost -p jdm
../mio_meta_add -event jdm_lights -name "Lights" -type location -u jdm@localhost -p jdm
../mio_node_create -event jdm_cameras -title Cameras -u jdm@localhost -p jdm
../mio_meta_add -event jdm_cameras -name "Cameras" -type location -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child jdm_lights -parent jdm_favorites -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child jdm_cameras -parent jdm_favorites -u jdm@localhost -p jdm
../mio_reference_child_add -add_ref_child -child ac_unit -parent jdm_lights -u jdm@localhost -p jdm

# Check if children were added correctly
../mio_reference_query -event root -u jdm@localhost -p jdm
../mio_reference_query -event floor_1 -u jdm@localhost -p jdm
../mio_reference_query -event lobby -u jdm@localhost -p jdm
../mio_reference_query -event it_office -u jdm@localhost -p jdm
../mio_reference_query -event floor_2 -u jdm@localhost -p jdm
../mio_reference_query -event gbh_office -u jdm@localhost -p jdm
../mio_reference_query -event ges_torres -u jdm@localhost -p jdm