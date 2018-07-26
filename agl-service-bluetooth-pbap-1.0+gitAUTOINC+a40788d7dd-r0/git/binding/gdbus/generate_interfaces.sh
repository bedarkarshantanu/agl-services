#!/bin/sh

CG=gdbus-codegen
API=api

$CG \
	--interface-prefix Obex.Client1. \
	--generate-c-code obex_client1_interface \
	--c-generate-object-manager \
	$API/client1.xml

$CG \
	--interface-prefix Obex.Session1. \
	--generate-c-code obex_session1_interface \
	$API/session1.xml

$CG \
	--interface-prefix Obex.PhonebookAccess1. \
	--generate-c-code obex_phonebookaccess1_interface \
	$API/phonebookaccess1.xml

$CG \
	--interface-prefix Obex.Transfer1. \
	--generate-c-code obex_transfer1_interface \
	$API/transfer1.xml

$CG \
	--interface-prefix org.freedesktop.DBus.Properties. \
	--generate-c-code freedesktop_dbus_properties_interface \
	$API/org.freedesktop.DBus.Properties.xml
