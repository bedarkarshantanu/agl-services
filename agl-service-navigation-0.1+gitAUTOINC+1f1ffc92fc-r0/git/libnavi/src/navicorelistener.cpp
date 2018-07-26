// Copyright 2017 AISIN AW CO.,LTD

#include "libnavicore.hpp"

naviapi::NavicoreListener::NavicoreListener()
{
}

naviapi::NavicoreListener::~NavicoreListener()
{
}

void naviapi::NavicoreListener::getAllSessions_reply(const std::map< uint32_t, std::string >& allSessions)
{
}

void naviapi::NavicoreListener::getPosition_reply(std::map< int32_t, variant > position)
{
}

void naviapi::NavicoreListener::getAllRoutes_reply(std::vector< uint32_t > allRoutes)
{
}

void naviapi::NavicoreListener::createRoute_reply(uint32_t routeHandle)
{
}

