#! /bin/bash

# Script to initialize event node
clear
USERNAME=$1
PASSWORD=$2
NAME=$3 
ROOT_PATH=$4
UUID="$(uuid)"
TYPE="lutron_area"

echo "NAME is $NAME"
echo "UUID is $UUID"
echo "USERNAME is $USERNAME"
echo "TYPE is $TYPE"

echo "../../tools/cmd-line/c/bin/mio_node_create -event $UUID -access open -title $NAME -u $USERNAME -p $PASSWORD"
../../tools/cmd-line/c/bin/mio_node_create -event $UUID -access open -title $NAME -u $USERNAME -p $PASSWORD

./bin/meta_tool -n $NAME -id $UUID -t $TYPE -u $USERNAME -p $PASSWORD

../../tools/cmd-line/c/bin/mio_reference_child_add -parent $ROOT_PATH -child $UUID -add_ref_child -u $USERNAME -p $PASSWORD
exit 0
