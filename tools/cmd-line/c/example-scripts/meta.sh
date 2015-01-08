#!/bin/bash
# MIO Meta tools example script

# Delete metaexample node if it is already present
../bin/mio_node_delete -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Create metaexample node
../bin/mio_node_create -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a Philips Hue device to the metaexample node
../bin/mio_meta_add -event metaexample -name hue -type device -info "Philips Hue lightbulb" -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a brightness transducer
../bin/mio_meta_transducer_add -event metaexample -name brightness -type brightness -unit percentage -min 0 -max 100 -u testuser@sensor.andrew.cmu.edu -p testuser

# In case something changes or we forgot something, we can update individual attributes of the device, transducer or property (Note that enum values are overwritten and not merged):
../bin/mio_meta_transducer_add -event metaexample -name brightness -unit lux -max 1000 -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a color transducer. Note that we can add comma seperated enum values as units
../bin/mio_meta_transducer_add -event metaexample -info testinfo -name color -type color -unit enum -enum_names "red,green,blue,white" -enum_values "0,1,2,3" -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a custom property to the device, indicating when it was installed
../bin/mio_meta_property_add -event metaexample -name purchaseDate -value 10/10/10 -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a custom property to the color transducer, indicating what color space is being used
../bin/mio_meta_property_add -event metaexample -t_name color -name colorSpace -value RGB -u testuser@sensor.andrew.cmu.edu -p testuser

# Add a second custom property to the color transducer, indicating what the default color being used
../bin/mio_meta_property_add -event metaexample -t_name color -name defaultColor -value White -u testuser@sensor.andrew.cmu.edu -p testuser

# Let's try reading the meta data from our new node
../bin/mio_meta_query -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser

# We can also remove properties, transducers or entire devices
#../bin/mio_meta_property_remove -event metaexample -name purchaseDate -u testuser@sensor.andrew.cmu.edu -p testuser

# Let's try reading the meta data from our new node
#../bin/mio_meta_query -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser

#../bin/mio_meta_geoloc_add -event metaexample -uri testURL -datum testdatum -accuracy 1.02 -lat 11.52 -lon -5.3 -building CIC -country USA -country_code US -zip 15213 -floor 2 -u testuser@sensor.andrew.cmu.edu -p testuser

# Add the same geoloc to the brightness transducer
#../bin/mio_meta_geoloc_add -event metaexample -t_name brightness -uri testURL -datum testdatum -accuracy 1.02 -lat 11.52 -lon -5.3 -building CIC -country USA -country_code US -zip 15213 -floor 2 -u testuser@sensor.andrew.cmu.edu -p testuser

# Let's try reading the meta data from our node
#../bin/mio_meta_query -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser

# Remove the geoloc from the brightness transducer
#../bin/mio_meta_geoloc_remove -event metaexample -t_name brightness -u testuser@sensor.andrew.cmu.edu -p testuser

# Let's try reading the meta data from our node
#../bin/mio_meta_query -event metaexample -u testuser@sensor.andrew.cmu.edu -p testuser


