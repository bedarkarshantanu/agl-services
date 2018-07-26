// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <json-c/json.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include "libnavicore.hpp"

/**
*  @brief JSON response analysis class
*/
class JsonResponseAnalyzer
{  
public:
	static std::map< int32_t, naviapi::variant > AnalyzeResponseGetPosition( std::string& res_json );
	static std::vector< uint32_t > AnalyzeResponseGetAllRoutes( std::string& res_json );
	static uint32_t AnalyzeResponseCreateRoute( std::string& res_json );
	static std::map<uint32_t, std::string> AnalyzeResponseGetAllSessions( std::string& res_json );
};

