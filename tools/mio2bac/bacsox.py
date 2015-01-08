#!/usr/bin/env python
import sys
import logging
import getpass
from optparse import OptionParser
import commands
import time
import threading
import re
import pyasn1
import pyasn1_modules
import copy
import sleekxmpp
from sleekxmpp.xmlstream import ET, tostring
from sleekxmpp.xmlstream.matcher import StanzaPath
from sleekxmpp.xmlstream.handler import Callback

import json
import socket
import xml.parsers.expat
import threading
import ssl
# BACpypes imports
from bacpypes.debugging import Logging, ModuleLogger
from bacpypes.consolelogging import ConfigArgumentParser

from bacpypes.core import run

from bacpypes.primitivedata import Real
from bacpypes.app import LocalDeviceObject, BIPMultiDeviceSimpleApplication
from bacpypes.object import AnalogValueObject, Property, register_object_type
from bacpypes.errors import ExecutionError

_log = logging.getLogger(__name__)
_debug = 1
_log = ModuleLogger(globals())
# Python versions before 3.0 do not use UTF-8 encoding
# by default. To ensure that Unicode is handled properly
# throughout SleekXMPP, we will set the default encoding
# ourselves to UTF-8.
if sys.version_info < (3, 0):
    reload(sys)
    sys.setdefaultencoding('utf8')
else:
    raw_input = input

context = None
#-----------------------------------------
def getIPAddresses():
    from ctypes import Structure, windll, sizeof
    from ctypes import POINTER, byref
    from ctypes import c_ulong, c_uint, c_ubyte, c_char
    MAX_ADAPTER_DESCRIPTION_LENGTH = 128
    MAX_ADAPTER_NAME_LENGTH = 256
    MAX_ADAPTER_ADDRESS_LENGTH = 8
    class IP_ADDR_STRING(Structure):
        pass
    LP_IP_ADDR_STRING = POINTER(IP_ADDR_STRING)
    IP_ADDR_STRING._fields_ = [
                               ("next", LP_IP_ADDR_STRING),
                               ("ipAddress", c_char * 16),
                               ("ipMask", c_char * 16),
                               ("context", c_ulong)]
    class IP_ADAPTER_INFO (Structure):
        pass
    LP_IP_ADAPTER_INFO = POINTER(IP_ADAPTER_INFO)
    IP_ADAPTER_INFO._fields_ = [
                               ("next", LP_IP_ADAPTER_INFO),
                               ("comboIndex", c_ulong),
                               ("adapterName", c_char * (MAX_ADAPTER_NAME_LENGTH + 4)),
                               ("description", c_char * (MAX_ADAPTER_DESCRIPTION_LENGTH + 4)),
                               ("addressLength", c_uint),
                               ("address", c_ubyte * MAX_ADAPTER_ADDRESS_LENGTH),
                               ("index", c_ulong),
                               ("type", c_uint),
                               ("dhcpEnabled", c_uint),
                               ("currentIpAddress", LP_IP_ADDR_STRING),
                               ("ipAddressList", IP_ADDR_STRING),
                               ("gatewayList", IP_ADDR_STRING),
                               ("dhcpServer", IP_ADDR_STRING),
                               ("haveWins", c_uint),
                               ("primaryWinsServer", IP_ADDR_STRING),
                               ("secondaryWinsServer", IP_ADDR_STRING),
                               ("leaseObtained", c_ulong),
                               ("leaseExpires", c_ulong)]
    GetAdaptersInfo = windll.iphlpapi.GetAdaptersInfo
    GetAdaptersInfo.restype = c_ulong
    GetAdaptersInfo.argtypes = [LP_IP_ADAPTER_INFO, POINTER(c_ulong)]
    adapterList = (IP_ADAPTER_INFO * 10)()
    buflen = c_ulong(sizeof(adapterList))
    rc = GetAdaptersInfo(byref(adapterList[0]), byref(buflen))
    if rc == 0:
        for a in adapterList:
            adNode = a.ipAddressList
            while True:
                ipAddr = adNode.ipAddress
                if ipAddr:
                    yield ipAddr
                adNode = adNode.next
                if not adNode:
                    break
#-----------------------------------------
class BACnetServer:
    local_ip=""
    local_port = 0
    sox_user=""
    sox_pass=""
    seg_type = ""
    foreign_ttl = 0
    max_apdu_len = 0
    devices = {}
    rddepeater = None

class Device:
    event_node_id = ""
    device_type = ""
    bacnet_name = ""
    bacnet_id = 0
    vendor_id = 0
    transducers = {}

class Transducer:
    tran_name = ""
    bac_type = ""
    bac_name=""
    object_id = 0;
    reg_id = 0;
    value = 0;

#------------------------------------------
class BACnetRepeater(BIPMultiDeviceSimpleApplication,Logging):
    def __init__(self,localDevices,context):
        self.localAddress = context.local_ip
        BIPMultiDeviceSimpleApplication.__init__(self, localDevices, self.localAddress)
        #------------------------------------------
# expat

class BACParser:
    bac_context = BACnetServer()
    device = None
    def __init__(self,bac_file,dev_file,reg_file):

        self.performParse(bac_file,self.BacStartElementHandler, \
                self.GenEndElementHandler,self.GenCharacterDataHandler)
        self.performParse(reg_file,self.RegStartElementHandler,\
                self.GenEndElementHandler,self.GenCharacterDataHandler)
        self.performParse(dev_file,self.DevStartElementHandler, \
                self.GenEndElementHandler,self.GenCharacterDataHandler)

        localDevices = {}
        for dev_name in self.bac_context.devices :
            print dev_name
            dev = self.bac_context.devices[dev_name]
            bac_name = dev.bacnet_name
            localDevice = LocalDeviceObject(
                            objectIdentifier=('device',int(dev.bacnet_id)),
                            objectName=dev_name
                            , maxApduLengthAccepted=self.bac_context.max_apdu_len
                            , segmentationSupported = self.bac_context.seg_type
                            , vendorIdentifier=dev.vendor_id
                            )
            localDevices[dev.bacnet_id] = localDevice
        print list(localDevices.keys())
        self.bac_context.repeater = BACnetRepeater(localDevices,self.bac_context)
        i = 0
        localKeys = list(localDevices.keys())
        for dev_name in self.bac_context.devices.keys():
            device = self.bac_context.devices[dev_name]
            for transducer in device.transducers.keys():
                tran = device.transducers[transducer]
                obj = self.transducerToObject(i,tran)
                while i in localKeys:
                    i += 1
                tran.object_id = i
                i += 1
                if obj == None:
                    print '%s no object' % tran.tran_name
                    continue
                self.bac_context.repeater.add_object((device.bacnet_id),obj)

    def performParse(self,xml_path, StartElementHandler, EndElementHandler, CharacterHandler):
        parser = xml.parsers.expat.ParserCreate()
        parser.StartElementHandler = StartElementHandler
        parser.EndElementHandler = EndElementHandler
        parser.CharacterDataHandler =CharacterHandler
        xml_file = open(xml_path)
        parser.ParseFile(xml_file)
        xml_file.close()

    def transducerToObject(self,ind,trans):
        print trans.bac_type
        if trans.bac_type == "Analog Input":
            trans.obj_id = ind
            obj = AnalogValueObject(objectIdentifier = ('analogValue', ind),
                    presentValue=trans.value,
                    objectName=trans.bac_name)
        elif trans.bac_type == "Multi-State Input":
            trans.obj_id = ind
            obj = MultiStateValueObject(objectIdentifier=('analogValue',ind),
                    objectName=trans.bac_name,presentValue=trans.value)
        else:
            obj = None
        return obj

    def getContext(self):
        return self.bac_context

    def DevStartElementHandler(self,name,attrs):
        print name
        print attrs
        if name == 'DEVICES':
            return
        elif name == 'DEVICE':
            attr_keys = list(attrs.keys())
            if 'device' not in attr_keys:
                self.dev_type = None
            else:
                self.dev_type = attrs['device']
        elif name == 'OBJECT':
            if self.dev_type is None:
                return
            attr_keys = list(attrs.keys())
            if 'tran_name' not in attr_keys:
                return
            tran_name = attrs['tran_name']
            if 'bac_type' in attr_keys:
                bac_type = attrs['bac_type']
            else:
                return
            if 'bac_name' in attr_keys:
                bac_name = str(attrs['bac_name'])
            else:
                bac_name = str(tran_name)


            for dev_name in self.bac_context.devices.keys():
                device = self.bac_context.devices[dev_name]
                if device.device_type != self.dev_type:
                    continue
                tran_keys = list(device.transducers.keys())
                if tran_keys != None and tran_name in tran_keys:
                    self.device.transducers[tran_name].bac_type = bac_type
                    self.device.transducers[tran_name].bac_name = bac_name
                else:
                    self.device.transducers[tran_name] = Transducer()
                    self.device.transducers[tran_name].tran_name = tran_name
                    self.device.transducers[tran_name].bac_type = bac_type
                    self.device.transducers[tran_name].bac_name = bac_name
                return
        else:
            print 'Could not handle %s, %s' % (name,attrs)

    def GenEndElementHandler(self,name):
        return
    def GenCharacterDataHandler(self,data):
        return

    def RegStartElementHandler(self,name,attrs):
        print name
        print attrs
        if name == "network":
            return
        elif name == "regMap":
            attr_keys = list(attrs.keys())
            node_keys = list(self.bac_context.devices.keys())
            if 'node' in attr_keys:
                node_id = attrs['node']
                if node_id in node_keys:
                    self.device = self.bac_context.devices[node_id]
                else:
                    self.device = None
            else:
                self.device = None
        elif name == "transducer":
            if self.device is None:
                return

            attr_keys = list(attrs.keys())
            tran_keys = list(self.device.transducers.keys())
            if 'name' not in attr_keys:
                return
            tran_name = attrs['name']
            if 'id' not in attr_keys:
                regid = 0
            else:
                regid = int(attrs['id'])
            if tran_keys != None and tran_name in tran_keys:
                self.device.transducers[tran_name].reg_id = regid
            else:
                self.device.transducers[tran_name] = Transducer()
                self.device.transducers[tran_name].reg_id = regid
                self.device.transducers[tran_name].tran_name = tran_name
        else:
            print 'Could not handle %s, %s' % (names,attrs)

    def BacStartElementHandler(self,name,attrs):
        print name
        print attrs
        attr_keys = list(attrs.keys())
        if name == "BACNET":
            if "jid-username" in attr_keys:
                self.bac_context.sox_user = attrs["jid-username"]
            if "jid-password" in attr_keys:
                self.bac_context.sox_pass = attrs["jid-password"]
            if "local_port" in attr_keys:
                self.bac_context.local_port = int(attrs["local_port"])
            else:
                self.bac_context.local_port = 47808
            if "segmentation-type" in attr_keys:
                self.bac_context.seg_type = str(attrs["segmentation-type"])
            else:
                self.bac_context.seg_type = str("segmented-both")
            if "foreign_ttl" in attr_keys:
                self.bac_context.foreign_ttl = int(attrs["foreign_ttl"])
            else:
                self.bac_context.foreigh_ttl = 90

            if "local_ip" in attr_keys:
                self.bac_context.local_ip = str(attrs["local_ip"])
            else:
                self.bac_context.local_ip = str("127.0.0.1")
            if "max_apdu_len" in attr_keys:
                self.bac_context.max_apdu_len = int(attrs["max_apdu_len"])
            else:
                self.bac_context.max_apdu_len = 1024
        elif name == "device":
            if "event_node_id" not in attr_keys:
                return
            if "bacnet_id" not in attr_keys:
                return
            if "device_type" not in attr_keys:
                return
            node_id = attrs["event_node_id"]
            node = Device()
            node.event_node_id = ('device',node_id)
            node.device_type = str(attrs["device_type"])
            node.bacnet_id = int(attrs["bacnet_id"])
            if "bacnet_name" not in attr_keys:
                node.bacnet_name = node.device_type
            else:
                node.bacnet_name=str(attrs["bacnet_name"])
            if "vendor_id" in attr_keys:
                node.vendor_id=int(attrs["vendor_id"])

            self.bac_context.devices[node_id] = node
        else:
            print 'Could not handle %s, %s' % (names,attrs)




#-------------------------------------------
def handle_xml(self,device, nodeid, elem):
   print "handling xml"
   for subelem in elem.getchildren():
      if(re.findall('[^\s]+transducerValue',subelem.tag)):
         print "there is a transducer value"
         attr = {}
         for pair in subelem.items():
            attr[pair[0]] = pair[1]
         print '%s name' % attr['name']
         if ('name' in list(attr.keys())) and (attr['name'] in list(device.transducers.keys())):
             print 'tran_name %s' % attr['name']
             trans = device.transducers[attr['name']]
             print list(attr.keys())
             if 'typedValue' in list(attr.keys()):
                 trans.value = float(attr['typedValue'])
                 obj = context.repeater.get_object_id(trans.object_id)
                 value = obj.__setattr__('presentValue', float(trans.value))
                 print obj.ReadProperty('presentValue')
                 print obj.ReadProperty('objectName')

                 print trans.value
             else:
                 print "no Value"
                 continue
         else:
             continue
   print ''
   print ''

class PubsubEvents(sleekxmpp.ClientXMPP,threading.Thread):

    def __init__(self, jid, password,context):
        super(PubsubEvents, self).__init__(jid, password)
        threading.Thread.__init__(self)
        self.context = copy.copy(context)
        self.register_plugin('xep_0030')
        self.register_plugin('xep_0059')
        self.register_plugin('xep_0060')

        self.add_event_handler('session_start', self._start)

        # Some services may require configuration to allow
        # sending delete, configuration, or subscription  events.
        self.add_event_handler('pubsub_publish', self._publish)
        self.add_event_handler('pubsub_retract', self._retract)
        self.add_event_handler('pubsub_purge', self._purge)
        self.add_event_handler('pubsub_delete', self._delete)
        self.add_event_handler('pubsub_config', self._config)
        self.add_event_handler('pubsub_subscription', self._subscription)
        self.connect(use_tls=False)


    def _start(self, event):
        self.get_roster()
        self.send_presence()

    def _publish(self, msg):
        """Handle receiving a publish item event."""
        event_id = msg['pubsub_event']['items']['node']

        print list(self.context.devices.keys())
        print event_id
        if str(event_id) not in list(self.context.devices.keys()):
            return

        print('Publish event with item %s to %s:' % (
            msg['pubsub_event']['items']['item']['id'],
            msg['pubsub_event']['items']['node']))
        data = msg['pubsub_event']['items']['item']['payload']
        if data is not None:
            #print tostring(data)
            handle_xml(self,self.context.devices[event_id],event_id,data)
        else:
            print('No item content')

    def _retract(self, msg):
        """Handle receiving a retract item event."""
        print('Retracted item %s from %s' % (
            msg['pubsub_event']['items']['retract']['id'],
            msg['pubsub_event']['items']['node']))

    def _purge(self, msg):
        """Handle receiving a node purge event."""
        print('Purged all items from %s' % (
            msg['pubsub_event']['purge']['node']))

    def _delete(self, msg):
        """Handle receiving a node deletion event."""
        print('Deleted node %s' % (
           msg['pubsub_event']['delete']['node']))

    def _config(self, msg):
        """Handle receiving a node configuration event."""
        print('Configured node %s:' % (
            msg['pubsub_event']['configuration']['node']))
        print(msg['pubsub_event']['configuration']['form'])

    def _subscription(self, msg):
        """Handle receiving a node subscription event."""
        print('Subscription change for node %s:' % (
            msg['pubsub_event']['subscription']['node']))
        print(msg['pubsub_event']['subscription'])


    def run(self):
        self.process(block=True)


if __name__ == '__main__':
    # Setup the command line arguments.
    optp = OptionParser()

    # Output verbosity options.
    optp.add_option('-q', '--quiet', help='set logging to ERROR',
                    action='store_const', dest='loglevel',
                    const=logging.ERROR, default=logging.INFO)
    optp.add_option('-v', '--verbose', help='set logging to COMM',
                    action='store_const', dest='loglevel',
                    const=5, default=logging.INFO)

    optp.add_option("-d", "--directory", dest="config_path",action="store", type="string")

    optp, args = optp.parse_args()


    bacnet_path = '%s/bacnet.xml' % optp.config_path
    device_path = '%s/devices.xml' % optp.config_path
    regmap_path = '%s/regmap.xml' % optp.config_path
    print bacnet_path
    print device_path
    print regmap_path
    #bac_parser = BACParser(optp.bacnet_path,optp.device_path,optp.objmap_path,optp.regmap_path)
    bac_parser = BACParser(bacnet_path,device_path,regmap_path)
    context = bac_parser.getContext()

    print "initialized context"

    # Setup logging.
    logging.basicConfig(level=optp.loglevel,
                        format='%(levelname)-8s %(message)s')

    # Setup the PubsubEvents listener
    xmpp = PubsubEvents(context.sox_user, context.sox_pass, context)
    xmpp.start()
    # Connect to the XMPP server and start processing XMPP stanzas.
    print "running bacnet"
    run()
#--------------------------------------------

