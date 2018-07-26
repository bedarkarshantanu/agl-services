/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Romain Forlot" <romain.forlot@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <uuid.h>
#include <string.h>
#include <fnmatch.h>

#include "clientApp.hpp"

extern "C" void searchNsetSignalValueHandle(const char* aName, uint64_t timestamp, struct signalValue value)
{
	std::vector<std::shared_ptr<Signal>> signals = Composer::instance().searchSignals(aName);
	if(!signals.empty())
	{
		for(auto& sig: signals)
			{sig->set(timestamp, value);}
	}
}

extern "C" void setSignalValueHandle(void* aSignal, uint64_t timestamp, struct signalValue value)
{
	Signal* sig = static_cast<Signal*>(aSignal);
	sig->set(timestamp, value);
}

bool startsWith(const std::string& str, const std::string& pattern)
{
	size_t sep;
	if( (sep = str.find(pattern)) != std::string::npos && !sep)
		{return true;}
	return false;
}

void extractString(void* closure, json_object* object)
{
	std::vector<std::string> *files = (std::vector<std::string>*) closure;
	const char *oneFile = json_object_get_string(object);

	files->push_back(oneFile);
}

// aSignal member value will be initialized in sourceAPI->addSignal()
static struct signalCBT pluginHandle = {
	.searchNsetSignalValue = searchNsetSignalValueHandle,
	.setSignalValue = setSignalValueHandle,
	.aSignal = nullptr,
	.pluginCtx = nullptr,
};

CtlSectionT Composer::ctlSections_[] = {
	[0]={.key="plugins" , .uid="plugins", .info=nullptr,
		.loadCB=pluginsLoad,
		.handle=&pluginHandle,
		.actions=nullptr},
	[1]={.key="sources" , .uid="sources", .info=nullptr,
		 .loadCB=loadSourcesAPI,
		 .handle=nullptr,
		 .actions=nullptr},
	[2]={.key="signals" , .uid="signals", .info=nullptr,
		 .loadCB=loadSignals,
		 .handle=nullptr,
		 .actions=nullptr},
	[3]={.key=nullptr, .uid=nullptr, .info=nullptr,
		 .loadCB=nullptr,
		 .handle=nullptr,
		 .actions=nullptr}
};

///////////////////////////////////////////////////////////////////////////////
//                             PRIVATE METHODS                               //
///////////////////////////////////////////////////////////////////////////////

Composer::Composer()
{}

Composer::~Composer()
{
	// This will free onReceived_ action member from signal objects
	// Not the best to have it occurs here instead of in Signal destructor
	for(auto& j: ctlActionsJ_)
	{
		if(j) json_object_put(j);
	}
	if (ctlConfig_->configJ) json_object_put(ctlConfig_->configJ);
	if (ctlConfig_->requireJ)json_object_put(ctlConfig_->requireJ);
	free(ctlConfig_);
}

json_object* Composer::buildPluginAction(std::string name, std::string function, json_object* functionArgsJ)
{
	json_object *callbackJ = nullptr, *ctlActionJ = nullptr;
	std::string uri = std::string(function).substr(9);
	std::vector<std::string> uriV = Composer::parseURI(uri);
	if(uriV.size() != 2)
	{
		AFB_ERROR("Miss something in uri either plugin name or function name. Uri has to be like: plugin://<plugin-name>/<function-name>");
		return nullptr;
	}
	wrap_json_pack(&callbackJ, "{ss,ss,so*}",
		"plugin", uriV[0].c_str(),
		"function", uriV[1].c_str(),
		"args", functionArgsJ);
	wrap_json_pack(&ctlActionJ, "{ss,so}",
		"uid", name.c_str(),
		"callback", callbackJ);

	return ctlActionJ;
}

json_object* Composer::buildApiAction(std::string name, std::string function, json_object* functionArgsJ)
{
	json_object *subcallJ = nullptr, *ctlActionJ = nullptr;
	std::string uri = std::string(function).substr(6);
	std::vector<std::string> uriV = Composer::parseURI(uri);
	if(uriV.size() != 2)
	{
		AFB_ERROR("Miss something in uri either plugin name or function name. Uri has to be like: api://<plugin-name>/<function-name>");
		return nullptr;
	}
	wrap_json_pack(&subcallJ, "{ss,ss}",
		"api", uriV[0].c_str(),
		"verb", uriV[1].c_str());
	wrap_json_pack(&ctlActionJ, "{ss,so,so*}",
		"uid", name.c_str(),
		"subcall", subcallJ,
		"args", functionArgsJ);

	return ctlActionJ;
}

json_object* Composer::buildLuaAction(std::string name, std::string function, json_object* functionArgsJ)
{
	json_object *luaJ = nullptr, *ctlActionJ = nullptr;
	std::string fName, filepath;
	std::string uri = std::string(function).substr(6);
	std::vector<std::string> uriV = Composer::parseURI(uri);
	if(uriV.size() > 2)
	{
		int i = 0;
		while(i < uriV.size()-1)
			{filepath += uriV[i] + "/";}
		fName = uriV[-1];
	}
	else if(uriV.size() == 2)
	{
		filepath = uriV[0];
		fName = uriV[2];
	}
	else if(uriV.size() == 1)
		{fName = uriV[0];}
	else
	{
		AFB_ERROR("Missing something in uri either lua filepath or function name. Uri has to be like: lua://file/path/file.lua/function_name with filepath optionnal. If not specified, search will be done in default directories");
		return nullptr;
	}

	wrap_json_pack(&luaJ, "{ss*,ss}",
		"load", filepath.empty() ? NULL:filepath.c_str(),
		"func", fName.c_str());
	wrap_json_pack(&ctlActionJ, "{ss,so,so*}",
		"uid", name.c_str(),
		"lua", luaJ,
		"args", functionArgsJ);

	return ctlActionJ;
}

CtlActionT* Composer::convert2Action(const std::string& name, json_object* actionJ)
{
	json_object *functionArgsJ = nullptr,
				*ctlActionJ = nullptr;
	char *function;
	const char *plugin;
	CtlActionT *ctlAction = new CtlActionT;
	memset(ctlAction, 0, sizeof(CtlActionT));

	if(actionJ &&
		!wrap_json_unpack(actionJ, "{ss,s?s,s?o !}", "function", &function,
			"plugin", &plugin,
			"args", &functionArgsJ))
	{
		if(startsWith(function, "lua://"))
		{
			ctlActionJ = buildLuaAction(name, function, functionArgsJ);
		}
		else if(startsWith(function, "api://"))
		{
			ctlActionJ = buildApiAction(name, function, functionArgsJ);
		}
		else if(startsWith(function, "plugin://"))
		{
			ctlActionJ = buildPluginAction(name, function, functionArgsJ);
		}
		else
		{
			AFB_ERROR("Wrong function uri specified. You have to specified 'lua://', 'plugin://' or 'api://'. (%s)", function);
			return nullptr;
		}
	}

	// Register json object for later release
	ctlActionsJ_.push_back(ctlActionJ);
	if(ctlActionJ)
	{
		int err = ActionLoadOne(nullptr, ctlAction, ctlActionJ, 0);
		if(! err)
			{return ctlAction;}
	}

	return nullptr;
}

/// @brief Load controller plugins
///
/// @param[in] section - Control Section structure
/// @param[in] pluginsJ - JSON object containing all plugins definition made in
///  JSON configuration file.
///
/// @return 0 if OK, other if not.
int Composer::pluginsLoad(AFB_ApiT apiHandle, CtlSectionT *section, json_object *pluginsJ)
{
	return PluginConfig(nullptr, section, pluginsJ);
}

int Composer::loadOneSourceAPI(json_object* sourceJ)
{
	json_object *initJ = nullptr,
				*getSignalsJ = nullptr,
				*onReceivedJ = nullptr;
	CtlActionT  *initCtl = nullptr,
				*getSignalsCtl = nullptr,
				*onReceivedCtl = nullptr;
	const char *uid, *api, *info;
	int retention = 0;

	int err = wrap_json_unpack(sourceJ, "{ss,s?s,s?o,s?o,s?o,s?i,s?o !}",
			"uid", &uid,
			"api", &api,
			"info", &info,
			"init", &initJ,
			"getSignals", &getSignalsJ,
			// Signals field to make signals conf by sources
			"onReceived", &onReceivedJ,
			"retention", &retention);
	if (err)
	{
		AFB_ERROR("Missing something api|[info]|[init]|[getSignals] in %s", json_object_get_string(sourceJ));
		return err;
	}

	// Checking duplicate entry and ignore if so
	for(auto& src: sourcesListV_)
	{
		if(*src == uid)
		{
			json_object_put(sourceJ);
			return 0;
		}
	}

	if(ctlConfig_ && ctlConfig_->requireJ)
	{
		const char* requireS = json_object_to_json_string(ctlConfig_->requireJ);
		if(!strcasestr(requireS, api) && !strcasestr(api, afbBindingV2.api))
			{AFB_WARNING("Caution! You don't specify the API source as required in the metadata section. This API '%s' may not be initialized", api);}
	}

	initCtl = initJ ? convert2Action("init", initJ) : nullptr;

	// Define default action to take to subscibe souce's signals if none
	// defined.
	if(!getSignalsJ)
	{
		std::string function = "api://" + std::string(api) + "/subscribe";
		getSignalsJ = buildApiAction("getSignals", function, nullptr);
		// Register json object for later release
		ctlActionsJ_.push_back(getSignalsJ);
	}
	getSignalsCtl = convert2Action("getSignals", getSignalsJ);

	onReceivedCtl = onReceivedJ ? convert2Action("onReceived", onReceivedJ) : nullptr;

	sourcesListV_.push_back(std::make_shared<SourceAPI>(uid, api, info, initCtl, getSignalsCtl, onReceivedCtl, retention));
	return err;
}

int Composer::loadSourcesAPI(AFB_ApiT apihandle, CtlSectionT* section, json_object *sourcesJ)
{
	int err = 0;
	Composer& composer = instance();

	if(sourcesJ)
	{
		int count = 1;
		json_object *sigCompJ = nullptr;
		// add the signal composer itself as source
		wrap_json_pack(&sigCompJ, "{ss,ss,ss}",
		"uid", "Signal-Composer-service",
		"api", afbBindingV2.api,
		"info", "Api on behalf the virtual signals are sent");

		if(json_object_is_type(sourcesJ, json_type_array))
			json_object_array_add(sourcesJ, sigCompJ);

		if (json_object_get_type(sourcesJ) == json_type_array)
		{
			count = json_object_array_length(sourcesJ);

			for (int idx = 0; idx < count; idx++)
			{
				json_object *sourceJ = json_object_array_get_idx(sourcesJ, idx);
				err = composer.loadOneSourceAPI(sourceJ);
				if (err) return err;
			}
		}
		else
		{
			if ((err = composer.loadOneSourceAPI(sourcesJ))) return err;
			if (sigCompJ && (err = composer.loadOneSourceAPI(sigCompJ))) return err;
		}
		AFB_NOTICE("%d new sources added to service", count);
	}
	else
		{Composer::instance().initSourcesAPI();}

	return err;
}

int Composer::loadOneSignal(json_object* signalJ)
{
	json_object *onReceivedJ = nullptr,
				*dependsJ = nullptr,
				*getSignalsArgs = nullptr;
	CtlActionT* onReceivedCtl;
	const char *id = nullptr,
			   *event = nullptr,
			   *unit = nullptr;
	int retention = 0;
	double frequency=0.0;
	std::vector<std::string> dependsV;
	ssize_t sep;
	std::shared_ptr<SourceAPI> src = nullptr;

	int err = wrap_json_unpack(signalJ, "{ss,s?s,s?o,s?o,s?i,s?s,s?F,s?o !}",
			"uid", &id,
			"event", &event,
			"depends", &dependsJ,
			"getSignalsArgs", &getSignalsArgs,
			"retention", &retention,
			"unit", &unit,
			"frequency", &frequency,
			"onReceived", &onReceivedJ);
	if (err)
	{
		AFB_ERROR("Missing something uid|[event|depends]|[getSignalsArgs]|[retention]|[unit]|[frequency]|[onReceived] in %s", json_object_get_string(signalJ));
		return err;
	}

	// event or depends field manadatory
	if( (!event && !dependsJ) || (event && dependsJ) )
	{
		AFB_ERROR("Missing something uid|[event|depends]|[getSignalsArgs]|[retention]|[unit]|[frequency]|[onReceived] in %s. Or you declare event AND depends, only one of them is needed.", json_object_get_string(signalJ));
		return -1;
	}

	// Declare a raw signal
	if(event)
	{
		std::string eventStr = std::string(event);
		if( (sep = eventStr.find("/")) == std::string::npos)
		{
			AFB_ERROR("Missing something in event declaration. Has to be like: <api>/<event>");
			return -1;
		}
		std::string api = eventStr.substr(0, sep);
		src = getSourceAPI(api);
	}
	else
	{
		event = "";
	}
	// Process depends JSON object to declare virtual signal dependencies
	if (dependsJ)
	{
		if(json_object_get_type(dependsJ) == json_type_array)
		{
			int count = json_object_array_length(dependsJ);
			for(int i = 0; i < count; i++)
			{
				std::string sourceStr = json_object_get_string(json_object_array_get_idx(dependsJ, i));
				if( (sep = sourceStr.find("/")) != std::string::npos)
				{
					AFB_ERROR("Signal composition needs to use signal 'id', don't use full low level signal name");
					return -1;
				}
				dependsV.push_back(sourceStr);
			}
		}
		else
		{
			std::string sourceStr = json_object_get_string(dependsJ);
			if( (sep = sourceStr.find("/")) != std::string::npos)
			{
				AFB_ERROR("Signal composition needs to use signal 'id', don't use full low level signal name");
				return -1;
			}
			dependsV.push_back(sourceStr);
		}
		if(!src) {src=*sourcesListV_.rbegin();}
	}

	// Set default retention if not specified
	if(!retention)
	{
		retention = src->signalsDefault().retention ?
			src->signalsDefault().retention :
			30;
	}
	unit = !unit ? "" : unit;

	// Set default onReceived action if not specified
	char uid[CONTROL_MAXPATH_LEN] = "onReceived_";
	strncat(uid, id, strlen(id));

	if(!onReceivedJ)
	{
		onReceivedCtl = src->signalsDefault().onReceived ?
			src->signalsDefault().onReceived :
			nullptr;
			// Overwrite uid to the signal one instead of the default
			if(onReceivedCtl)
				{onReceivedCtl->uid = uid;}
	}
	else {onReceivedCtl = convert2Action(uid, onReceivedJ);}

	if(src != nullptr)
		{src->addSignal(id, event, dependsV, retention, unit, frequency, onReceivedCtl, getSignalsArgs);}
	else
		{err = -1;}

	return err;
}

int Composer::loadSignals(AFB_ApiT apihandle, CtlSectionT* section, json_object *signalsJ)
{
	int err = 0;
	Composer& composer = instance();

	if(signalsJ)
	{
		int count = 1;
		if (json_object_get_type(signalsJ) == json_type_array)
		{
			count = json_object_array_length(signalsJ);
			for (int idx = 0; idx < count; idx++)
			{
				json_object *signalJ = json_object_array_get_idx(signalsJ, idx);
				err += composer.loadOneSignal(signalJ);
			}
		}
		else
			{err = composer.loadOneSignal(signalsJ);}
		AFB_NOTICE("%d new signals added to service", count);
	}

	return err;
}

void Composer::processOptions(const std::map<std::string, int>& opts, std::shared_ptr<Signal> sig, json_object* response) const
{
	for(const auto& o: opts)
	{
		bool avg = false, min = false, max = false, last = false;
		if (o.first.compare("average") && !avg)
		{
			avg = true;
			double value = sig->average(o.second);
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("minimum") && !min)
		{
			min = true;
			double value = sig->minimum();
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("maximum") && !max)
		{
			max = true;
			double value = sig->maximum();
			json_object_object_add(response, "value",
				json_object_new_double(value));
		}
		else if (o.first.compare("last") && !last)
		{
			last = true;
			struct signalValue value = sig->last();
			if(value.hasBool)
			{
				json_object_object_add(response, "value",
					json_object_new_boolean(value.boolVal));
			}
			else if(value.hasNum)
			{
				json_object_object_add(response, "value",
					json_object_new_double(value.numVal));
			}
			else if(value.hasStr)
			{
				json_object_object_add(response, "value",
					json_object_new_string(value.strVal.c_str()));
			}
			else
			{
				json_object_object_add(response, "value",
					json_object_new_string("No recorded value so far."));
			}
		}
		else
		{
			json_object_object_add(response, "value",
				json_object_new_string("No recorded value so far."));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
//                             PUBLIC METHODS                                //
///////////////////////////////////////////////////////////////////////////////

Composer& Composer::instance()
{
	static Composer* composer;
	if(!composer) composer = new Composer();
	return *composer;
}

void* Composer::createContext(void* ctx)
{
	uuid_t x;
	char cuid[38];
	uuid_generate(x);
	uuid_unparse(x, cuid);
	clientAppCtx* ret = new clientAppCtx(cuid);

	return (void*)ret;
}

void Composer::destroyContext(void* ctx)
{
	delete(reinterpret_cast<clientAppCtx*>(ctx));
}

std::vector<std::string> Composer::parseURI(const std::string& uri)
{
	std::vector<std::string> uriV;
	std::string delimiters = "/";

	std::string::size_type start = 0;
	auto pos = uri.find_first_of(delimiters, start);
	while(pos != std::string::npos)
	{
		if(pos != start) // ignore empty tokens
			uriV.emplace_back(uri, start, pos - start);
		start = pos + 1;
		pos = uri.find_first_of(delimiters, start);
	}

	if(start < uri.length()) // ignore trailing delimiter
	uriV.emplace_back(uri, start, uri.length() - start); // add what's left of the string

	return uriV;
}

int Composer::loadConfig(std::string& filepath)
{
	const char *dirList= getenv("CONTROL_CONFIG_PATH");
	if (!dirList) dirList=CONTROL_CONFIG_PATH;
	filepath.append(":");
	filepath.append(dirList);
	const char *configPath = CtlConfigSearch(nullptr, filepath.c_str(), "control-");

	if (!configPath) {
		AFB_ApiError(apiHandle, "CtlPreInit: No control-* config found invalid JSON %s ", filepath.c_str());
		return -1;
	}

	// create one API per file
	ctlConfig_ = CtlLoadMetaData(nullptr, configPath);
	if (!ctlConfig_) {
		AFB_ApiError(apiHandle, "CtrlPreInit No valid control config file in:\n-- %s", configPath);
		return -1;
	}

	if (ctlConfig_->api) {
		int err = afb_daemon_rename_api(ctlConfig_->api);
		if (err) {
			AFB_ApiError(apiHandle, "Fail to rename api to:%s", ctlConfig_->api);
			return -1;
		}
	}

	int err= CtlLoadSections(nullptr, ctlConfig_, ctlSections_);
	return err;
}

int Composer::loadSources(json_object* sourcesJ)
{
	int err = loadSourcesAPI(nullptr, nullptr, sourcesJ);
	if(err)
	{
		AFB_ERROR("Loading sources failed. JSON: %s", json_object_to_json_string(sourcesJ));
		return err;
	}
	initSourcesAPI();
	return err;
}

int Composer::loadSignals(json_object* signalsJ)
{
	return loadSignals(nullptr, nullptr, signalsJ);
}

CtlConfigT* Composer::ctlConfig()
{
	return ctlConfig_;
}

void Composer::initSourcesAPI()
{
	for(int i=0; i < newSourcesListV_.size(); i++)
	{
		std::shared_ptr<SourceAPI> src = newSourcesListV_.back();
		newSourcesListV_.pop_back();
		src->init();
		sourcesListV_.push_back(src);
	}
}

void Composer::initSignals()
{
	for(int i=0; i < sourcesListV_.size(); i++)
	{
		std::shared_ptr<SourceAPI> src = sourcesListV_[i];
		src->initSignals();
	}
	execSignalsSubscription();
}

std::shared_ptr<SourceAPI> Composer::getSourceAPI(const std::string& api)
{
	for(auto& source: sourcesListV_)
	{
		if (source->api() == api)
			{return source;}
	}
	for(auto& source: newSourcesListV_)
	{
		if (source->api() == api)
			{return source;}
	}
	return nullptr;
}

std::vector<std::shared_ptr<Signal>> Composer::getAllSignals()
{
	std::vector<std::shared_ptr<Signal>> allSignals;
	for( auto& source : sourcesListV_)
	{
		std::vector<std::shared_ptr<Signal>> srcSignals = source->getSignals();
		allSignals.insert(allSignals.end(), srcSignals.begin(), srcSignals.end());
	}

	return allSignals;
}

std::vector<std::shared_ptr<Signal>> Composer::searchSignals(const std::string& aName)
{
	std::string api;
	std::vector<std::shared_ptr<Signal>> signals;
	size_t sep = aName.find_first_of("/");
	if(sep != std::string::npos)
	{
		api = aName.substr(0, sep);
		std::shared_ptr<SourceAPI> source = getSourceAPI(api);
		return source->searchSignals(aName);
	}
	else
	{
		std::vector<std::shared_ptr<Signal>> allSignals = getAllSignals();
		for (std::shared_ptr<Signal>& sig : allSignals)
		{
			if(*sig == aName)
				{signals.emplace_back(sig);}
		}
	}
	return signals;
}

json_object* Composer::getsignalValue(const std::string& sig, json_object* options)
{
	std::map<std::string, int> opts;
	json_object *response = nullptr, *finalResponse = json_object_new_array();

	wrap_json_unpack(options, "{s?i, s?i, s?i, s?i !}",
		"average", &opts["average"],
		"minimum", &opts["minimum"],
		"maximum", &opts["maximum"],
		"last", &opts["last"]);

	std::vector<std::shared_ptr<Signal>> sigP = searchSignals(sig);
	if(!sigP.empty())
	{
		for(auto& sig: sigP)
		{
			wrap_json_pack(&response, "{ss}",
				"signal", sig->id().c_str());
			if (opts.empty())
			{
				struct signalValue value = sig->last();
				if(value.hasBool)
				{
					json_object_object_add(response, "value",
						json_object_new_boolean(value.boolVal));
				}
				else if(value.hasNum)
				{
					json_object_object_add(response, "value",
						json_object_new_double(value.numVal));
				}
				else if(value.hasStr)
				{
					json_object_object_add(response, "value",
						json_object_new_string(value.strVal.c_str()));
				}
				else
				{
					json_object_object_add(response, "value",
						json_object_new_string("No recorded value so far."));
				}
			}
			else
				{processOptions(opts, sig, response);}
			json_object_array_add(finalResponse, response);
		}
	}

	return finalResponse;
}

void Composer::execSignalsSubscription()
{
	for(std::shared_ptr<SourceAPI> srcAPI: sourcesListV_)
	{
		if (srcAPI->api() != std::string(ctlConfig_->api))
		{
			srcAPI->makeSubscription();
		}
	}
}
