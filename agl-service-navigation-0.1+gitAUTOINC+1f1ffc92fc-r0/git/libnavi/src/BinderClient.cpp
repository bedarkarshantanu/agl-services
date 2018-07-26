// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include <cstring>

#include "BinderClient.h"
#include "JsonRequestGenerator.h"
#include "JsonResponseAnalyzer.h"

#include "traces.h"

/**
 *  @brief constructor
 */
BinderClient::BinderClient() : navicoreListener(nullptr)
{
	requestMng = new RequestManage();
}

/**
 *  @brief Destructor
 */
BinderClient::~BinderClient()
{
	delete requestMng;
}

/**
 *  @brief Connect with the Binder server
 */
bool BinderClient::ConnectServer(std::string url, naviapi::NavicoreListener* listener)
{
	this->navicoreListener = listener;

	if( !requestMng->Connect(url.c_str(), this))
	{
		TRACE_ERROR("cannot connect to binding service.\n");
		return false;
	}

	return true;
}

/**
 *  @brief Call Genivi's GetPosition via Binder and get the result
 */
void BinderClient::NavicoreGetPosition(const std::vector< int32_t >& valuesToReturn)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		std::string req_json = JsonRequestGenerator::CreateRequestGetPosition(valuesToReturn);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_GETPOSITION, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_getposition success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_getposition failed.\n");
		}
	}
}

/**
 *  @brief Get route handle
 */
void BinderClient::NavicoreGetAllRoutes()
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		std::string req_json = JsonRequestGenerator::CreateRequestGetAllRoutes();

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_GETALLROUTES, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_getallroutes success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_getallroutes failed.\n");
		}
	}
}

/**
 *  @brief Generate route handle
 */
void BinderClient::NavicoreCreateRoute(const uint32_t& sessionHandle)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestCreateRoute(&session);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_CREATEROUTE, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_createroute success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_createroute failed.\n");
		}
	}
}

/**
 *  @brief  Pause demo
 */
void BinderClient::NavicorePauseSimulation(const uint32_t& sessionHandle)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestPauseSimulation(&session);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_PAUSESIMULATION, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_pausesimulationmode success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_pausesimulationmode failed.\n");
		}
	}
}

/**
 *  @brief  Simulation mode setting
 */
void BinderClient::NavicoreSetSimulationMode(const uint32_t& sessionHandle, const bool& activate)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestSetSimulationMode(&session, &activate);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_SETSIMULATIONMODE, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_setsimulationmode success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_setsimulationmode failed.\n");
		}
	}
}

/**
 *  @brief  Delete route information
 */
void BinderClient::NavicoreCancelRouteCalculation(const uint32_t& sessionHandle, const uint32_t& routeHandle)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestCancelRouteCalculation(&session, &routeHandle);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_CANCELROUTECALCULATION, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_cancelroutecalculation success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_cancelroutecalculation failed.\n");
		}
	}
}

/**
 *  @brief Destination setting
 */
void BinderClient::NavicoreSetWaypoints(const uint32_t& sessionHandle, const uint32_t& routeHandle, const bool& startFromCurrentPosition, const std::vector<naviapi::Waypoint>& waypointsList)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		uint32_t route = requestMng->GetRouteHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestSetWaypoints(&session, &route, 
					&startFromCurrentPosition, &waypointsList);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_SETWAYPOINTS, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_setwaypoints success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_setwaypoints failed.\n");
		}
	}
}

/**
 *  @brief  Route calculation processing
 */
void BinderClient::NavicoreCalculateRoute(const uint32_t& sessionHandle, const uint32_t& routeHandle)
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		uint32_t session = requestMng->GetSessionHandle();
		uint32_t route = requestMng->GetRouteHandle();
		std::string req_json = JsonRequestGenerator::CreateRequestCalculateroute(&session, &route);

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_CALCULATEROUTE, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_calculateroute success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_calculateroute failed.\n");
		}
	}
}

/**
 *  @brief  Retrieve session information
 *  @return Map of session information
 */
void BinderClient::NavicoreGetAllSessions()
{
	// Check if it is connected
	if( requestMng->IsConnect() )
	{
		// JSON request generation
		std::string req_json = JsonRequestGenerator::CreateRequestGetAllSessions();

		// Send request
		if( requestMng->CallBinderAPI(API_NAME, VERB_GETALLSESSIONS, req_json.c_str()) )
		{
			TRACE_DEBUG("navicore_getallsessions success.\n");
		}
		else
		{
			TRACE_ERROR("navicore_getallsessions failed.\n");
		}
	}
}

void BinderClient::OnReply(struct json_object* reply)
{
	struct json_object* requestObject = nullptr;
	json_object_object_get_ex(reply, "request", &requestObject);

	struct json_object* infoObject = nullptr;
	json_object_object_get_ex(requestObject, "info", &infoObject);

	const char* info = json_object_get_string(infoObject);

	char tmpVerb[256];
	strcpy(tmpVerb, info);

	// Create a new JSON response
	const char* json_str = json_object_to_json_string_ext(reply, JSON_C_TO_STRING_PRETTY);
	std::string response_json = std::string( json_str );

	if (strcmp(VERB_GETALLSESSIONS, tmpVerb) == 0)
	{
		std::map<uint32_t, std::string> ret = JsonResponseAnalyzer::AnalyzeResponseGetAllSessions(response_json);

		// keep session handle
		requestMng->SetSessionHandle( ret.begin()->first );

		this->navicoreListener->getAllSessions_reply(ret);
	}
	else if (strcmp(VERB_GETPOSITION, tmpVerb) == 0)
	{
		std::map< int32_t, naviapi::variant > ret = JsonResponseAnalyzer::AnalyzeResponseGetPosition(response_json);

		this->navicoreListener->getPosition_reply(ret);
	}
	else if (strcmp(VERB_GETALLROUTES, tmpVerb) == 0)
	{
		std::vector< uint32_t > ret = JsonResponseAnalyzer::AnalyzeResponseGetAllRoutes(response_json);

		// route handle
		if(ret.size() > 0)
		{
			requestMng->SetRouteHandle(ret[0]);
		}

		this->navicoreListener->getAllRoutes_reply(ret);
	}
	else if (strcmp(VERB_CREATEROUTE, tmpVerb) == 0)
	{
		uint32_t ret = JsonResponseAnalyzer::AnalyzeResponseCreateRoute(response_json);

		// keep route handle
		requestMng->SetRouteHandle(ret);

		this->navicoreListener->createRoute_reply(ret);
	}
}

