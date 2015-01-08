#!/bin/bash

USERNAME=$1
CONFIG_PATH=$2

echo -n "Password:"
read -s PASSWORD
echo

#Config format
BIN=../../drivers/libfuse/bin


RESULT="$($BIN/enfuse_get_locations -u $USERNAME -p $PASSWORD)"

LOCATION_ID=$(echo "$RESULT" | grep "id [0-9]\+" | grep -oh "[0-9]\+")
LOCATION_NAME=$(echo "$RESULT" | grep "name *" | sed -e 's/name //g')
LOCATION_ADDRESS=$(echo "$RESULT"| grep "Address *" | sed -e 's/Address //g')
LOCATION_LATITUDE=$(echo "$RESULT" | grep "Latitude *" | sed -e 's/Latitude //g')
LOCATION_LONGITUDE=$(echo "$RESULT" | grep "Longitude *" | sed -e 's/Longitude //g')

touch "$CONFIG_PATH"
echo "enfuse_location,$LOCATION_NAME,$LOCATION_ID,$LOCATION_ADDRESS,$LOCATION_LATITUDE,$LOCATION_LONGITUDE" >> $CONFIG_PATH

RESULT=$($BIN/enfuse_get_panels -u $USERNAME -p $PASSWORD -l $LOCATION_ID)
PANEL_IND=$(echo "$RESULT" | grep -n "Panel ID")


while read -r line; 
do
  # parse through the Information
  LINE_NUMBER=$(echo $line | grep -oh '^[0-9]\+')
  LINE_END=$(($LINE_NUMBER + 3))

  PANEL_BLOCK=$(echo "$RESULT" | sed -n -e "$LINE_NUMBER,$LINE_END"p)

  PANEL_ID=$(echo $line | grep -oh '[0-9]\+$')
  PANEL_NAME=$(echo "$PANEL_BLOCK" | grep 'Name *' | sed -e 's/Name //g')
  PANEL_LOCATION=$LOCATION_ID

  echo "enfuse_panel,$PANEL_NAME,$PANEL_ID,$PANEL_LOCATION" >> $CONFIG_PATH
  BRANCH_RESULT=$($BIN/enfuse_get_branches -u $USERNAME -p $PASSWORD -l $LOCATION_ID -pid $PANEL_ID)
  BRANCH_INDS=$(echo "$BRANCH_RESULT" | grep -n "Branch ID")
  
  while read -r branch_line;
  do
    LINE_NUMBER=$(echo $branch_line | grep -oh '^[0-9]\+')
    LINE_END="$(($LINE_NUMBER + 3))"

    BRANCH_BLOCK=$(echo "$BRANCH_RESULT" | sed -n -e "$LINE_NUMBER,$LINE_END"p)

    BRANCH_ID=$(echo $branch_line | grep -oh '[0-9]\+$')
    BRANCH_NAME=$(echo "$BRANCH_BLOCK" | grep Name | sed -e 's/Name //g')

    echo "enfuse_branch,$BRANCH_NAME,$BRANCH_ID,$PANEL_ID" >> $CONFIG_PATH

  done <<< "$BRANCH_INDS"
done <<< "$PANEL_IND"

exit 0
