#!/usr/bin/env python

# Mortar.io HTTP Adapter
# author: Max Buevich

import sys
import logging
import getpass
from optparse import OptionParser

import sleekxmpp
from sleekxmpp.xmlstream import ET, tostring
from sleekxmpp.xmlstream.matcher import StanzaPath
from sleekxmpp.xmlstream.handler import Callback

import xml.dom.minidom
import threading
import socket

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

httpd_port = 8000



def httpd(xmpp):
   s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
   s.bind(('', httpd_port))

   while(1):
      s.listen(1)
      connection, addr = s.accept()
      print "client (ip,port) are", addr

      data = connection.recv(1024)
      req = data.split('\n')[0]
      print "client request:", req
      print ''
      arr = req.split()[1].split('/')
      
      node = arr[1]
      if(len(arr)>2):
         item = arr[2]
      else:
         item = ''

      try:
         reply = "HTTP/1.0 200 OK\n\n"
         xml_data = xmpp['xep_0060'].get_item(xmpp.pubsub, node, item)
         if(xml_data is not None):
            reply += str(xml.dom.minidom.parseString(str(xml_data)).toprettyxml(indent="  "))
            #print reply
      except:
         reply = "HTTP/1.0 404 Not Found\n\n"
      
      connection.sendall(reply)
      connection.close()


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
        #self.add_event_handler('pubsub_publish', self._publish)
        #self.add_event_handler('pubsub_retract', self._retract)
        #self.add_event_handler('pubsub_purge', self._purge)
        #self.add_event_handler('pubsub_delete', self._delete)
        #self.add_event_handler('pubsub_config', self._config)
        #self.add_event_handler('pubsub_subscription', self._subscription)

        # Want to use nicer, more specific pubsub event names?
        # self['xep_0060'].map_node_event('node_name', 'event_prefix')
        # self.add_event_handler('event_prefix_publish', handler)
        # self.add_event_handler('event_prefix_retract', handler)
        # self.add_event_handler('event_prefix_purge', handler)
        # self.add_event_handler('event_prefix_delete', handler)

    def start(self, event):
        self.get_roster()
        self.send_presence()



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
        
        lock = threading.Lock()
        thr = threading.Thread(target=httpd, args=(xmpp,))
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
