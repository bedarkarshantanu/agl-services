// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <map>
#include <vector>
#include <stdint.h>

typedef std::tuple<double, double> Waypoint;

/**
 *  @brief Genivi API call.
 */
class GeniviRequest
{
public:
	~GeniviRequest();

	std::map< int32_t, double > NavicoreGetPosition( const std::vector< int32_t >& valuesToReturn );
	std::vector< uint32_t >	 NavicoreGetAllRoutes();
	uint32_t					NavicoreCreateRoute( const uint32_t& sessionHandle );
	void						NavicorePauseSimulation( const uint32_t& sessionHandle );
	void						NavicoreSetSimulationMode( const uint32_t& sessionHandle, const bool& activate );
	void						NavicoreCancelRouteCalculation( const uint32_t& sessionHandle, const uint32_t& routeHandle );
	void						NavicoreSetWaypoints( const uint32_t& sessionHandle, const uint32_t& routeHandle,
										const bool& startFromCurrentPosition, const std::vector<Waypoint>& waypointsList );
	void						NavicoreCalculateRoute( const uint32_t& sessionHandle, const uint32_t& routeHandle );
	std::map<uint32_t, std::string> NavicoreGetAllSessions();

private:
	void* navicore_;

	void CreateDBusSession();
	bool CheckSession();
};

