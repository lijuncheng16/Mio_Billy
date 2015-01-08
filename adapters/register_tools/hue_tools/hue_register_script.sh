#!/bin/bash
USER=$1
PASSWORD=$2
PARENT=$3


USER_FLAG="-u $USER -p $PASSWORD"


#Config format
BIN=../../drivers/libhue/bin


GATEWAY_RESULT=$($BIN/discover_local_bridges)
echo "$GATEWAY_RESULT"
GATEWAY_IND=$(echo "$GATEWAY_RESULT" | grep -n ID)
echo "$GATEWAY_IND"

while read -r gateway_line; 
do
	LINE_NUMBER=$(echo $gateway_line | grep -oh '^[0-9]\+') 
	LINE_END="$(($LINE_NUMBER + 2))" 
        
        echo "$gateway_line"
	echo "$LINE_NUMBER $LINE_END"
	GATEWAY_BLOCK=$(echo "$GATEWAY_RESULT" | sed -n -e "$LINE_NUMBER,$LINE_END"p) 


	BRIDGE_ID=$(echo "$GATEWAY_BLOCK" | grep "ID" | sed -e 's/ID //g')
	BRIDGE_IP=$(echo "$GATEWAY_BLOCK" | grep "IP" | sed -e 's/IP //g')

        #echo "$BIN/register_user -b $BRIDGE_IP -d "Mortar.io"
	USER_RESULT=$($BIN/register_user -b $BRIDGE_IP -d "Mortar.io" )

        echo "USER $USER_RESULT"
	echo "BRIDGE ID $BRIDGE_ID"
        echo "BRIDGE IP $BRIDGE_IP"

	BRIDGE_USER=$(echo "$USER_RESULT" | grep 'Username' | sed -e 's/Username //g')
	
	echo "$BRIDGE_USER"
	GATEWAY_UUID=$(uuid)
    
    echo "../bin/meta_tool $USER_FLAG -n "Hue Gateway" -id $GATEWAY_UUID -t $PARENT -type hue_bridge -a 1 -acm open"
	../bin/meta_tool $USER_FLAG -n "Hue Gateway" -id $GATEWAY_UUID -t $PARENT -type hue_bridge -a 1 -acm open

	../../../tools/cmd-line/c/bin/mio_meta_property_add $USER_FLAG -event $GATEWAY_UUID -name "Bridge User" -value "$BRIDGE_USER"

	../../../tools/cmd-line/c/bin/mio_meta_property_add $USER_FLAG -event $GATEWAY_UUID -name "IP Address" -value "$BRIDGE_IP"
	
	../../../tools/cmd-line/c/bin/mio_meta_property_add $USER_FLAG -event $GATEWAY_UUID -name "Bridge ID" -value "$BRIDGE_ID"

	BULB_RESULT=$($BIN/get_all_lights -u "$BRIDGE_USER" -b "$BRIDGE_IP")
	BULB_IND=$(echo "$BULB_RESULT" | grep -n "LIGHT:" )

 	echo "BUlb result $BULB_RESULT"

	while read -r bulb_index; 
	do
		LINE_NUMBER=$(echo $bulb_index | grep -oh '^[0-9]\+') 
		LINE_END=$(($LINE_NUMBER + 2))
		BULB_BLOCK=$(echo "$BULB_RESULT" | sed -n -e "$LINE_NUMBER,$LINE_END"p) 

		BULB_ID=$(echo "$BULB_BLOCK" | grep 'BULB ID: ' | sed -e 's/BULB ID: //g')
		BULB_NAME=$(echo "$BULB_BLOCK" | grep 'BULB NAME: ' | sed -e 's/BULB NAME: //g')

		BULB_UUID=$(uuid)
        echo "../bin/meta_tool $USER_FLAG -n "$BULB_NAME" -id $BULB_UUID -t $GATEWAY_UUID -type hue_bulb -a 1 -acm open"
		../bin/meta_tool $USER_FLAG -n "$BULB_NAME" -id $BULB_UUID -t $GATEWAY_UUID -type hue_bulb -a 1 -acm open
		../../../tools/cmd-line/c/bin/mio_meta_property_add $USER_FLAG -event $BULB_UUID -name "Bulb ID" -value "$BULB_ID" 
		../../../tools/cmd-line/c/bin/mio_meta_property_add $USER_FLAG -event $BULB_UUID -name "Bulb Name" -value "$BULB_NAME" 


	done <<< "$BULB_IND"
done <<< "$GATEWAY_IND"

exit 0
