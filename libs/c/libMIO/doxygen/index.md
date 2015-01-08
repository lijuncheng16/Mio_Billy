Mortar IO                        {#index}
=================

[TOC]

@section Introduction Introduction

This document describes how to use the XMPP based libMIO API. These tools enable devices to commmunicate device and sensor data via a XMPP server. This api relies on the libstrophe C XMPP library. For mor information on installation visit [Mortar IO Installation](install.md).

@subsection Publish-Subscribe Publish-Subscribe Systems

Publish-subscribe systems generally support one or many data generators, and none to many data consumers. Here publishing is the way in which data generators transmit their data. Each of these data packets is taged by an id for the stream the data is associated with. A broker server then distributes the data packet to the subscribers that register as listening for data from that channel. Subscribing is the process of registering to an id of a channel.

@subsection usecase Publish-Subscribe Use Cases

Below are a few use cases that will familiarize the reader with some of the typical use cases for publish subscribe systems.

@subsubsection infrastructure Building Infrastructure Sensing / Actuation


@subsection xmpp Extensible Messaging and Presence Protocol (XMPP)

[XMPP](http://xmpp.org/) is an XML based communication protocol great for messaging applications. Its original purpose was to enable messaging services such as GChat and Facebook Messanger, it was also originally named Jabber. Key to the XMPP infrastructure is the ability to add on protocols known as XMPP Extension Protocols (XEP). [XEP-0060](http://www.xmpp.org/extensions/xep-0060.html). 

@subsection ejabberd Ejabberd

The XMPP this code is run and tested with is [Ejabberd](http://www.ejabberd.im/). For more information on how to install and configure Ejabberd visit <http://www.process-one.net/en/ejabberd/docs/>.


@section schema Mortar IO Schema

@subsection xep MIO XEP

This will For more information on how to form the packets that libMIO is built on visit [Mortar IO XEP](http://dev.mortr.io/).

@subsection devices Devices

@subsubsection meta Device Metadata


@section registration Registration/Authentication

Current access control allows for two entities, administrators and users. It is possible to configure an XMPP server so that it supports other authentication methods as well.



@subsubsection reg Registration

@subsubsection auth Authentication

@section dev MIO Device

@subsection devinterface MIO Interface

@subsubsection devcreate Device Creation


@subsubsection devdel Deletion

@subsubsection devacl Access Control

Owner: Creator of an event Node. Owners are admins of event nodes they create, and can add/modify user relationships of their respective nodes

Publisher: Allowed to publish data to an event Node. Owners are automatically publishers.

Subscriber: Allowed to subscribe to data from an event node. Subscribers receive updates when the event node has data published to it.




@section metainfo Meta Information

@subsection metaover Overview

@subsection metamio MIO Interface

@subsubsection metaget Get Meta Information

@subsubsection metaset Set Meta Information

@section subscriptions Device Subscriptions

@subsection subover Overview

@subsection submio MIO Interface

@subsubsection sub Subscribe

@subsubsection unsub Unsubscribe



@section publication Device Publication

@subsection pubover Overview

@subsection pubmio MIO Interface

@subsubsection pub Publish
