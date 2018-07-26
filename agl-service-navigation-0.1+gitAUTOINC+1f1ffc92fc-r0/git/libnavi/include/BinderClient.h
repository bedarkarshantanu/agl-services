// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <map>
#include <tuple>
#include <vector>
#include <string>

#include "libnavicore.hpp"

#include "RequestManageListener.h"
#include "RequestManage.h"

#define API_NAME		"naviapi"

/**
 *  @brief API name
 */
#define VERB_GETPOSITION	"navicore_getposition"
#define VERB_GETALLROUTES	"navicore_getallroutes"
#define VERB_CREATEROUTE	"navicore_createroute"
#define VERB_PAUSESIMULATION	"navicore_pausesimulation"
#define VERB_SETSIMULATIONMODE	"navicore_setsimulationmode"
#define VERB_CANCELROUTECALCULATION	"navicore_cancelroutecalculation"
#define VERB_SETWAYPOINTS	"navicore_setwaypoints"
#define VERB_CALCULATEROUTE	"navicore_calculateroute"
#define VERB_GETALLSESSIONS	"navicore_getallsessions"

/**
 *  @brief Binder client class
 */
class BinderClient : public RequestManageListener
{
public:
	BinderClient();
	~BinderClient();

	bool ConnectServer(std::string url , naviapi::NavicoreListener* listener);
	void NavicoreGetPosition(const std::vector< int32_t >& valuesToReturn);
	void NavicoreGetAllRoutes();
	void NavicoreCreateRoute(const uint32_t& sessionHandle);
	void NavicorePauseSimulation(const uint32_t& sessionHandle);
	void NavicoreSetSimulationMode(const uint32_t& sessionHandle, const bool& activate);
	void NavicoreCancelRouteCalculation(const uint32_t& sessionHandle, const uint32_t& routeHandle);
	void NavicoreSetWaypoints(const uint32_t& sessionHandle, const uint32_t& routeHandle, const bool& startFromCurrentPosition, const std::vector<naviapi::Waypoint>& waypointsList);
	void NavicoreCalculateRoute(const uint32_t& sessionHandle, const uint32_t& routeHandle);
	void NavicoreGetAllSessions();

private:
	void OnReply(struct json_object *reply);

private:
	naviapi::NavicoreListener* navicoreListener;
	RequestManage* requestMng;
};

