#!/bin/sh

CG=gdbus-codegen
API=api

$CG \
	--interface-prefix org.lightmediascanner. \
	--generate-c-code lightmediascanner_interface \
	$API/org.lightmediascanner.xml
