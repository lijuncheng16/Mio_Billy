To build registration tools. First build libMIO and libstrophe, under mio/libs/c/. Then build the meta took by running make.

./meta_tool -n [device name] -type [device type] -id [node_id] -u [username] -p [password] -t [parent uuid in tree] -a [add reference to child 1 or 0] -acm [access control model]
device name - the name of the device, stored in device meta information.
type (optional) - The type of meta information to include in the device. Phillips Hue, Lutron Area Etc. If none of these, assumes it is a uuid and checks for the meta data of an event node. If that metadata is found, it copies it.
node id - The Event node id of the device.
username - username of device creater
password - password associated with device creator
paren uuid (optional) - the uuid of the reference node to add new event node id to.
add refference to child (optional) - if 1 adds reference from child event node (the created event node), to the parent event node
acm (optional) - access control model to use for the event node. Default open
