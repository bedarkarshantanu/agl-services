// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include <json-c/json.h>
#include <traces.h>
#include "JsonRequestGenerator.h"

/**
 *  @brief Generate request for navicore_getposition
 *  @param valuesToReturn Key information you want to obtain
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestGetPosition(const std::vector< int32_t >& valuesToReturn)
{
	std::vector< int32_t >::const_iterator itr;

	struct json_object* request_json = json_object_new_object();
	struct json_object* json_array = json_object_new_array();

	for (itr = valuesToReturn.begin(); itr != valuesToReturn.end(); itr++)
	{
		json_object_array_add(json_array, json_object_new_int(*itr));
	}

	json_object_object_add(request_json, "valuesToReturn", json_array);
	TRACE_DEBUG("CreateRequestGetPosition request_json:\n%s\n", json_object_to_json_string(request_json));
    
	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_getallroutes
 *  @return json strin
 */
std::string JsonRequestGenerator::CreateRequestGetAllRoutes()
{
	// Request is empty and OK
	struct json_object* request_json = json_object_new_object();
	TRACE_DEBUG("CreateRequestGetAllRoutes request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_createroute
 *  @param sessionHandle session handle
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestCreateRoute(const uint32_t* sessionHandle)
{
	struct json_object* request_json = json_object_new_object();
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));
	TRACE_DEBUG("CreateRequestCreateRoute request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_pausesimulation
 *  @param sessionHandle session handle
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestPauseSimulation(const uint32_t* sessionHandle)
{
	struct json_object* request_json = json_object_new_object();
	// sessionHandle
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));
	TRACE_DEBUG("CreateRequestPauseSimulation request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_pausesimulation
 *  @param sessionHandle session handle
 *  @param active Simulation state
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestSetSimulationMode(const uint32_t* sessionHandle, const bool* activate)
{
	struct json_object* request_json = json_object_new_object();

	// "sessionHandle"
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));

	// "simulationMode"
	json_object_object_add(request_json, "simulationMode", json_object_new_boolean(*activate));
	TRACE_DEBUG("CreateRequestSetSimulationMode request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_pausesimulation
 *  @param sessionHandle session handle
 *  @param routeHandle route handle
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestCancelRouteCalculation(const uint32_t* sessionHandle, const uint32_t* routeHandle)
{
	struct json_object* request_json = json_object_new_object();
    
	// "sessionHandle"
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));

	// "route"
	json_object_object_add(request_json, "route", json_object_new_int(*routeHandle));
	TRACE_DEBUG("CreateRequestCancelRouteCalculation request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_setwaypoints
 *  @param sessionHandle session handle
 *  @param routeHandle route handle
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestSetWaypoints(const uint32_t* sessionHandle, const uint32_t* routeHandle, 
		const bool* startFromCurrentPosition, const std::vector<naviapi::Waypoint>* waypointsList)
{
	naviapi::Waypoint destWp;
    
	struct json_object* request_json = json_object_new_object();
	struct json_object* json_array = json_object_new_array();

	// "sessionHandle"
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));

	// "route"
	json_object_object_add(request_json, "route", json_object_new_int(*routeHandle));

	// "startFromCurrentPosition"
	json_object_object_add(request_json, "startFromCurrentPosition", json_object_new_boolean(*startFromCurrentPosition));

	// "latitude", "longitude"
	std::vector<naviapi::Waypoint>::const_iterator it;
	for (it = waypointsList->begin(); it != waypointsList->end(); ++it)
	{
		struct json_object* destpoint = json_object_new_object();

		double latitude = std::get<0>(*it);
		json_object_object_add(destpoint, "latitude", json_object_new_double(latitude));

		double longitude = std::get<1>(*it);
		json_object_object_add(destpoint, "longitude", json_object_new_double(longitude));
    	
   		json_object_array_add(json_array, destpoint);
	}

	json_object_object_add(request_json, "", json_array);
	TRACE_DEBUG("CreateRequestSetWaypoints request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_calculateroute
 *  @param sessionHandle session handle
 *  @param routeHandle route handle
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestCalculateroute(const uint32_t* sessionHandle, const uint32_t* routeHandle)
{
	struct json_object* request_json = json_object_new_object();
	// "sessionHandle"
	json_object_object_add(request_json, "sessionHandle", json_object_new_int(*sessionHandle));

	// "route"
	json_object_object_add(request_json, "route", json_object_new_int(*routeHandle));
	TRACE_DEBUG("CreateRequestCalculateroute request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

/**
 *  @brief Generate request for navicore_getallsessions
 *  @return json string
 */
std::string JsonRequestGenerator::CreateRequestGetAllSessions()
{
	// Request is empty and OK
	struct json_object* request_json = json_object_new_object();
	TRACE_DEBUG("CreateRequestGetAllSessions request_json:\n%s\n", json_object_to_json_string(request_json));

	return std::string( json_object_to_json_string( request_json ) );
}

