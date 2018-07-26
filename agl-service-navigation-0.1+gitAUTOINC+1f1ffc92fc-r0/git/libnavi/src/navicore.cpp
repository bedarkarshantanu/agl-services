// Copyright 2017 AISIN AW CO.,LTD

#include "libnavicore.hpp"
#include "BinderClient.h"

static BinderClient mBinderClient;

naviapi::Navicore::Navicore()
{
}

naviapi::Navicore::~Navicore()
{
}

bool naviapi::Navicore::connect(int argc, char *argv[], NavicoreListener* listener)
{
	this->mListener = listener;

	if (argc != 3)
	{
		printf("Error: argc != 3 : argc = %d\n", argc);
		return false;
	}

	char url[1024];
	sprintf(url, "ws://localhost:%d/api?token=%s", atoi(argv[1]), argv[2]);

	return mBinderClient.ConnectServer(url, this->mListener);
}

void naviapi::Navicore::disconnect()
{
	// TODO
}

void naviapi::Navicore::getAllSessions()
{
	mBinderClient.NavicoreGetAllSessions();
}

void naviapi::Navicore::getPosition(std::vector<int32_t> params)
{
	mBinderClient.NavicoreGetPosition(params);
}

void naviapi::Navicore::getAllRoutes()
{
	mBinderClient.NavicoreGetAllRoutes();
}

void naviapi::Navicore::createRoute(uint32_t session)
{
	mBinderClient.NavicoreCreateRoute(session);
}

void naviapi::Navicore::pauseSimulation(uint32_t session)
{
	mBinderClient.NavicorePauseSimulation(session);
}

void naviapi::Navicore::setSimulationMode(uint32_t session, bool activate)
{
	mBinderClient.NavicoreSetSimulationMode(session, activate);
}

void naviapi::Navicore::cancelRouteCalculation(uint32_t session, uint32_t routeHandle)
{
	mBinderClient.NavicoreCancelRouteCalculation(session, routeHandle);
}

void naviapi::Navicore::setWaypoints(uint32_t session, uint32_t routeHandle, bool flag, std::vector<Waypoint> waypoints)
{
	mBinderClient.NavicoreSetWaypoints(session, routeHandle, flag, waypoints);
}

void naviapi::Navicore::calculateRoute(uint32_t session, uint32_t routeHandle)
{
	mBinderClient.NavicoreCalculateRoute(session, routeHandle);
}

