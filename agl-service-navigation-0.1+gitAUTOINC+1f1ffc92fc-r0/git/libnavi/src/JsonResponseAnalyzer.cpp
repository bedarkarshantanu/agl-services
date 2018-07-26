// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include <traces.h>

#include "JsonResponseAnalyzer.h"

/**
 *  @brief Response analysis of navicore_getallroutes
 *  @param res_json JSON string of response
 *  @return Map information for the key sent in the request
 */
std::map< int32_t, naviapi::variant > JsonResponseAnalyzer::AnalyzeResponseGetPosition( std::string& res_json )
{
	std::map< int32_t, naviapi::variant > ret;

	TRACE_DEBUG("AnalyzeResponseGetPosition json_obj:\n%s\n", json_object_to_json_string(json_obj));

	// convert to Json Object
	struct json_object *json_obj = json_tokener_parse( res_json.c_str() );

	// Check key
	struct json_object *json_map_ary = NULL;
	if( json_object_object_get_ex(json_obj, "response", &json_map_ary) )
	{
		// Check if the response is array information
		if( json_object_is_type(json_map_ary, json_type_array) )
		{
			for (int i = 0; i < json_object_array_length(json_map_ary); ++i) 
			{
				struct json_object* j_elem = json_object_array_get_idx(json_map_ary, i);
                
				if( json_object_is_type( j_elem, json_type_object) )
				{
					// Check key
					struct json_object* key = NULL;
					struct json_object* value = NULL;
					if( json_object_object_get_ex(j_elem, "key", &key) 
					    &&  json_object_object_get_ex(j_elem, "value", &value) )
					{
						if( json_object_is_type(key, json_type_int) )
						{
							uint32_t req_key = (uint32_t)json_object_get_int(key);

							switch( req_key )
							{
							case naviapi::NAVICORE_LATITUDE:
								ret[req_key]._double = json_object_get_double(value);
								break;

							case naviapi::NAVICORE_LONGITUDE:
								ret[req_key]._double = json_object_get_double(value);
								break;

							case naviapi::NAVICORE_TIMESTAMP:
								ret[req_key]._uint32_t = (uint32_t)json_object_get_int(value);
								break;

							case naviapi::NAVICORE_HEADING:
								ret[req_key]._uint32_t = (uint32_t)json_object_get_int(value);
								break;

							case naviapi::NAVICORE_SPEED:
								ret[req_key]._int32_t = json_object_get_int(value);
								break;

							case naviapi::NAVICORE_SIMULATION_MODE:
								ret[req_key]._bool = json_object_get_boolean(value);
								break;

							default:
								TRACE_WARN("unknown key type.\n");
								break;
							}
						}
					}
				}
				else
				{
					TRACE_WARN("element type is not object.\n");
					break;
				}
			}
		}
		else
		{
			TRACE_WARN("response type is not array.\n");
		}
	}

	json_object_put(json_obj);
	return ret;
}

/**
 *  @brief Response analysis of navicore_getallroutes
 *  @param res_json JSON string of response
 *  @return Route handle array
 */
std::vector< uint32_t > JsonResponseAnalyzer::AnalyzeResponseGetAllRoutes( std::string& res_json )
{
	std::vector< uint32_t > routeList;

	TRACE_DEBUG("AnalyzeResponseGetAllRoutes json_obj:\n%s\n", json_object_to_json_string(json_obj));

	// convert to Json Object
	struct json_object *json_obj = json_tokener_parse( res_json.c_str() );

	// Check key
	struct json_object *json_route_ary = NULL;
	if( json_object_object_get_ex(json_obj, "response", &json_route_ary) )
	{
		// Check if the response is array information
		if( json_object_is_type(json_route_ary, json_type_array) )
		{
			for (int i = 0; i < json_object_array_length(json_route_ary); ++i) 
			{
				struct json_object* j_elem = json_object_array_get_idx(json_route_ary, i);

				// Check that it is an element
				struct json_object *json_route_obj = NULL;
				if( json_object_object_get_ex(j_elem, "route", &json_route_obj) )
				{
					// Check if it is an int value
					if( json_object_is_type(json_route_obj, json_type_int) )
					{
						uint32_t routeHandle = (uint32_t)json_object_get_int(json_route_obj);
						routeList.push_back( routeHandle );
					}
					else
					{
						TRACE_WARN("route value is not integer.\n");
						break;
					}
				}
				else
				{
					TRACE_WARN("key route is not found.\n");
					break;
				}
			}
		}
		else
		{
			TRACE_WARN("response type is not array.\n");
		}
	}

	json_object_put(json_obj);
	return routeList;
}

/**
 *  @brief Response analysis of navicore_createroute
 *  @param res_json JSON string of response
 *  @return Route handle
 */
uint32_t JsonResponseAnalyzer::AnalyzeResponseCreateRoute( std::string& res_json )
{
	uint32_t routeHandle = 0;

	TRACE_DEBUG("AnalyzeResponseCreateRoute json_obj:\n%s\n", json_object_to_json_string(json_obj));

	// convert to Json Object
	struct json_object *json_obj = json_tokener_parse( res_json.c_str() );

	// Check key
	struct json_object *json_root_obj = NULL;
	struct json_object *json_route_obj = NULL;
	if( json_object_object_get_ex(json_obj, "response", &json_root_obj)
	    &&  json_object_object_get_ex(json_root_obj, "route", &json_route_obj) )
	{
		// Check if the response is array information
		if( json_object_is_type(json_route_obj, json_type_int) )
		{
			// Get route handle
			routeHandle = (uint32_t)json_object_get_int(json_route_obj);
		}
		else
		{
			TRACE_WARN("response type is not integer.\n");
		}
	}

	json_object_put(json_obj);
	return routeHandle;
}

/**
 *  @brief Response analysis of navicore_getallsessions
 *  @param res_json JSON string of response
 *  @return Map of session information
 */
std::map<uint32_t, std::string> JsonResponseAnalyzer::AnalyzeResponseGetAllSessions( std::string& res_json )
{
	std::map<uint32_t, std::string> session_map;

	TRACE_DEBUG("AnalyzeResponseGetAllSessions json_obj:\n%s\n", json_object_to_json_string(json_obj));

	// convert to Json Object
	struct json_object *json_obj = json_tokener_parse( res_json.c_str() );

	// Check key
	struct json_object *json_map_ary = NULL;
	if( json_object_object_get_ex(json_obj, "response", &json_map_ary) )
	{
		// Check if the response is array information
		if( json_object_is_type(json_map_ary, json_type_array) )
		{
			for (int i = 0; i < json_object_array_length(json_map_ary); ++i) 
			{
				struct json_object* j_elem = json_object_array_get_idx(json_map_ary, i);

				if( json_object_is_type( j_elem, json_type_object) )
				{
					// Check key
					struct json_object* handle = NULL;
					struct json_object* client = NULL;
					if( json_object_object_get_ex(j_elem, "sessionHandle", &handle) 
					    &&  json_object_object_get_ex(j_elem, "client", &client) )
					{
						if( json_object_is_type(handle, json_type_int)
						    &&  json_object_is_type(client, json_type_string) )
						{
							uint32_t sessionHandle = (uint32_t)json_object_get_int(handle);
							std::string clientName = std::string( json_object_get_string(client) );

							// add to map
							session_map[sessionHandle] = clientName;
						}
						else
						{
							TRACE_WARN("invalid response.\n");
							break;
						}
					}
				}
				else
				{
					TRACE_WARN("element type is not object.\n");
					break;
				}
			}
		}
		else
		{
			TRACE_WARN("response type is not array.\n");
		}
	}
    
	json_object_put(json_obj);
	return session_map;
}

