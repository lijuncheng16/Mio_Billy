#!/bin/bash

# GBH Building
# Remove the nodes if they already exist
../mio_node_delete -event root -u admin@localhost -p admin
../mio_node_delete -event admin_favorites  -u admin@localhost -p admin
../mio_node_delete -event jose_favorites -u jose@localhost -p jose
../mio_node_delete -event javier_favorites -u javier@localhost -p javier
../mio_node_delete -event "DA610D89-12C6-4293-B0BE-E20FFB0681EE"  -u admin@localhost -p admin
../mio_node_delete -event "78E7435A-3A2B-45E0-9F60-A24AA545C031"  -u admin@localhost -p admin
../mio_node_delete -event "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B"  -u admin@localhost -p admin
../mio_node_delete -event "556F354A-D3AE-4E4D-B19F-A44A548A849E"  -u admin@localhost -p admin
../mio_node_delete -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56"  -u admin@localhost -p admin
../mio_node_delete -event "51AC95D5-98AA-4704-8308-19C5C2E3F691"  -u admin@localhost -p admin
../mio_node_delete -event "3CD781B8-0216-4E6E-917A-196FB4A80355"  -u admin@localhost -p admin
../mio_node_delete -event "D4485E26-70AD-4496-945D-FB6C9CF91ED8"  -u admin@localhost -p admin
../mio_node_delete -event "E691ED56-18E2-4C69-930D-9D2AC1E41356"  -u admin@localhost -p admin
../mio_node_delete -event "E21C3E37-507E-436A-B4B5-A066F7EB73BB"  -u admin@localhost -p admin
../mio_node_delete -event "208EF795-ED19-47E0-9965-F42F6EE7D175"  -u admin@localhost -p admin


# Set admins metadata
 ejabberdctl private_set admin localhost "<user xmlns='http://mortar.io/user'><name>Admin</name><email>admin@localhost</email><group>admin</group></user>"
 ejabberdctl register jose localhost jose
 ejabberdctl register javier localhost javier
 ejabberdctl private_set jose localhost "<user xmlns='http://mortar.io/user'><name>Jose</name><email>jose@localhost</email><group>user</group></user>"
 ejabberdctl private_set javier localhost "<user xmlns='http://mortar.io/user'><name>Javier</name><email>javier@localhost</email><group>user</group></user>"
# Create building
../mio_node_create -event root -title root -u admin@localhost -p admin
../mio_meta_add -event root -name "Building" -type location -u admin@localhost -p admin

# Create Admins Fav folder
../mio_node_create -event admin_favorites -title "Favorites" -u admin@localhost -p admin
../mio_meta_add -event admin_favorites -name "Favorites" -type location -u admin@localhost -p admin
# Create Jose Favs
../mio_node_create -event jose_favorites -title "Favorites" -u jose@localhost -p jose
../mio_meta_add -event jose_favorites -name "Favorites" -type location -u jose@localhost -p jose
# Create Javier Favs
../mio_node_create -event javier_favorites -title "Favorites" -u javier@localhost -p javier
../mio_meta_add -event javier_favorites -name "Favorites" -type location -u javier@localhost -p javier

# Create building floors
../mio_node_create -event "DA610D89-12C6-4293-B0BE-E20FFB0681EE" -title "Floor 1" -u admin@localhost -p admin
../mio_meta_add -event "DA610D89-12C6-4293-B0BE-E20FFB0681EE" -name "Floor 1" -type location -u admin@localhost -p admin
../mio_node_create -event "78E7435A-3A2B-45E0-9F60-A24AA545C031" -title "Floor 2" -u admin@localhost -p admin
../mio_meta_add -event "78E7435A-3A2B-45E0-9F60-A24AA545C031" -name "Floor 2" -type location -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "DA610D89-12C6-4293-B0BE-E20FFB0681EE" -parent root -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "78E7435A-3A2B-45E0-9F60-A24AA545C031" -parent root -u admin@localhost -p admin

# Create rooms floor 1
../mio_node_create -event "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B" -title "Lobby" -u admin@localhost -p admin
../mio_meta_add -event "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B" -name "Lobby" -type location -u admin@localhost -p admin
../mio_node_create -event "556F354A-D3AE-4E4D-B19F-A44A548A849E" -title "Office" -u admin@localhost -p admin
../mio_meta_add -event "556F354A-D3AE-4E4D-B19F-A44A548A849E" -name "Office" -type location -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B" -parent "DA610D89-12C6-4293-B0BE-E20FFB0681EE" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "556F354A-D3AE-4E4D-B19F-A44A548A849E" -parent "DA610D89-12C6-4293-B0BE-E20FFB0681EE" -u admin@localhost -p admin
# Add device to Lobby
../mio_node_create -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -title "Door Lock" -u admin@localhost -p admin
../mio_meta_add -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -name "Door Lock" -type device -info "LG Door Lock" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -parent "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B" -u admin@localhost -p admin
../mio_meta_transducer_add -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -name mode -type mode -unit enum -enum_names "close,open" -enum_values "0,1" -u admin@localhost -p admin
../mio_schedule_event_add -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -t_name mode -t_value 1 -time "2014-08-22T07:30:00.412338-0500" -info "Remove lock at work start" -r_freq "DAILY" -r_int 1 -r_count 10 -r_until "2014-08-22T07:30:00.412338-0500" -r_bymonth "0,1,2,3" -r_byday "MO,TU" -r_exdate "2014-08-22" -u admin@localhost -p admin
../mio_schedule_event_add -event "8D982A4C-EAD1-46AF-AA67-BC996E857A56" -t_name mode -t_value 0 -time "2014-08-22T18:15:00.412338-0500" -info "Lock door after work close" -u admin@localhost -p admin
../mio_node_create -event "51AC95D5-98AA-4704-8308-19C5C2E3F691" -title "Air Unit" -u admin@localhost -p admin
../mio_meta_add -event "51AC95D5-98AA-4704-8308-19C5C2E3F691" -name "Air Unit" -type device -info "LG Air Unit" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "51AC95D5-98AA-4704-8308-19C5C2E3F691" -parent "BC7BA8C1-34FD-42FF-BDDB-8526B4E9F82B" -u admin@localhost -p admin
../mio_meta_transducer_add -event "51AC95D5-98AA-4704-8308-19C5C2E3F691" -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u admin@localhost -p admin
../mio_meta_transducer_add -event "51AC95D5-98AA-4704-8308-19C5C2E3F691" -name temperature -type temperature -unit celcius -min 15 -max 50 -u admin@localhost -p admin
../mio_schedule_event_add -event "51AC95D5-98AA-4704-8308-19C5C2E3F691" -t_name mode -t_value 0 -time "2014-09-22T12:00:00.412338-0500" -info "Turn AC Unit off" -u admin@localhost -p admin


# add device to Office
../mio_node_create -event "3CD781B8-0216-4E6E-917A-196FB4A80355" -title "Air Unit" -u admin@localhost -p admin
../mio_meta_add -event "3CD781B8-0216-4E6E-917A-196FB4A80355" -name "Air Unit" -type device -info "LG Air Unit" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "3CD781B8-0216-4E6E-917A-196FB4A80355" -parent "556F354A-D3AE-4E4D-B19F-A44A548A849E" -u admin@localhost -p admin
../mio_meta_transducer_add -event "3CD781B8-0216-4E6E-917A-196FB4A80355" -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u admin@localhost -p admin
../mio_meta_transducer_add -event "3CD781B8-0216-4E6E-917A-196FB4A80355" -name temperature -type temperature -unit celcius -min 15 -max 50 -u admin@localhost -p admin

# Create rooms floor 2
../mio_node_create -event "D4485E26-70AD-4496-945D-FB6C9CF91ED8" -title "Jose's Office" -u admin@localhost -p admin
../mio_meta_add -event "D4485E26-70AD-4496-945D-FB6C9CF91ED8" -name "Jose's Office" -type location -u admin@localhost -p admin
../mio_node_create -event "E691ED56-18E2-4C69-930D-9D2AC1E41356" -title "Javier's Office" -u admin@localhost -p admin
../mio_meta_add -event "E691ED56-18E2-4C69-930D-9D2AC1E41356" -name "Javier's Office" -type location -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "D4485E26-70AD-4496-945D-FB6C9CF91ED8" -parent "78E7435A-3A2B-45E0-9F60-A24AA545C031" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "E691ED56-18E2-4C69-930D-9D2AC1E41356" -parent "78E7435A-3A2B-45E0-9F60-A24AA545C031" -u admin@localhost -p admin

# add device to Jose's office
../mio_node_create -event "E21C3E37-507E-436A-B4B5-A066F7EB73BB" -title "Air Unit" -u admin@localhost -p admin
../mio_meta_add -event "E21C3E37-507E-436A-B4B5-A066F7EB73BB" -name "Air Unit" -type device -info "LG Air Unit" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "E21C3E37-507E-436A-B4B5-A066F7EB73BB" -parent "D4485E26-70AD-4496-945D-FB6C9CF91ED8" -u admin@localhost -p admin
../mio_meta_transducer_add -event "E21C3E37-507E-436A-B4B5-A066F7EB73BB" -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u admin@localhost -p admin
../mio_meta_transducer_add -event "E21C3E37-507E-436A-B4B5-A066F7EB73BB" -name temperature -type temperature -unit celcius -min 15 -max 50 -u admin@localhost -p admin

# add device to Javier's office
../mio_node_create -event "208EF795-ED19-47E0-9965-F42F6EE7D175" -title "Air Unit" -u admin@localhost -p admin
../mio_meta_add -event "208EF795-ED19-47E0-9965-F42F6EE7D175" -name "Air Unit" -type device -info "LG Air Unit" -u admin@localhost -p admin
../mio_reference_child_add -add_ref_child -child "208EF795-ED19-47E0-9965-F42F6EE7D175" -parent "E691ED56-18E2-4C69-930D-9D2AC1E41356" -u admin@localhost -p admin
../mio_meta_transducer_add -event "208EF795-ED19-47E0-9965-F42F6EE7D175" -name mode -type mode -unit enum -enum_names "off,heat,cold" -enum_values "0,1,2" -u admin@localhost -p admin
../mio_meta_transducer_add -event "208EF795-ED19-47E0-9965-F42F6EE7D175" -name temperature -type temperature -unit celcius -min 15 -max 50 -u admin@localhost -p admin
