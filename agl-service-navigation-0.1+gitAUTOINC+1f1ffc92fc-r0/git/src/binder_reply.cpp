// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include "binder_reply.h"
#include "genivi/genivi-navicore-constants.h"

/**
 *  @brief      GeniviAPI GetPosition call
 *  @param[in]  posList Map information on key and value of information acquired from Genivi
 *  @return     Response information
 */
APIResponse BinderReply::ReplyNavicoreGetPosition( std::map<int32_t, double>& posList )
{
	APIResponse response = {0};

	// Json information to return as a response
	struct json_object* response_json = json_object_new_array();
	std::map<int32_t, double>::iterator it;
    
	// If the argument map is empty return
	if(posList.empty())
	{
		response.isSuccess  = false;
		response.errMessage = "posList is empty";
		response.json_data  = response_json;
		return response;
	}

	// Make the passed Genivi response json format
	for (it = posList.begin(); it != posList.end(); it++)
	{
		struct json_object* obj = json_object_new_object();

		switch(it->first)
		{
		case NAVICORE_LATITUDE:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_LATITUDE));
			json_object_object_add(obj, "value", json_object_new_double(it->second) );
			json_object_array_add(response_json, obj);
			break;

		case NAVICORE_LONGITUDE:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_LONGITUDE));
			json_object_object_add(obj, "value", json_object_new_double(it->second));
			json_object_array_add(response_json, obj);
			break;

		case NAVICORE_HEADING:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_HEADING));
			json_object_object_add(obj, "value", json_object_new_boolean (it->second));
			json_object_array_add(response_json, obj);
			break;
#if 0
		// no support
		case NAVICORE_TIMESTAMP:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_TIMESTAMP));
			json_object_object_add(obj, "value", json_object_new_int(it->second));
			json_object_array_add(response_json, obj);
			break;

		// no support
		case NAVICORE_SPEED:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_SPEED));
			json_object_object_add(obj, "value", json_object_new_int(it->second));
			json_object_array_add(response_json, obj);
			break;
#endif

		case NAVICORE_SIMULATION_MODE:
			json_object_object_add(obj, "key", json_object_new_int(NAVICORE_SIMULATION_MODE));
			json_object_object_add(obj, "value", json_object_new_boolean (it->second));
			json_object_array_add(response_json, obj);
			break;

		default:
			fprintf(stderr, "Unknown key.");
			json_object_put(obj);
			break;
		}
	}

	response.json_data = response_json;
	response.isSuccess = true;
	return response;
}

/**
 *  @brief      GeniviAPI GetAllRoutes call
 *  @param[in]  allRoutes Route handle information
 *  @return     Response information
 */
APIResponse BinderReply::ReplyNavicoreGetAllRoutes( std::vector< uint32_t > &allRoutes )
{
	APIResponse response = {0};

	// Json information to return as a response
	struct json_object* response_json = json_object_new_array();

	if (0 < allRoutes.size())
	{
		std::vector< uint32_t >::iterator it;

		for (it = allRoutes.begin(); it != allRoutes.end(); it++)
		{
			struct json_object* obj = json_object_new_object();
			json_object_object_add(obj, "route", json_object_new_int(*it));
			json_object_array_add(response_json, obj);
		}
	}

	response.json_data = response_json;
	response.isSuccess = true;
	return response;
}

/**
 *  @brief      GeniviAPI CreateRoute call
 *  @param[in]  route Route handle
 *  @return     Response information
 */
APIResponse BinderReply::ReplyNavicoreCreateRoute( uint32_t route )
{
	APIResponse response;

	// Json information to return as a response
	struct json_object* response_json = json_object_new_object();
	json_object_object_add(response_json, "route", json_object_new_int(route));

	response.json_data = response_json;
	response.isSuccess = true;
	return response;
}

/**
 *  @brief      GeniviAPI GetAllSessions call
 *  @param[in]  allSessions Map information on key and value of information acquired from Genivi
 *  @return     Response information
 */
APIResponse BinderReply::ReplyNavicoreGetAllSessions( std::map<uint32_t, std::string> &allSessions )
{
	APIResponse response = {0};

	// Json information to return as a response
	struct json_object* response_json = json_object_new_array();
	std::map<uint32_t, std::string>::iterator it;

	for (it = allSessions.begin(); it != allSessions.end(); it++)
	{
		struct json_object* obj = json_object_new_object();

		if (NAVICORE_INVALID != it->first)
		{
			json_object_object_add(obj, "sessionHandle", json_object_new_int(it->first));
			json_object_object_add(obj, "client", json_object_new_string(it->second.c_str()));
			json_object_array_add(response_json, obj);
		}
		else
		{
			fprintf(stderr, "invalid key.");
			json_object_put(obj);
		}
	}

	response.json_data = response_json;
	response.isSuccess = true;
	return response;
}

