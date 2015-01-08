#!/usr/bin/env python

import sys
import logging
import getpass
from optparse import OptionParser

import sleekxmpp
from sleekxmpp.xmlstream import ET, tostring
from sleekxmpp.xmlstream.matcher import StanzaPath
from sleekxmpp.xmlstream.handler import Callback

import threading
import commands
import json
import time
import re
import iso8601
import socket
import os

# Python versions before 3.0 do not use UTF-8 encoding
# by default. To ensure that Unicode is handled properly
# throughout SleekXMPP, we will set the default encoding
# ourselves to UTF-8.
if sys.version_info < (3, 0):
    reload(sys)
    sys.setdefaultencoding('utf8')
else:
    raw_input = input

#--------------------------------------------


bin_dir = 'bt_datastore/'
store = 'store.kvs'
temp_json = '/tmp/xmpp.store.pid'+str(os.getpid())+'.json'

import_json = {}
lock = 0
ip = ''
nodes_seen = []


def add_to_json(dev, channel, timestamp, val):
   global lock
   global import_json
   
   dev = dev.replace(' ','-')
   channel = channel.replace(' ','-')

   key = dev + '.' + channel
   lock.acquire()   
   if key not in import_json:
      obj = {}
      obj['channel_names'] = [channel]
      obj['method'] = 'post'
      obj['action'] = 'upload'
      obj['dev_nickname'] = dev
      obj['user_id']  = '1'
      obj['controller'] = 'logrecs'
      obj['data'] = list()
      import_json[key] = obj
   try:
      import_json[key]['data'].append([timestamp,float(val)])
   except ValueError:
      print "VALUE ERROR!"
   lock.release()


def handle_xml(self, nodeid, itemid, elem):
   #print '-------------'
   
   if(nodeid not in nodes_seen):
      nodes_seen.append(nodeid)
      r = self['xep_0060'].get_item(self.pubsub, nodeid, 'storage') 
      storage = ''
      if(r['pubsub']['items']['item']['payload']):
         for a in r['pubsub']['items']['item']['payload']:
            storage += tostring(a)
      if(ip not in storage):
         storage += "<address link='http://"+ip+":4720' />"
         storage = "<addresses>"+storage+"</addresses>"
         #print "publish:", storage
         try:
            self['xep_0060'].publish(self.pubsub, nodeid, id='storage', payload=ET.fromstring(storage))
         except sleekxmpp.exceptions.IqError:
            print "IqError: Publish to storage item failed. You are probably not the owner of this node..."

   for subelem in elem.getchildren():
      if(re.findall('[^\s]+transducerValue',subelem.tag)):
         attr = {}
         for pair in subelem.items():
            attr[pair[0]] = pair[1]
         #print attr
         dt = iso8601.parse_date(attr['timestamp'])
         t = time.mktime(dt.timetuple()) + (dt.microsecond/1e6)
         #print attr['timestamp']
         #print("%.9f" % t)
         if(attr['id'] != 'none'):
            add_to_json(nodeid, attr['id'], t, attr['typedValue'])
            pass
      if(re.findall('[^\s]+nodeRegister',subelem.tag)):
         attr = {}
         for pair in subelem.items():
            attr[pair[0]] = pair[1]
         #print attr['id']
         if(attr['id']):
            nodes = []
            result = self['xep_0060'].get_subscriptions(self.pubsub)
            for sub in result['pubsub']['subscriptions']:
               nodes.append(sub['node'])
            if(attr['id'] not in nodes):
               self['xep_0060'].subscribe(self.pubsub, attr['id'], subscribee=self.boundjid.bare, bare=True)
               print('Subscribed %s to node %s' % (self.boundjid.bare, attr['id']))


def datastore_daemon():
   global lock
   global import_json
   
   while(1):
      lock.acquire()
      import_json_temp = import_json
      import_json = {}
      lock.release()

      for key in import_json_temp:
         obj = import_json_temp[key]
         jsonobj = json.dumps(obj)
         f = open(temp_json,'w')
         f.write(jsonobj)
         f.close()
         
         print commands.getoutput(bin_dir + 'import ' + store + ' 1 ' 
               + obj['dev_nickname'] + ' ' + temp_json)
         print 'STORED', key, '/', len(obj['data']), 'points'
      import_json_temp = {}
      
      for i in range(10):
         print "...storing in", 10*(10-i), "seconds..."
         time.sleep(10)


class PubsubEvents(sleekxmpp.ClientXMPP):

    def __init__(self, jid, password, pubsub=''):
        super(PubsubEvents, self).__init__(jid, password)

        self.register_plugin('xep_0030')
        self.register_plugin('xep_0059')
        self.register_plugin('xep_0060')

        self.pubsub = pubsub

        self.add_event_handler('session_start', self.start)

        # Some services may require configuration to allow
        # sending delete, configuration, or subscription  events.
        self.add_event_handler('pubsub_publish', self._publish)
        self.add_event_handler('pubsub_retract', self._retract)
        self.add_event_handler('pubsub_purge', self._purge)
        self.add_event_handler('pubsub_delete', self._delete)
        self.add_event_handler('pubsub_config', self._config)
        self.add_event_handler('pubsub_subscription', self._subscription)

        # Want to use nicer, more specific pubsub event names?
        # self['xep_0060'].map_node_event('node_name', 'event_prefix')
        # self.add_event_handler('event_prefix_publish', handler)
        # self.add_event_handler('event_prefix_retract', handler)
        # self.add_event_handler('event_prefix_purge', handler)
        # self.add_event_handler('event_prefix_delete', handler)

    def start(self, event):
        self.get_roster()
        self.send_presence()

    def _publish(self, msg):
        """Handle receiving a publish item event."""
        print('Published item %s to %s:' % (
            msg['pubsub_event']['items']['item']['id'],
            msg['pubsub_event']['items']['node']))
        data = msg['pubsub_event']['items']['item']['payload']
        if data is not None:
            #print tostring(data)
            handle_xml(self,msg['pubsub_event']['items']['node'],msg['pubsub_event']['items']['item']['id'],data)
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


if __name__ == '__main__':
    # Setup the command line arguments.
    optp = OptionParser()
    optp.usage = "%prog [options] -j jid@host -p password"

    # Output verbosity options.
    optp.add_option('-q', '--quiet', help='set logging to ERROR',
                    action='store_const', dest='loglevel',
                    const=logging.ERROR, default=logging.INFO)
    optp.add_option('-d', '--debug', help='set logging to DEBUG',
                    action='store_const', dest='loglevel',
                    const=logging.DEBUG, default=logging.INFO)
    optp.add_option('-v', '--verbose', help='set logging to COMM',
                    action='store_const', dest='loglevel',
                    const=5, default=logging.INFO)

    # JID and password options.
    optp.add_option("-j", "--jid", dest="jid",
                    help="JID to use")
    optp.add_option("-p", "--password", dest="password",
                    help="password to use")

    opts, args = optp.parse_args()

    # Setup logging.
    logging.basicConfig(level=opts.loglevel,
                        format='%(levelname)-8s %(message)s')

    if opts.jid is None:
        #opts.jid = 'user@server.org'
        optp.print_help()
        exit()
    if opts.password is None:
        #opts.password = 'password'
        optp.print_help()
        exit()

    logging.info("Run this in conjunction with the pubsub_client.py " + \
                 "example to watch events happen as you give commands.")

    pubsub = 'pubsub.'+opts.jid.split('@')[1]
    # Setup the PubsubEvents listener
    xmpp = PubsubEvents(opts.jid, opts.password, pubsub=pubsub)

    # If you are working with an OpenFire server, you may need
    # to adjust the SSL version used:
    # xmpp.ssl_version = ssl.PROTOCOL_SSLv3

    # If you want to verify the SSL certificates offered by a server:
    # xmpp.ca_certs = "path/to/ca/cert"

    # Connect to the XMPP server and start processing XMPP stanzas.
    if xmpp.connect():
        
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(('google.com', 80))
        ip = s.getsockname()[0]
        print "IP:", ip
        s.close()
        
        lock = threading.Lock()
        thr = threading.Thread(target=datastore_daemon)
        thr.daemon = True
        thr.start()

        # If you do not have the dnspython library installed, you will need
        # to manually specify the name of the server if it does not match
        # the one in the JID. For example, to use Google Talk you would
        # need to use:
        #
        # if xmpp.connect(('talk.google.com', 5222)):
        #     ...
        xmpp.process(block=True)
        print("Done")
    else:
        print("Unable to connect.")
