#!/bin/bash

#screen -dmS store ./store_xmpp.py -j user@host -p password

cd bt_datastore
screen -dmS http ./tile_server 4720 ../www/

echo "store agent and tile server started in screen. use 'screen -ls' to view..."
