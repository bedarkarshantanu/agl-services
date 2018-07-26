// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#include <string.h>

#include "binder_reply.h"
#include "genivi_request.h"
#include "analyze_request.h"
#include "genivi/genivi-navicore-constants.h"

#define AFB_BINDING_VERSION 2

extern "C" {
	#include <afb/afb-binding.h>
}

/**
 *  Variable declaration
 */
GeniviRequest* geniviRequest;	// Send request to Genivi
BinderReply* binderReply;	// Convert Genivi response result to json format
AnalyzeRequest* analyzeRequest;	// Analyze BinderClient's request and create arguments to pass to GeniviAPI

/**
 *  @brief navicore_getposition request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreGetPosition(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_getposition");

	// Request of Json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	std::vector< int32_t > Params;
	if( !analyzeRequest->CreateParamsGetPosition( req_json_str, Params ))
	{
		afb_req_fail(req, "failed", "navicore_getposition Bad Request");
		return;
	}

	// GENIVI API call
	std::map< int32_t, double > posList = geniviRequest->NavicoreGetPosition( Params );

	// Convert to json style response
	APIResponse response = binderReply->ReplyNavicoreGetPosition( posList );

	// On success
	if(response.isSuccess)
	{
		AFB_REQ_NOTICE(req, "res_json_str = %s", json_object_to_json_string(response.json_data));
		// Return success to BinderClient
		afb_req_success(req, response.json_data, "navicore_getposition");
	}
	else
	{
		AFB_REQ_ERROR(req, "%s - %s:%d", response.errMessage.c_str(), __FILE__, __LINE__);
		afb_req_fail(req, "failed", "navicore_getposition Bad Request");
	}

	// Json object release
	json_object_put(response.json_data);

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_getallroutes request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreGetAllRoutes(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_getallroutes");

	// No request information in json format
	AFB_REQ_NOTICE(req, "req_json_str = none");

	// GENEVI API call
	std::vector< uint32_t > allRoutes = geniviRequest->NavicoreGetAllRoutes();

	// Convert to json style response
	APIResponse response = binderReply->ReplyNavicoreGetAllRoutes( allRoutes );

	// On success
	if(response.isSuccess)
	{
		AFB_REQ_NOTICE(req, "res_json_str = %s", json_object_to_json_string(response.json_data));
		// Return success to BinderClient
		afb_req_success(req, response.json_data, "navicore_getallroutes");
	}
	else
	{
		AFB_REQ_ERROR(req, "%s - %s:%d", response.errMessage.c_str(), __FILE__, __LINE__);
		afb_req_fail(req, "failed", "navicore_getallroutes Bad Request");
	}

	// json object release
	json_object_put(response.json_data);

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_createroute request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreCreateRoute(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s ", __func__);
	AFB_REQ_DEBUG(req, "request navicore_createroute");

	// Request of json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	if( !analyzeRequest->CreateParamsCreateRoute( req_json_str, sessionHdl ))
	{
		afb_req_fail(req, "failed", "navicore_createroute Bad Request");
		return;
	}

	// GENEVI API call
	uint32_t routeHdl = geniviRequest->NavicoreCreateRoute( sessionHdl );

	// Convert to json style response
	APIResponse response = binderReply->ReplyNavicoreCreateRoute( routeHdl );

	// On success
	if(response.isSuccess)
	{
		AFB_REQ_NOTICE(req, "res_json_str = %s", json_object_to_json_string(response.json_data));
		// Return success to BinderClient
		afb_req_success(req, response.json_data, "navicore_createroute");
	}
	else
	{
		AFB_REQ_ERROR(req, "%s - %s:%d", response.errMessage.c_str(), __FILE__, __LINE__);
		afb_req_fail(req, "failed", "navicore_createroute Bad Request");
	}

	// json object release
	json_object_put(response.json_data);

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_pausesimulation request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicorePauseSimulation(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_pausesimulation");

	// Request of json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	if( !analyzeRequest->CreateParamsPauseSimulation( req_json_str, sessionHdl ))
	{
		afb_req_fail(req, "failed", "navicore_pausesimulation Bad Request");
		return;
	}

	// GENEVI API call
	geniviRequest->NavicorePauseSimulation( sessionHdl );

	// No reply unnecessary API for conversion to json format response is unnecessary
	AFB_REQ_NOTICE(req, "res_json_str = none");

	// Return success to BinderClient
	afb_req_success(req, NULL, "navicore_pausesimulation");

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_setsimulationmode request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreSetSimulationMode(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_setsimulationmode");

	// Request of json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	bool simuMode = false;
	if( !analyzeRequest->CreateParamsSetSimulationMode( req_json_str, sessionHdl, simuMode ))
	{
		afb_req_fail(req, "failed", "navicore_setsimulationmode Bad Request");
		return;
	}

	// GENEVI API call
	geniviRequest->NavicoreSetSimulationMode( sessionHdl, simuMode );

	// No reply unnecessary API for conversion to json format response is unnecessary
	AFB_REQ_NOTICE(req, "res_json_str = none");

	// Return success to BinderClient
	afb_req_success(req, NULL, "navicore_setsimulationmode");

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_cancelroutecalculation request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreCancelRouteCalculation(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_cancelroutecalculation");

	// Request of Json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	uint32_t routeHdl = 0;
	if( !analyzeRequest->CreateParamsCancelRouteCalculation( req_json_str, sessionHdl, routeHdl ))
	{
		afb_req_fail(req, "failed", "navicore_cancelroutecalculation Bad Request");
		return;
	}

	// GENEVI API call
	geniviRequest->NavicoreCancelRouteCalculation( sessionHdl, routeHdl );

	// No reply unnecessary API for conversion to json format response is unnecessary
	AFB_REQ_NOTICE(req, "res_json_str = none");

	// Return success to BinderClient
	afb_req_success(req, NULL, "navicore_cancelroutecalculation");

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_setwaypoints request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreWaypoints(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_setwaypoints");

	// Request of Json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	uint32_t routeHdl = 0;
	bool currentPos = false;
	std::vector<Waypoint> waypointsList;
	if( !analyzeRequest->CreateParamsSetWaypoints( req_json_str, sessionHdl, routeHdl, currentPos, waypointsList ))
	{
		afb_req_fail(req, "failed", "navicore_setwaypoints Bad Request");
		return;
	}

	// GENEVI API call
	geniviRequest->NavicoreSetWaypoints( sessionHdl, routeHdl, currentPos, waypointsList );

	// No reply unnecessary API for conversion to json format response is unnecessary
	AFB_REQ_NOTICE(req, "res_json_str = none");

	// Return success to BinderClient
	afb_req_success(req, NULL, "navicore_setwaypoints");

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_calculateroute request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreCalculateRoute(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_calculateroute");

	// Request of Json format request
	json_object* req_json = afb_req_json(req);
	const char* req_json_str = json_object_to_json_string(req_json);
	AFB_REQ_NOTICE(req, "req_json_str = %s", req_json_str);

	// Request analysis and create arguments to pass to Genivi
	uint32_t sessionHdl = 0;
	uint32_t routeHdl = 0;
	if( !analyzeRequest->CreateParamsCalculateRoute( req_json_str, sessionHdl, routeHdl ))
	{
		afb_req_fail(req, "failed", "navicore_calculateroute Bad Request");
		return;
	}

	// GENEVI API call
	geniviRequest->NavicoreCalculateRoute( sessionHdl, routeHdl );

	// No reply unnecessary API for conversion to json format response is unnecessary
	AFB_REQ_NOTICE(req, "res_json_str = none");

	// Return success to BinderClient
	afb_req_success(req, NULL, "navicore_calculateroute");

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief navicore_getallsessions request callback
 *  @param[in] req Request from client
 */
void OnRequestNavicoreGetAllSessions(afb_req req)
{
	AFB_REQ_NOTICE(req, "--> Start %s()", __func__);
	AFB_REQ_DEBUG(req, "request navicore_getallsessions");

	// No request information in Json format
	AFB_REQ_NOTICE(req, "req_json_str = none");

	// GENEVI API call
	std::map<uint32_t, std::string> allSessions = geniviRequest->NavicoreGetAllSessions();

	// Convert to json style response
	APIResponse response = binderReply->ReplyNavicoreGetAllSessions( allSessions );

	// On success
	if(response.isSuccess)
	{
		AFB_REQ_NOTICE(req, "res_json_str = %s", json_object_to_json_string(response.json_data));
		// Return success to BinderClient
		afb_req_success(req, response.json_data, "navicore_getallsessions");
	}
	else
	{
		AFB_REQ_ERROR(req, "%s - %s:%d", response.errMessage.c_str(), __FILE__, __LINE__);
		afb_req_fail(req, "failed", "navicore_getallsessions Bad Request");
	}

	// json object release
	json_object_put(response.json_data);

	AFB_REQ_NOTICE(req, "<-- End %s()", __func__);
}


/**
 *  @brief Callback called at service startup
 */
int Init()
{
	// Create instance
	geniviRequest   = new GeniviRequest();
	binderReply	 = new BinderReply();
	analyzeRequest  = new AnalyzeRequest();
	
	return 0;
}

/**
 *  @brief API definition
 */
const afb_verb_v2 verbs[] = 
{
	 { verb : "navicore_getposition",			callback : OnRequestNavicoreGetPosition },
	 { verb : "navicore_getallroutes",		   callback : OnRequestNavicoreGetAllRoutes },
	 { verb : "navicore_createroute",			callback : OnRequestNavicoreCreateRoute },
	 { verb : "navicore_pausesimulation",		callback : OnRequestNavicorePauseSimulation },
	 { verb : "navicore_setsimulationmode",	  callback : OnRequestNavicoreSetSimulationMode },
	 { verb : "navicore_cancelroutecalculation", callback : OnRequestNavicoreCancelRouteCalculation },
	 { verb : "navicore_setwaypoints",		   callback : OnRequestNavicoreWaypoints },
	 { verb : "navicore_calculateroute",		 callback : OnRequestNavicoreCalculateRoute },
	 { verb : "navicore_getallsessions",		 callback : OnRequestNavicoreGetAllSessions },
	 { verb : NULL }
};

/**
 *  @brief Service definition
 */
const afb_binding_v2 afbBindingV2 = 
{
	 "naviapi",
	 "",
	 "",
	 verbs,
	 NULL,
	 Init
};

