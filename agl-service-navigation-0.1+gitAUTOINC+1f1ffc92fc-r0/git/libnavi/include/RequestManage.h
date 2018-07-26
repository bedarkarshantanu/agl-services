// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <stdint.h>
#include <string>
#include <pthread.h>

extern "C" {
	#include <afb/afb-wsj1.h>
	#include <afb/afb-ws-client.h>
}

#include "RequestManageListener.h"

/**
*  @brief Class for request
*/
class RequestManage
{
public:
	pthread_cond_t cond;
	pthread_mutex_t mutex;

	struct afb_wsj1* wsj1;
	std::string* requestURL;
	struct afb_wsj1_itf wsj1_itf;

private:
	RequestManageListener* listener;
	int request_cnt;
	uint32_t sessionHandle;
	uint32_t routeHandle;

	// Function called from thread
	static void* BinderThread(void* param);

	// Callback function
	void OnReply(struct afb_wsj1_msg *msg);
	void OnHangup(struct afb_wsj1 *wsj1);
	void OnCallStatic(const char *api, const char *verb, struct afb_wsj1_msg *msg);
	void OnEventStatic(const char *event, struct afb_wsj1_msg *msg);

	static void OnReplyStatic(void *closure, struct afb_wsj1_msg *msg);
	static void OnHangupStatic(void *closure, struct afb_wsj1 *wsj1);
	static void OnCallStatic(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg);
	static void OnEventStatic(void *closure, const char *event, struct afb_wsj1_msg *msg);
  
// ==================================================================================================
// public
// ==================================================================================================
public:
	RequestManage();
	~RequestManage();
    
	bool Connect(const char* api_url, RequestManageListener* listener);
	bool IsConnect();
	bool CallBinderAPI(const char *api, const char *verb, const char *object);
	void SetSessionHandle(uint32_t session);
	uint32_t GetSessionHandle();
	void SetRouteHandle(uint32_t route);
	uint32_t GetRouteHandle();
};

