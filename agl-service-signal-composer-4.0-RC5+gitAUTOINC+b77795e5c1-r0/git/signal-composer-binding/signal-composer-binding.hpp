#pragma once

extern "C"
{
	#define AFB_BINDING_VERSION 2
	#include <afb/afb-binding.h>
}

void onEvent(const char *event, struct json_object *object);
int loadConf();
int execConf();
