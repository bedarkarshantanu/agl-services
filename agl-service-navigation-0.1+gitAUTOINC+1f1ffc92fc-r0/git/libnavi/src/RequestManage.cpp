// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <systemd/sd-event.h>
#include <json-c/json.h>
#include <traces.h>
#include "RequestManage.h"

/**
 *  @brief constructor
 */
RequestManage::RequestManage() : listener(nullptr)
{
	// Callback setting
	this->wsj1_itf.on_hangup    = RequestManage::OnHangupStatic;
	this->wsj1_itf.on_call      = RequestManage::OnCallStatic;
	this->wsj1_itf.on_event     = RequestManage::OnEventStatic;

	pthread_cond_init(&this->cond, nullptr);
	pthread_mutex_init(&this->mutex, nullptr);
}

/**
 *  @brief Destructor
 */
RequestManage::~RequestManage()
{
}

void* RequestManage::BinderThread(void* param)
{
	RequestManage* instance = (RequestManage*) param;
	sd_event *loop;

	int rc = sd_event_default(&loop);
	if (rc < 0) {
		TRACE_ERROR("connection to default event loop failed: %s\n", strerror(-rc));
		return nullptr;
	}

	instance->wsj1 = afb_ws_client_connect_wsj1(loop, instance->requestURL->c_str(), &instance->wsj1_itf, nullptr);
	if (instance->wsj1 == nullptr)
	{
		TRACE_ERROR("connection to %s failed: %m\n", api_url);
		return nullptr;
	}

	// Signal
	pthread_mutex_unlock(&instance->mutex);
	pthread_cond_signal(&instance->cond);

	while (1)
	{
		sd_event_run(loop, 1000 * 1000); // 1sec
	}

	return nullptr;
}

/**
 *  @brief  Connect with a service
 *  @param  URL
 *  @return Success or failure of connection
 */
bool RequestManage::Connect(const char* api_url, RequestManageListener* listener)
{
	this->listener = listener;
	this->requestURL = new std::string(api_url);

	pthread_t thread_id;
	pthread_create(&thread_id, nullptr, RequestManage::BinderThread, (void*)this);

	// Wait until response comes
	pthread_mutex_lock(&this->mutex);
	pthread_cond_wait(&this->cond, &this->mutex);
	pthread_mutex_unlock(&this->mutex);

	if (this->wsj1 == nullptr)
	{
		return false;
	}

	return true;
}

/**
 *  @brief  Connection status check with service
 *  @return Connection status
 */
bool RequestManage::IsConnect(){
	return (this->wsj1 != NULL);
}

/**
 *  @brief  Call Binder's API
 *  @param  api      api
 *  @param  verb     method
 *  @param  req_json Json style request
 *  @return Success or failure of processing
 */
bool RequestManage::CallBinderAPI(const char* api, const char* verb, const char* req_json)
{
	// Send request
	int rc = afb_wsj1_call_s(this->wsj1, api, verb, req_json, RequestManage::OnReplyStatic, this);
	if (rc < 0)
	{
		TRACE_ERROR("calling %s/%s(%s) failed: %m\n", api, verb, req_json);
		return false;
	}

	return true;
}

/**
 *  @brief  Set session handle
 *  @param session Session handle
 */
void RequestManage::SetSessionHandle( uint32_t session )
{
	this->sessionHandle = session;
}

/**
 *  @brief  Get session handle
 *  @return Session handle
 */
uint32_t RequestManage::GetSessionHandle()
{
	return this->sessionHandle;
}

/**
 *  @brief  Set route handle
 *  @param route Route handle
 */
void RequestManage::SetRouteHandle( uint32_t route )
{
	this->routeHandle = route;
}

/**
 *  @brief  Get route handle
 *  @return Route handle
 */
uint32_t RequestManage::GetRouteHandle()
{
	return this->routeHandle;
}

void RequestManage::OnReply(struct afb_wsj1_msg *msg)
{
	struct json_object * json = afb_wsj1_msg_object_j(msg);

	this->listener->OnReply(json);
}

void RequestManage::OnHangup(struct afb_wsj1 *wsj1)
{
}

void RequestManage::OnCallStatic(const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
}

void RequestManage::OnEventStatic(const char *event, struct afb_wsj1_msg *msg)
{
}


/**
 *  @brief  Answer callback from service
 */
void RequestManage::OnReplyStatic(void *closure, struct afb_wsj1_msg *msg)
{
	RequestManage* instance = (RequestManage *)closure;
	instance->OnReply(msg);
}

/**
 *  @brief  Service hang notification
 */
void RequestManage::OnHangupStatic(void *closure, struct afb_wsj1 *wsj1)
{
	printf("DEBUG:%s:%d (%p,%p)\n", __func__, __LINE__, closure, wsj1);
	fflush(stdout);
}

void RequestManage::OnCallStatic(void *closure, const char *api, const char *verb, struct afb_wsj1_msg *msg)
{
	printf("DEBUG:%s:%d (%p,%s,%s,%p)\n", __func__, __LINE__, closure, api, verb, msg);
	fflush(stdout);
}

void RequestManage::OnEventStatic(void *closure, const char *event, struct afb_wsj1_msg *msg)
{
	printf("DEBUG:%s:%d (%p,%s,%p)\n", __func__, __LINE__, closure, event, msg);
	fflush(stdout);
}

