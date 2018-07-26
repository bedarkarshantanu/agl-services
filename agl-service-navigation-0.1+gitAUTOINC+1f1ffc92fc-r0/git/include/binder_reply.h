// Copyright 2017 AW SOFTWARE CO.,LTD
// Copyright 2017 AISIN AW CO.,LTD

#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <string>
#include <map>
#include <vector>
#include <json-c/json.h>

/**
 *  @brief Response to return to Binder client.
 */
typedef struct APIResponse_
{
	bool isSuccess;
	std::string errMessage;
	json_object* json_data;
}APIResponse;

/**
 *  @brief Convert information acquired by Genevi API to JSON format.
 */
class BinderReply
{
public:
	APIResponse ReplyNavicoreGetPosition( std::map<int32_t, double>& posList );
	APIResponse ReplyNavicoreGetAllRoutes( std::vector< uint32_t > &allRoutes );
	APIResponse ReplyNavicoreCreateRoute( uint32_t route );
	APIResponse ReplyNavicoreGetAllSessions( std::map<uint32_t, std::string> &allSessions );
};

