// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "libnavicore.hpp"

/**
*  @brief Class for generating Json request
*/
class JsonRequestGenerator
{ 
public:
	static std::string CreateRequestGetPosition(const std::vector< int32_t >& valuesToReturn);
	static std::string CreateRequestGetAllRoutes();
	static std::string CreateRequestCreateRoute(const uint32_t* sessionHandle);
	static std::string CreateRequestPauseSimulation(const uint32_t* sessionHandle);
	static std::string CreateRequestSetSimulationMode(const uint32_t* sessionHandle, const bool* activate);
	static std::string CreateRequestCancelRouteCalculation(const uint32_t* sessionHandle, const uint32_t* routeHandle);
	static std::string CreateRequestSetWaypoints(const uint32_t* sessionHandle, const uint32_t* routeHandle, 
						const bool* startFromCurrentPosition, const std::vector<naviapi::Waypoint>* waypointsList);
	static std::string CreateRequestCalculateroute(const uint32_t* sessionHandle, const uint32_t* routeHandle);
	static std::string CreateRequestGetAllSessions();
};

