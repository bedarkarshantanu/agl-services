// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <stdint.h>

namespace naviapi {

static const uint32_t NAVICORE_TIMESTAMP = 0x0010;
static const uint32_t NAVICORE_LATITUDE = 0x00a0;
static const uint32_t NAVICORE_LONGITUDE = 0x00a1;
static const uint32_t NAVICORE_HEADING = 0x00a3;
static const uint32_t NAVICORE_SPEED = 0x00a4;
static const uint32_t NAVICORE_SIMULATION_MODE = 0x00e3;

typedef union
{
	bool _bool;
	int32_t _int32_t;
	uint32_t _uint32_t;
	double _double;
} variant;

typedef std::tuple<double, double> Waypoint;

class NavicoreListener
{
public:
	NavicoreListener();
	virtual ~NavicoreListener();

	virtual void getAllSessions_reply(const std::map< uint32_t, std::string >& allSessions);
	virtual void getPosition_reply(std::map< int32_t, variant > position);
	virtual void getAllRoutes_reply(std::vector< uint32_t > allRoutes);
	virtual void createRoute_reply(uint32_t routeHandle);
}; // class NavicoreListener

class Navicore
{
private:
	NavicoreListener* mListener;

public:
	Navicore();
	virtual ~Navicore();

	bool connect(int argc, char *argv[], NavicoreListener* listener);
	void disconnect();

	void getAllSessions();
	void getPosition(std::vector<int32_t> params);
	void getAllRoutes();
	void createRoute(uint32_t session);

	void pauseSimulation(uint32_t session);
	void setSimulationMode(uint32_t session, bool activate);
	void cancelRouteCalculation(uint32_t session, uint32_t routeHandle);
	void setWaypoints(uint32_t session, uint32_t routeHandle, bool flag, std::vector<Waypoint>);
	void calculateRoute(uint32_t session, uint32_t routeHandle);

}; // class Navicore

}; // namespace naviapi

