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

#include "source.hpp"
#include "signal-composer.hpp"

SourceAPI::SourceAPI()
{}

SourceAPI::SourceAPI(const std::string& uid, const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignals, CtlActionT* onReceived, int retention):
 uid_{uid},
 api_{api},
 info_{info},
 init_{init},
 getSignals_{getSignals},
 signalsDefault_({onReceived, retention})
{}

bool SourceAPI::operator ==(const SourceAPI& other) const
{
	if(uid_ == other.uid_) {return true;}
	return false;
}

bool SourceAPI::operator ==(const std::string& other) const
{
	if(uid_ == other) {return true;}
	return false;
}

void SourceAPI::init()
{
	if(init_)
	{
		CtlSourceT source;
		source.uid = init_->uid;
		source.api  = nullptr; // We use binding v2, no dynamic API.
		source.request = {nullptr, nullptr};
		ActionExecOne(&source, init_, nullptr);
		return;
	}
	else if(api_ == afbBindingV2.api)
		{api_ = Composer::instance().ctlConfig()->api;}
}

std::string SourceAPI::api() const
{
	return api_;
}

const struct signalsDefault& SourceAPI::signalsDefault() const
{
	return signalsDefault_;
}

void SourceAPI::addSignal(const std::string& id, const std::string& event, std::vector<std::string>& depends, int retention, const std::string& unit, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs)
{
	std::shared_ptr<Signal> sig = std::make_shared<Signal>(id, event, depends, unit, retention, frequency, onReceived, getSignalsArgs);

	newSignalsM_[id] = sig;
}

void SourceAPI::initSignals()
{
	Composer& composer = Composer::instance();
	int err = 0;
	for(auto& i: newSignalsM_)
		{i.second->attachToSourceSignals(composer);}

	for(auto i = newSignalsM_.begin(); i != newSignalsM_.end();)
	{
		if (err += i->second->initialRecursionCheck())
		{
			AFB_ERROR("There is an infinite recursion loop in your signals definition. Root coming from signal: %s. Ignoring it.", i->second->id().c_str());
			++i;
			continue;
		}
		signalsM_[i->first] = i->second;
		i = newSignalsM_.erase(i);
	}
}

std::vector<std::shared_ptr<Signal>> SourceAPI::getSignals() const
{
	std::vector<std::shared_ptr<Signal>> signals;
	for (auto& sig: signalsM_)
	{
		signals.push_back(sig.second);
	}
	for (auto& sig: newSignalsM_)
	{
		signals.push_back(sig.second);
	}

	return signals;
}

/// @brief Search a signal in a source instance. If an exact signal name is find
///  then it will be returned else it will be search against each signals
///  contained in the map and signal will be deeper evaluated.
///
/// @param[in] name - A signal name to be searched
///
/// @return Returns a vector of found signals.
std::vector<std::shared_ptr<Signal>> SourceAPI::searchSignals(const std::string& name)
{
	std::vector<std::shared_ptr<Signal>> signals;

	if(signalsM_.count(name))
		{signals.emplace_back(signalsM_[name]);}
	if(newSignalsM_.count(name))
		{signals.emplace_back(signalsM_[name]);}
	else
	{
		for (auto& sig: signalsM_)
		{
			if(*sig.second == name)
				{signals.emplace_back(sig.second);}
		}
		for (auto& sig: newSignalsM_)
		{
			if(*sig.second == name)
				{signals.emplace_back(sig.second);}
		}
	}

	return signals;
}

void SourceAPI::makeSubscription()
{
	if(getSignals_)
	{
		CtlSourceT source;
		source.uid = api_.c_str();
		source.api = nullptr; // We use binding v2, no dynamic API.
		source.request = {nullptr, nullptr};

		for(auto& sig: signalsM_)
		{
			if(!sig.second->subscribed_)
			{
				json_object* signalJ = sig.second->toJSON();
				if(!signalJ)
				{
					AFB_ERROR("Error building JSON query object to subscribe to for signal %s", sig.second->id().c_str());
					break;
				}
				source.uid = sig.first.c_str();
				source.context = getSignals_->type == CTL_TYPE_CB ?
					getSignals_->exec.cb.plugin->context:
					nullptr;
				ActionExecOne(&source, getSignals_, signalJ);
				// Considerate signal subscribed no matter what
				sig.second->subscribed_ = true;
				json_object_put(signalJ);
			}
		}
		source.uid = "";
		ActionExecOne(&source, getSignals_, nullptr);
	}
}
