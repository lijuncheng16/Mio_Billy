#!/usr/bin/env python

# A variant of the SleekXMPP pubsub client
# modified for use with MIO. Requires the
# SleekXMPP python library to be installed.
#
# Max Buevich

import sys
import logging
import getpass
import ssl
import xml.dom.minidom
from optparse import OptionParser

import sleekxmpp
from sleekxmpp.xmlstream import ET, tostring


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
                       node=None, action='list', data='', itemid=None):
        super(PubsubClient, self).__init__(jid, password)

        self.register_plugin('xep_0030')
        self.register_plugin('xep_0059')
        self.register_plugin('xep_0060')

        self.actions = ['nodes', 'create', 'delete', 
                        'publish', 'get', 'retract',
                        'purge', 'subscribe', 'unsubscribe',
                        'subs']

        self.action = action
        self.node = node
        self.data = data
        self.itemid = itemid
        self.pubsub_server = server

        self.add_event_handler('session_start', self.start, threaded=True)

    def start(self, event):
        self.get_roster()
        self.send_presence()

        try:
            getattr(self, self.action)()
        except:
            logging.error('Could not execute: %s' % self.action)
        self.disconnect()

    def nodes(self):
        try:
            result = self['xep_0060'].get_nodes(self.pubsub_server, self.node)
            for item in result['disco_items']['items']:
                print('  - %s' % str(item))
        except:
            logging.error('Could not retrieve node list.')

    def create(self):
        try:
            self['xep_0060'].create_node(self.pubsub_server, self.node)
        except:
            logging.error('Could not create node: %s' % self.node)

    def delete(self):
        try:
            self['xep_0060'].delete_node(self.pubsub_server, self.node)
            print('Deleted node: %s' % self.node)
        except:
            logging.error('Could not delete node: %s' % self.node)

    def publish(self):
        #payload = ET.fromstring("<test xmlns='test'>%s</test>" % self.data)
        
        #--- Warning: publish will fail if data does not contain XML elements ---#
        payload = ET.fromstring(self.data)
        try:
            result = self['xep_0060'].publish(self.pubsub_server, self.node, id=self.itemid, payload=payload)
            id = result['pubsub']['publish']['item']['id']
            print('Published at item id: %s' % id)
        except:
            logging.error('Could not publish to: %s' % self.node)

    def get(self):
        try:
            result = self['xep_0060'].get_item(self.pubsub_server, self.node, self.data)
            #for item in result['pubsub']['items']['substanzas']:
            #    print('Retrieved item %s: %s' % (item['id'], tostring(item['payload'])))
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not retrieve item %s from node %s' % (self.data, self.node))

    def retract(self):
        try:
            result = self['xep_0060'].retract(self.pubsub_server, self.node, self.data)
            print('Retracted item %s from node %s' % (self.data, self.node))
        except:
            logging.error('Could not retract item %s from node %s' % (self.data, self.node))

    def purge(self):
        try:
            result = self['xep_0060'].purge(self.pubsub_server, self.node)
            print('Purged all items from node %s' % self.node)
        except:
            logging.error('Could not purge items from node %s' % self.node)

    def subscribe(self):
        try:
            result = self['xep_0060'].subscribe(self.pubsub_server, self.node, subscribee=self.boundjid.bare, bare=True)
            print('Subscribed %s to node %s' % (self.boundjid.bare, self.node))
        except:
            logging.error('Could not subscribe %s to node %s' % (self.boundjid.bare, self.node))

    def unsubscribe(self):
        try:
            result = self['xep_0060'].unsubscribe(self.pubsub_server, self.node, subscribee=self.boundjid.bare, bare=True)
            print('Unsubscribed %s from node %s' % (self.boundjid.bare, self.node))
        except:
            logging.error('Could not unsubscribe %s from node %s' % (self.boundjid.bare, self.node))

    def subs(self):
        try:
            result = self['xep_0060'].get_subscriptions(self.pubsub_server)
            #for sub in result['pubsub']['subscriptions']:
            #   print sub['node']
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not retrieve subscriptions.')


###--- Additional Functions, call-able from CLI ---###
    
    def affils(self):
        try:
            result = self['xep_0060'].get_affiliations(self.pubsub_server, self.node)
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not get affiliations')

    def node_affils(self):
        try:
            result = self['xep_0060'].get_node_affiliations(self.pubsub_server, self.node)
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not get affiliations from node %s' % (self.node))

    def node_subs(self):
        try:
            result = self['xep_0060'].get_node_subscriptions(self.pubsub_server, self.node)
            print result
        except:
            logging.error('Could not get subscriptions from node %s' % (self.node))

    def node_config(self):
        try:
            result = self['xep_0060'].get_node_config(self.pubsub_server, self.node)
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not get config from node %s' % (self.node))

    def item_ids(self):
        try:
            result = self['xep_0060'].get_item_ids(self.pubsub_server, self.node)
            print xml.dom.minidom.parseString(str(result)).toprettyxml(indent="   ")
        except:
            logging.error('Could not get Item IDs from node %s' % (self.node))


    # Other existing functions not exposed: map_node_event, get_subscription_options,
    # set_subscription_options, set_node_config, get_items, modify_affiliations,
    # modify_subscriptions


if __name__ == '__main__':
    # Setup the command line arguments.
    optp = OptionParser()
    optp.version = '%%prog 0.1'
    optp.usage = "%prog [options] " + \
                             'nodes|create|delete|purge|subscribe|unsubscribe|subs|publish|retract|get' + \
                             ' [<node> <data>]'

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

    if len(args) < 1:
        optp.print_help()
        exit()


    #--- Uncomment and modify for convenience ---#
    #--------------------------------------------#
    #opts.jid = 'user@sensor.andrew.cmu.edu'
    #opts.password = 'password'
    #--------------------------------------------#
    #--------------------------------------------#
    

    if opts.jid is None:
        opts.jid = raw_input("Username: ")
    if opts.password is None:
        opts.password = getpass.getpass("Password: ")

    if opts.itemid is None:
        opts.itemid = 'current'

    if len(args) == 1:
        args = (args[0], '', '', '')
    elif len(args) == 2:
        args = (args[0], args[1], '', '')
    elif len(args) == 3:
        args = (args[0], args[1], args[2], '')

    pubsub = 'pubsub.'+opts.jid.split('@')[1]

    # Setup the Pubsub client
    xmpp = PubsubClient(opts.jid, opts.password,
                        #server=args[0],
                        #server = 'pubsub.sensor.andrew.cmu.edu',
                        server = pubsub,
                        node=args[1],
                        action=args[0],
                        data=args[2],
                        itemid=opts.itemid)

    # If you are working with an OpenFire server, you may need
    # to adjust the SSL version used:
    xmpp.ssl_version = ssl.PROTOCOL_SSLv3

    # If you want to verify the SSL certificates offered by a server:
    # xmpp.ca_certs = "path/to/ca/cert"
    
    # Connect to the XMPP server and start processing XMPP stanzas.
    if xmpp.connect():
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
