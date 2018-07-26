// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <json-c/json.h>

class RequestManageListener
{
public:
	RequestManageListener() {
	}
	virtual ~RequestManageListener() {
	}

	virtual void OnReply(struct json_object *reply) = 0;
};

