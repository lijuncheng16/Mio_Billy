#!/bin/bash

CONFIG_PATH=$1
NETWORK_PATH=$2

CONFIG=$(cat $CONFIG_PATH)
touch $NETWORK_PATH

echo '<?xml vrsion="1.0" encoding="UTF-8"?>' > $NETWORK_PATH
echo '<ENFUSE url="https://enfuseapi.inscopeapi.com" user="cmu/arowe">' >> $NETWORK_PATH
PANEL_PREV='false'
while read -r config_line
do
	split_line=$(echo "$config_line" | tr "," "\n")
	TYPE=$(echo "$split_line" | sed -n -e 4p)

	if [ "$TYPE" = "enfuse_branch" ]
	then
		BRANCH_ID=$(echo "$split_line" | sed -n -e 8p)
		UUID=$(echo "$split_line" | sed -n -e 2p)
		echo "<BRANCH node_id=\"$UUID\" branch_id=\"$BRANCH_ID\" />" >> $NETWORK_PATH
	elif [ "$TYPE" = "enfuse_panel" ]
	then
		LOCATION_ID=$(echo "$split_line" | sed -n -e 6p)
		PANEL_ID=$(echo "$split_line" | sed -n -e 7p)
		NAME=$(echo "$split_line" | sed -n -e 3p)
		if [ "$PANEL_PREV" = 'true' ]
		then
			echo '</ENFUSE_PANEL>' >> $NETWORK_PATH
		fi
		echo "<ENFUSE_PANEL type=\"Enfuse Panel\" location_id=\"$LOCATION_ID\" panel_id=\"$PANEL_ID\" name=\"$NAME\">" >> $NETWORK_PATH
		PANEL_PREV='true'
	fi
done <<< "$CONFIG"

echo '</ENFUSE_PANEL>' >> $NETWORK_PATH
echo '</ENFUSE>' >> $NETWORK_PATH

exit 0
