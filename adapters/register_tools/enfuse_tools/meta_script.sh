#!/bin/bash

USAGE="./meta_script.sh [username] [password] [in_path] [out_path] [root_uuid]"

USERNAME=$1
PASSWORD=$2

# where to print final configuration
CONFIG_PATH=$3
EVENT_PATH=$4

ROOT_UUID=$5

USER_FLAG="-u $USERNAME -p $PASSWORD"

# check authentication
RESULT='../../tools/cmd-line/c/bin/mio_node_create'

CONFIG=$(cat $CONFIG_PATH) 

echo "$CONFIG"
touch $EVENT_PATH

while read -r config_line; 
do

  split_line=$(echo "$config_line" | tr ',' "\n" )
  UUID=$(uuid)
  TYPE=$(echo "$split_line" | sed -n -e 1p)
  NAME=$(echo "$split_line" | sed -n -e 2p)

  echo "$TYPE"
  if [ "$TYPE" = "enfuse_location" ]
  then
    echo "here"
    LOCATION_ID=$(echo "$split_line" | sed -n -e 3p)
    LOCATION_UUID=$UUID
    PARENT=$ROOT_UUID
    ID=$LOCATION_ID
    CONFIG_ENTRY="$USERNAME,$UUID,$NAME,$TYPE,$PARENT,$LOCATION_ID"
  elif [ "$TYPE" = "enfuse_panel" ]
  then
    PANEL_ID=$(echo "$split_line" | sed -n -e 3p)
    PARENT=$LOCATION_UUID
    PANEL_UUID=$UUID
    ID=$PANEL_ID
    CONFIG_ENTRY="$USERNAME,$UUID,$NAME,$TYPE,$PARENT,$LOCATION_ID,$PANEL_ID"
  elif [ "$TYPE" = "enfuse_branch" ]
  then
    BRANCH_ID=$(echo "$split_line" | sed -n -e 3p)
    PARENT=$PANEL_UUID
    ID=$BRANCH_ID
    CONFIG_ENTRY="$USERNAME,$UUID,$NAME,$TYPE,$PARENT,$LOCATION_ID,$PANEL_ID,$BRANCH_ID"
  fi

 #RESULT=$(../../../tools/cmd-line/c/bin/mio_node_create $USER_FLAG -event $UUID -access open -title "\"$NAME\"")
  #echo "../../../tools/cmd-line/c/bin/mio_node_create $USER_FLAG -event $UUID -access open -title "\"$NAME\"""

  RESUlT=$(../bin/meta_tool $USER_FLAG -n "$NAME" -id $UUID -t $PARENT -type  $TYPE -a 1 -acm open)
  echo "../bin/meta_tool $USER_FLAG -n "$NAME" -id $UUID -t $PARENT -type  $TYPE -a 1 -acm open"

  #RESULT=$(../../../tools/cmd-line/c/bin/mio_reference_child_add $USER_FLAG -parent $PARENT -child $UUID -add_ref_child)
  #echo "../../../tools/cmd-line/c/bin/mio_reference_child_add $USER_FLAG -parent $PARENT -child $UUID -add_ref_child"


  echo $CONFIG_ENTRY >> $EVENT_PATH
done <<< "$CONFIG"

exit 1
