#!/bin/bash

#login
./mio_authenticate -u admin@sensor.andrew.cmu.edu -p summernegr0n1
# get user data
sudo ejabberdctl private_get admin sensor.andrew.cmu.edu user http://mortar.io/user

# Getting the users permitted devices and starting the device browser, occur asynchronously
# Get user's permitted devices (Owner, publisher, subscriber of)
./mio_acl_affiliations_query -u admin@sensor.andrew.cmu.edu -p summernegr0n1 -stanza
./mio_subscriptions_query -u admin@sensor.andrew.cmu.edu -p summernegr0n1

# Start device browser, get root metadata and get root children
./mio_item_query_stanza -event root -item meta -u admin@sensor.andrew.cmu.edu -p summernegr0n1
./mio_item_query_stanza -event root -item references -u admin@sensor.andrew.cmu.edu -p summernegr0n1