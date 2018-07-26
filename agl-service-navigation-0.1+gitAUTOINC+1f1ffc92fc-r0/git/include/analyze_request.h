// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vector>

#include "genivi_request.h"

/**
 *  @brief Analyze requests from BinderClient and create arguments to pass to Genivi API.
 */
class AnalyzeRequest
{
public:
	bool CreateParamsGetPosition( const char* req_json_str, std::vector< int32_t >& Params );
	bool CreateParamsCreateRoute( const char* req_json_str, uint32_t& sessionHdl );
	bool CreateParamsPauseSimulation( const char* req_json_str, uint32_t& sessionHdl );
	bool CreateParamsSetSimulationMode( const char* req_json_str, uint32_t& sessionHdl, bool& simuMode );
	bool CreateParamsCancelRouteCalculation( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl );
	bool CreateParamsSetWaypoints( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl,
											   bool& currentPos, std::vector<Waypoint>& waypointsList );
	bool CreateParamsCalculateRoute( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl );

private:
	bool JsonObjectGetSessionHdl( const char* req_json_str, uint32_t& sessionHdl);
	bool JsonObjectGetSessionHdlRouteHdl( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl);
};

