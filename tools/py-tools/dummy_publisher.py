#!/usr/bin/env python

import sys
import logging
import getpass
import ssl
import xml.dom.minidom
from optparse import OptionParser

import sleekxmpp
from sleekxmpp.xmlstream import ET, tostring

import threading
import time
import datetime
import math

# Python versions before 3.0 do not use UTF-8 encoding
# by default. To ensure that Unicode is handled properly
# throughout SleekXMPP, we will set the default encoding
# ourselves to UTF-8.
if sys.version_info < (3, 0):
    reload(sys)
    sys.setdefaultencoding('utf8')
else:
    raw_input = input


class PubsubClient(sleekxmpp.ClientXMPP):

    def __init__(self, jid, password, server, 
                       node=None, itemid=None):
        super(PubsubClient, self).__init__(jid, password)

        self.register_plugin('xep_0030')
        self.register_plugin('xep_0059')
        self.register_plugin('xep_0060')

        self.actions = ['nodes', 'create', 'delete', 
                        'publish', 'get', 'retract',
                        'purge', 'subscribe', 'unsubscribe', 'subscriptions']

        self.node = node
        self.pubsub_server = server
        self.itemid = itemid

        self.add_event_handler('session_start', self.start, threaded=True)

    def start(self, event):
        self.get_roster()
        self.send_presence()

    def publish(self, data):
        #payload = ET.fromstring("<test xmlns='test'>%s</test>" % self.data)
        
        #--- Warning: publish will fail if data does not contain XML elements ---#
        payload = ET.fromstring(data)
        try:
            result = self['xep_0060'].publish(self.pubsub_server, self.node, id=self.itemid, payload=payload)
            id = result['pubsub']['publish']['item']['id']
            print('Published "%s" at item id: %s' % (data, id))
        except:
            logging.error('Could not publish to: %s' % self.node)



def publish_daemon(xmpp):
   while(1):
      for i in range(15):
         xmpp.publish('<data><transducerValue typedValue="%f" timestamp="%s" id="sine" name="sine" /></data>' 
                % (math.sin(2.0*math.pi*i/15.0), datetime.datetime.now().isoformat()))
         time.sleep(1)



if __name__ == '__main__':
    # Setup the command line arguments.
    optp = OptionParser()
    optp.version = '%%prog 0.1'
    optp.usage = "%%prog [options] " + \
                             'node'

    optp.add_option('-q','--quiet', help='set logging to ERROR',
                    action='store_const',
                    dest='loglevel',
                    const=logging.ERROR,
                    default=logging.ERROR)
    optp.add_option('-d','--debug', help='set logging to DEBUG',
                    action='store_const',
                    dest='loglevel',
                    const=logging.DEBUG,
                    default=logging.ERROR)
    optp.add_option('-v','--verbose', help='set logging to COMM',
                    action='store_const',
                    dest='loglevel',
                    const=5,
                    default=logging.ERROR)

    # JID and password options.
    optp.add_option("-j", "--jid", dest="jid",
                    help="JID to use")
    optp.add_option("-p", "--password", dest="password",
                    help="password to use")

    optp.add_option("-m", "--itemid", dest="itemid",
                    help="Item ID to use") 

    opts,args = optp.parse_args()

    # Setup logging.
    logging.basicConfig(level=opts.loglevel,
                        format='%(levelname)-8s %(message)s')

    if len(args) != 1:
        optp.print_help()
        exit()

    if opts.jid is None:
        opts.jid = raw_input("Username: ")
    if opts.password is None:
        opts.password = getpass.getpass("Password: ")

    if opts.itemid is None:
        opts.itemid = 'current'

    if len(args) == 1:
        args = (args[0], '', '', '')

    pubsub = 'pubsub.'+opts.jid.split('@')[1]
    
    # Setup the Pubsub client
    xmpp = PubsubClient(opts.jid, opts.password,
                        #server = 'pubsub.sensor.andrew.cmu.edu',
                        server = pubsub,
                        node=args[0],
                        itemid=opts.itemid)

    # If you are working with an OpenFire server, you may need
    # to adjust the SSL version used:
    xmpp.ssl_version = ssl.PROTOCOL_SSLv3

    # If you want to verify the SSL certificates offered by a server:
    # xmpp.ca_certs = "path/to/ca/cert"
    
    # Connect to the XMPP server and start processing XMPP stanzas.
    if xmpp.connect():
        
        thr = threading.Thread(target=publish_daemon, args=(xmpp,))
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
    else:
        print("Unable to connect.")
