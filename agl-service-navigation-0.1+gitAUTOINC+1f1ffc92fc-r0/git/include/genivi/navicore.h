// Copyright 2017 AISIN AW CO.,LTD

#ifndef NAVICORE_H
#define NAVICORE_H

#include <dbus-c++-1/dbus-c++/dbus.h>
#include "genivi-navigationcore-proxy.h"
#include <stdio.h>

class Navicore :
  public org::genivi::navigationcore::Session_proxy,
  public org::genivi::navigationcore::Routing_proxy,
  public org::genivi::navigationcore::MapMatchedPosition_proxy,
  public org::genivi::navigationcore::Guidance_proxy,
  public DBus::IntrospectableProxy,
  public DBus::ObjectProxy
{
public:
	Navicore(DBus::Connection &connection, const char *path, const char *name)
		: DBus::ObjectProxy(connection, path, name)
	{
	};

	// Session
	void SessionDeleted(const uint32_t& sessionHandle)
	{
		printf("NavicoreSession - Session %d deleted\n",sessionHandle);
	};

	// Routing
	void RouteDeleted(const uint32_t& routeHandle)
	{
		// TODO
	};

	void RouteCalculationCancelled(const uint32_t& routeHandle)
	{
		// TODO
	};

	void RouteCalculationSuccessful(const uint32_t& routeHandle, const std::map< int32_t, int32_t >& unfullfilledPreferences)
	{
		// TODO
	};

	void RouteCalculationFailed(const uint32_t& routeHandle, const int32_t& errorCode, const std::map< int32_t, int32_t >& unfullfilledPreferences)
	{
		// TODO
	};

	void RouteCalculationProgressUpdate(const uint32_t& routeHandle, const int32_t& status, const uint8_t& percentage)
	{
		// TODO
	};

	void AlternativeRoutesAvailable(const std::vector< uint32_t >& routeHandlesList)
	{
		// TODO
	};

	// MapMatchedPosition
	void SimulationStatusChanged(const int32_t& simulationStatus)
	{
		// TODO
	};

	void SimulationSpeedChanged(const uint8_t& speedFactor)
	{
		// TODO
	};

	void PositionUpdate(const std::vector< int32_t >& changedValues)
	{
		// TODO
	};

	void AddressUpdate(const std::vector< int32_t >& changedValues)
	{
		// TODO
	};

	void PositionOnSegmentUpdate(const std::vector< int32_t >& changedValues)
	{
		// TODO
	};

	void StatusUpdate(const std::vector< int32_t >& changedValues)
	{
		// TODO
	};

	void OffRoadPositionChanged(const uint32_t& distance, const int32_t& direction)
	{
		// TODO
	};

	// Guidance
	void VehicleLeftTheRoadNetwork()
	{
		// TODO
	};

	void GuidanceStatusChanged(const int32_t& guidanceStatus, const uint32_t& routeHandle)
	{
		// TODO
	};

	void WaypointReached(const bool& isDestination)
	{
		// TODO
	};

	void ManeuverChanged(const int32_t& maneuver)
	{
		// TODO
	};

	void PositionOnRouteChanged(const uint32_t& offsetOnRoute)
	{
		// TODO
	};

	void VehicleLeftTheRoute()
	{
		// TODO
	};

	void PositionToRouteChanged(const uint32_t& distance, const int32_t& direction)
	{
		// TODO
	};

	void ActiveRouteChanged(const int32_t& changeCause)
	{
		// TODO
	};
	
};

#endif
