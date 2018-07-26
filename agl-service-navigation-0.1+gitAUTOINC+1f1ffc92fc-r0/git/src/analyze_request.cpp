// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include "genivi/genivi-navicore-constants.h"
#include "analyze_request.h"
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <string>


/**
 *  @brief	Create arguments to pass to Genivi API GetPosition.
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	Params An array of key information you want to obtain
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsGetPosition( const char* req_json_str, std::vector< int32_t >& Params)
{
	struct json_object *req_json = json_tokener_parse(req_json_str);
	struct json_object* jValuesToReturn = NULL;
	if( json_object_object_get_ex(req_json, "valuesToReturn", &jValuesToReturn) )
	{
		if( json_object_is_type(jValuesToReturn, json_type_array) )
		{
			for (int i = 0; i < json_object_array_length(jValuesToReturn); ++i) 
			{
				struct json_object* j_elem = json_object_array_get_idx(jValuesToReturn, i);

				// JSON type acquisition
				if( json_object_is_type(j_elem, json_type_int ) )
				{
					int32_t req_key = json_object_get_int (j_elem);

					// no supported.
					if ((NAVICORE_TIMESTAMP == req_key) || (NAVICORE_SPEED == req_key))
					{
						continue;
					}
					Params.push_back(req_key);
				}
				else
				{
					fprintf(stdout, "key is not integer type.\n");
					return false;
				}
			}
		}
		else
		{
			fprintf(stdout, "request is not array type.\n");
			return false;
		}
	}
	else
	{
		fprintf(stdout, "key valuesToReturn not found.\n");
		return false;
	}

	return true;
}


/**
 *  @brief	Create arguments to pass to Genivi API CreateRoute
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsCreateRoute( const char* req_json_str, uint32_t& sessionHdl )
{
	// Get sessionHandle information
	return JsonObjectGetSessionHdl(req_json_str, sessionHdl);
}


/**
 *  @brief	Create arguments to pass to Genivi API PauseSimulation
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsPauseSimulation( const char* req_json_str, uint32_t& sessionHdl )
{
	// Get sessionHandle information
	return JsonObjectGetSessionHdl(req_json_str, sessionHdl);
}


/**
 *  @brief	Create arguments to pass to Genivi API CreateRoute
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @param[out]	simuMode Simulation mode
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsSetSimulationMode( const char* req_json_str, uint32_t& sessionHdl, bool& simuMode )
{
	bool		ret = false;
	struct json_object *sess  = NULL;
	struct json_object *simu  = NULL;

	struct json_object *req_json = json_tokener_parse(req_json_str);
	if ((json_object_object_get_ex(req_json, "sessionHandle", &sess)) &&
		(json_object_object_get_ex(req_json, "simulationMode", &simu)))
	{
		if (json_object_is_type(sess, json_type_int) &&
			json_object_is_type(simu, json_type_boolean))
		{
			sessionHdl = json_object_get_int(sess);
			simuMode = json_object_get_int(simu);
			ret = true;
		}
		else
		{
			fprintf(stdout, "key is invalid type.\n");
		}
	}
	else
	{
		fprintf(stdout, "key sessionHandle or simulationMode not found.\n");
	}

	return ret;
}


/**
 *  @brief	Create arguments to pass to Genivi API CancelRouteCalculation
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @param[out]	routeHdl Route handle
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsCancelRouteCalculation( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl )
{
	// Get sessionHandle, RouteHandle
	return JsonObjectGetSessionHdlRouteHdl(req_json_str, sessionHdl, routeHdl);
}


/**
 *  @brief	Create arguments to pass to Genivi API SetWaypoints
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @param[out]	routeHdl Route handle
 *  @param[out]	currentPos Whether or not to draw a route from the position of the vehicle
 *  @param[out]	waypointsList Destination coordinates
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsSetWaypoints( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl,
											   bool& currentPos, std::vector<Waypoint>& waypointsList )
{
	bool		ret = false;
	struct json_object *sess  = NULL;
	struct json_object *rou  = NULL;
	struct json_object *current  = NULL;
	struct json_object *wpl  = NULL;

	struct json_object *req_json = json_tokener_parse(req_json_str);
	if ((json_object_object_get_ex(req_json, "sessionHandle", &sess)) &&
		(json_object_object_get_ex(req_json, "route", &rou))		  &&
		(json_object_object_get_ex(req_json, "startFromCurrentPosition", &current)) &&
		(json_object_object_get_ex(req_json, "", &wpl)))
	{
		if (json_object_is_type(sess, json_type_int) &&
			json_object_is_type(rou, json_type_int)  && 
			json_object_is_type(current, json_type_boolean) &&
			json_object_is_type(wpl, json_type_array))
		{
			sessionHdl = json_object_get_int(sess);
			routeHdl = json_object_get_int(rou);
			currentPos = json_object_get_boolean(current);

			// Get latitude, longitude
			for (int i = 0; i < json_object_array_length(wpl); ++i)
			{
				struct json_object *array = json_object_array_get_idx(wpl, i);
				struct json_object *lati  = NULL;
				struct json_object *longi = NULL;

				if (json_object_object_get_ex(array, "latitude", &lati) && 
					json_object_object_get_ex(array, "longitude", &longi)) {

					double latitude  = json_object_get_double(lati);
					double longitude = json_object_get_double(longi);
					Waypoint destWp(latitude, longitude);
					waypointsList.push_back(destWp);
					ret = true;
				}
				else
				{
					fprintf(stdout, "key latitude or longitude not found.\n");
				}
		   }
		}
		else
		{
			fprintf(stdout, "key is invalid type.\n");
		}
	}
	else
	{
		fprintf(stdout, "key valuesToReturn not found.\n");
	}

	return ret;
}


/**
 *  @brief	Create arguments to pass to Genivi API CalculateRoute
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	sessionHdl Session handle
 *  @param[out]	routeHdl Route handle
 *  @return	Success or failure of processing
 */
bool AnalyzeRequest::CreateParamsCalculateRoute( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl )
{
	// Get sessionHandle, RouteHandle
	return JsonObjectGetSessionHdlRouteHdl(req_json_str, sessionHdl, routeHdl);
}


/**
 *  @brief	Get session handle and route handle information from JSON
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	Session handle value
 *  @return	Success or failure of processing
 */

bool AnalyzeRequest::JsonObjectGetSessionHdl( const char* req_json_str, uint32_t& sessionHdl)
{
	bool		ret = false;
	struct json_object *sess  = NULL;

	struct json_object *req_json = json_tokener_parse(req_json_str);
	if (json_object_object_get_ex(req_json, "sessionHandle", &sess))
	{
		if (json_object_is_type(sess, json_type_int))
		{
			sessionHdl = json_object_get_int(sess);
			ret = true;
		}
		else
		{
			fprintf(stdout, "key is not integer type.\n");
		}
	}
	else
	{
		fprintf(stdout, "key sessionHandle not found.\n");
	}

	return ret;
}


/**
 *  @brief	Get session handle and route handle information from JSON
 *  @param[in]	req_json_str JSON request from BinderClient
 *  @param[out]	Session handle value
 *  @param[out]	Route handle value
 *  @return	Success or failure of processing
 */

bool AnalyzeRequest::JsonObjectGetSessionHdlRouteHdl( const char* req_json_str, uint32_t& sessionHdl, uint32_t& routeHdl)
{
	bool		ret = false;
	struct json_object *sess  = NULL;
	struct json_object *rou  = NULL;

	struct json_object *req_json = json_tokener_parse(req_json_str);
	if ((json_object_object_get_ex(req_json, "sessionHandle", &sess)) &&
		(json_object_object_get_ex(req_json, "route", &rou)))
	{
		if (json_object_is_type(sess, json_type_int) &&
			json_object_is_type(rou, json_type_int))
		{
			sessionHdl = json_object_get_int(sess);
			routeHdl = json_object_get_int(rou);
			ret = true;
		}
		else
		{
			fprintf(stdout, "key is not integer type.\n");
		}
	}
	else
	{
		fprintf(stdout, "key sessionHandle or route not found.\n");
	}

	return ret;
}
