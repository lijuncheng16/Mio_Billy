#!/bin/bash

CONFIG_PATH=/etc/mortar/mortar.cfg

# find server
# check for configuration
if [ -s $CONFIG_PATH ] 
then
  CONFIG=$(cat "$CONFIG_PATH")
  line_ind=0
  while read -r $line ;
  do
    line_ind=($line_ind + 1); 
    if [ $line_ind -eq 0 ]
    then
      JID=$line
    elif [ $line_ind -eq 1 ]
    then
      PASSWORD=$line
    fi
  done <<< "$CONFIG"
else
  # if none begin bluetoooth username setup
  exit 0
fi

# parser user from JID
USER=$(echo $JID | sed 's/@.*//g')
DOMAIN=$(echo $JID | sed 's/.*@//g')

# begin ipc etc

# begin scheduler

# create adapter node
ADAPTER_NODE="$USER_adapters"
RESULT=$(mio_node_create -event $ADAPTER_NODE -u $JID -p "$PASSWORD")

RESULT=$(mio_acl_publisher_add -event $ADAPTER_NODE -u $JID -p "$PASSWORD" -publisher admin@$DOMAIN)
#begin respawn

while [ 1 ] ;
do
  # begin gateway adapter
  RESULT=$(adapter_listener $JID $PASSWORD $ADAPTER_NODE)
  sleep 10
done

exit 0
