#!/bin/bash
# MIO schedule tools example script

# Delete scheduleexample node if it is already present
../bin/mio_node_delete -event scheduleexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Create metaexample node
../bin/mio_node_create -event scheduleexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Schedule three events
../bin/mio_schedule_event_add -event scheduleexample -t_name atrasducer -t_value 22 -time "2014-08-22T14:00:00.412338-0500" -info "First test event" -r_freq daily -r_int 1 -r_count 5 -r_until Monday -r_bymonth 1 -r_byday 2 -r_exdate Monday -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_schedule_event_add -event scheduleexample -t_name atrasducer -t_value 23 -time "2014-08-23T14:00:00.412338-0500" -info "First test event" -u testuser@sensor.andrew.cmu.edu -p testuser
../bin/mio_schedule_event_add -event scheduleexample -t_name atrasducer -t_value 24 -time "2014-08-23T15:00:00.412338-0500" -info "First test event" -u testuser@sensor.andrew.cmu.edu -p testuser

# Check if all events were added
../bin/mio_schedule_query -event scheduleexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Update the time and transducer value of event with ID 1
../bin/mio_schedule_event_add -event scheduleexample -t_value 20 -time "2014-08-21T14:00:00.412338-0500" -id 1 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check if event was updated
../bin/mio_schedule_query -event scheduleexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Remove event with ID 0
../bin/mio_schedule_event_remove -event scheduleexample -id 0 -u testuser@sensor.andrew.cmu.edu -p testuser

# Check if event was removed
../bin/mio_schedule_query -event scheduleexample -u testuser@sensor.andrew.cmu.edu -p testuser

