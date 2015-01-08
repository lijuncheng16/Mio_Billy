#!/bin/bash
# MIO Meta tools example script

# Remove example nodes if they already exist
../bin/mio_node_delete -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_delete -event refparent2 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_delete -event refchild1 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_delete -event refchild2 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_delete -event refchild3 -u testuser@sensor.andrew.cmu.edu -p testuser

# Create example nodes
../bin/mio_node_create -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_create -event refparent2 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_create -event refchild1 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_create -event refchild2 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_node_create -event refchild3 -u testuser@sensor.andrew.cmu.edu -p testuser

# Populate nodes with meta data so that the meta_type references for each reference can be populated
../bin/mio_meta_add -event refparent1 -name refparent1 -type device -info "This node contains a device" -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_meta_add -event refparent2 -name refparent2 -type location -info "This node contains a location" -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_meta_add -event refchild1 -name refchild1 -type device -info "This node contains a device" -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_meta_add -event refchild2 -name refchild2 -type device -info "This node contains a device" -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_meta_add -event refchild3 -name refchild3 -type device -info "This node contains a device" -u testuser@sensor.andrew.cmu.edu -p testuser

# Add all three children to refparent
../bin/mio_reference_child_add -child refchild1 -parent refparent1 -add_ref_child -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_reference_child_add -child refchild2 -parent refparent1 -add_ref_child -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_reference_child_add -child refchild3 -parent refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check if children were added corrctly
../bin/mio_reference_query -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Add refparent1 as the child of refparent2
../bin/mio_reference_child_add -child refparent1 -parent refparent2 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check if the child was added corrctly
../bin/mio_reference_query -event refparent2 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check to see if refparent2 is the parent of refparent1
../bin/mio_reference_query -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Remove refchild2 from refparent1
../bin/mio_reference_child_remove -child refchild2 -parent refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check to see if refchild2 has been removed
../bin/mio_reference_query -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_reference_query -event refchild2 -u testuser@sensor.andrew.cmu.edu -p testuser

# Change the meta type of refchild3 to location
../bin/mio_meta_add -event refchild3 -name refchild3 -type location -info "This node contains a location" -u testuser@sensor.andrew.cmu.edu -p testuser

# Check to see if meta type changed for refchild3 in the references of parent1
../bin/mio_reference_query -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Remove the meta data from refchild1
../bin/mio_meta_remove -event refchild1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check to see if meta type is now unknown for refchild1 in the references of parent1
../bin/mio_reference_query -event refparent1 -u testuser@sensor.andrew.cmu.edu -p testuser
