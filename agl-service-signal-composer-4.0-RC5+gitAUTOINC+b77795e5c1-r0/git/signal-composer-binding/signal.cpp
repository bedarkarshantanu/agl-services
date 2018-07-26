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

#include <float.h>
#include <string.h>

#include "signal.hpp"
#include "signal-composer.hpp"

#define USEC_TIMESTAMP_FLAG 1506514324881224

Signal::Signal()
:id_(""),
 event_(""),
 dependsSigV_(),
 timestamp_(0.0),
 value_(),
 retention_(0),
 frequency_(0),
 unit_(""),
 onReceived_(nullptr),
 getSignalsArgs_(nullptr),
 signalCtx_({nullptr, nullptr, nullptr, nullptr}),
 subscribed_(false)
{}

Signal::Signal(const std::string& id, const std::string& event, std::vector<std::string>& depends, const std::string& unit, int retention, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs)
:id_(id),
 event_(event),
 dependsSigV_(depends),
 timestamp_(0.0),
 value_(),
 retention_(retention),
 frequency_(frequency),
 unit_(unit),
 onReceived_(onReceived),
 getSignalsArgs_(getSignalsArgs),
 signalCtx_({nullptr, nullptr, nullptr, nullptr}),
 subscribed_(false)
{}

Signal::Signal(const std::string& id,
	std::vector<std::string>& depends,
	const std::string& unit,
	int retention,
	double frequency,
	CtlActionT* onReceived)
:id_(id),
 event_(),
 dependsSigV_(depends),
 timestamp_(0.0),
 value_(),
 retention_(retention),
 frequency_(frequency),
 unit_(unit),
 onReceived_(onReceived),
 getSignalsArgs_(),
 signalCtx_({nullptr, nullptr, nullptr, nullptr}),
 subscribed_(false)
{}

Signal::~Signal()
{
	if(getSignalsArgs_) json_object_put(getSignalsArgs_);
	if(onReceived_) delete(onReceived_);
}

Signal::operator bool() const
{
	if(id_.empty())
		{return false;}
	return true;
}

bool Signal::operator ==(const Signal& other) const
{
	if(id_ == other.id_) {return true;}
	return false;
}

bool Signal::operator ==(const std::string& aName) const
{
	if(id_.find(aName)    != std::string::npos) {return true;}
	if(event_.find(aName) != std::string::npos) {return true;}

	return false;
}

const std::string Signal::id() const
{
	return id_;
}

/// @brief Build a JSON object with data members of Signal object
///
/// @return the built JSON object representing the Signal
json_object* Signal::toJSON() const
{
	json_object* queryJ = nullptr;
	std::vector<std::string> dependsSignalName;
	for (const std::string& src: dependsSigV_ )
	{
		ssize_t sep = src.find_first_of("/");
		if(sep != std::string::npos)
		{
			dependsSignalName.push_back(src.substr(sep+1));
		}
	}
	json_object *nameArrayJ = json_object_new_array();
	for (const std::string& lowSig: dependsSignalName)
	{
		json_object_array_add(nameArrayJ, json_object_new_string(lowSig.c_str()));
	}

	wrap_json_pack(&queryJ, "{ss,so*}",
			"uid", id_.c_str(),
			"getSignalsArgs", getSignalsArgs_);

	if (!event_.empty())
		{json_object_object_add(queryJ, "event", json_object_new_string(event_.c_str()));}

	if (json_object_array_length(nameArrayJ))
		{json_object_object_add(queryJ, "depends", nameArrayJ);}
	else
		{json_object_put(nameArrayJ);}

	if (!unit_.empty()) {json_object_object_add(queryJ, "unit", json_object_new_string(unit_.c_str()));}

	if (frequency_) {json_object_object_add(queryJ, "frequency", json_object_new_double(frequency_));}

	if(timestamp_) {json_object_object_add(queryJ, "timestamp", json_object_new_int64(timestamp_));}

	if (value_.hasBool) {json_object_object_add(queryJ, "value", json_object_new_boolean(value_.boolVal));}
	else if (value_.hasNum) {json_object_object_add(queryJ, "value", json_object_new_double(value_.numVal));}
	else if (value_.hasStr) {json_object_object_add(queryJ, "value", json_object_new_string(value_.strVal.c_str()));}

	return queryJ;
}

/// @brief Initialize signal context if not already done and return it.
///  Signal context is a handle to be use by plugins then they can call
///  some signal object method setting signal values.
///  Also if plugin set a context it retrieve it and initiaze the pluginCtx
///  member then plugin can find a persistent memory area where to hold its
///  value.
///
/// @return a pointer to the signalCtx_ member initialized.
struct signalCBT* Signal::get_context()
{
	if(!signalCtx_.aSignal ||
		!signalCtx_.searchNsetSignalValue ||
		!signalCtx_.setSignalValue)
	{
		signalCtx_.searchNsetSignalValue = searchNsetSignalValueHandle;
		signalCtx_.setSignalValue = setSignalValueHandle;

		signalCtx_.aSignal = (void*)this;

		signalCtx_.pluginCtx = onReceived_ && onReceived_->type == CTL_TYPE_CB ?
			onReceived_->exec.cb.plugin->context:
			nullptr;
	}

	return &signalCtx_;
}

/// @brief Set Signal timestamp and value property when an incoming
/// signal arrived. Called by a plugin because treatment can't be
/// standard as signals sources format could changes. See low-can plugin
/// example.
///
/// @param[in] timestamp - timestamp of occured signal
/// @param[in] value - value of change
void Signal::set(uint64_t timestamp, struct signalValue& value)
{
	uint64_t diff = retention_+1;
	value_ = value;
	timestamp_ = timestamp;
	history_[timestamp_] = value_;

	while(diff > retention_)
	{
		uint64_t first = history_.begin()->first;
		diff = (timestamp_ - first)/MICRO;
		if(diff > retention_)
			{history_.erase(history_.cbegin());}
	}
}

/// @brief Observer method called when a Observable Signal has changes.
///
/// @param[in] Observable - object from which update come from
void Signal::update(Signal* sig)
{
	AFB_NOTICE("Got an update from observed signal %s", sig->id().c_str());
}

/// @brief
///
/// @param[in] eventJ - json_object containing event data to process
///
/// @return 0 if ok, -1 or others if not
void Signal::defaultReceivedCB(json_object *eventJ)
{
	uint64_t ts = 0;
	struct signalValue sv;
	json_object_iterator iter = json_object_iter_begin(eventJ);
	json_object_iterator iterEnd = json_object_iter_end(eventJ);
	while(!json_object_iter_equal(&iter, &iterEnd))
	{
		std::string name = json_object_iter_peek_name(&iter);
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);
		json_object *value = json_object_iter_peek_value(&iter);
		if (name.find("value") || name.find(id_))
		{
			if(json_object_is_type(value, json_type_double))
				{sv = json_object_get_double(value);}
			else if(json_object_is_type(value, json_type_boolean))
				{sv = json_object_get_int(value);}
			else if(json_object_is_type(value, json_type_string))
				{sv = json_object_get_string(value);}
		}
		else if (name.find("timestamp"))
		{
			ts = json_object_is_type(value, json_type_int) ? json_object_get_int64(value):ts;
		}
		json_object_iter_next(&iter);
	}

	if(!sv.hasBool && !sv.hasNum && !sv.hasStr)
	{
		AFB_ERROR("No data found to set signal %s in %s", id_.c_str(), json_object_to_json_string(eventJ));
		return;
	}
	else if(ts == 0)
	{
		struct timespec t_usec;
		if(!::clock_gettime(CLOCK_MONOTONIC, &t_usec))
			ts = (t_usec.tv_nsec / 1000ll) + (t_usec.tv_sec* 1000000ll);
	}

	set(ts, sv);
}

/// @brief Notify observers that there is a change and execute callback defined
/// when signal is received
///
/// @param[in] eventJ - JSON query object to transmit to callback function
///
void Signal::onReceivedCB(json_object *eventJ)
{
	if(onReceived_ && onReceived_->type == CTL_TYPE_LUA)
	{
		json_object_iterator iter = json_object_iter_begin(eventJ);
		json_object_iterator iterEnd = json_object_iter_end(eventJ);
		while(!json_object_iter_equal(&iter, &iterEnd))
		{
			const char *name = ::strdup(json_object_iter_peek_name(&iter));
			json_object *value = json_object_iter_peek_value(&iter);
			if(json_object_is_type(value, json_type_int))
			{
				int64_t newVal = json_object_get_int64(value);
				newVal = newVal > USEC_TIMESTAMP_FLAG ? newVal/MICRO:newVal;
				json_object_object_del(eventJ, name);
				json_object* luaVal = json_object_new_int64(newVal);
				json_object_object_add(eventJ, name, luaVal);
			}
			json_object_iter_next(&iter);
		}
	}

	CtlSourceT source;
	source.uid = id_.c_str();
	source.api  = nullptr; // We use binding v2, no dynamic API.
	source.request = {nullptr, nullptr};
	source.context = (void*)get_context();
	onReceived_ ? ActionExecOne(&source, onReceived_, eventJ) : defaultReceivedCB(eventJ);
	notify();
}

/// @brief Make a Signal observer observes Signals observables
/// set in its observable vector.
///
/// @param[in] composer - bindinApp instance
void Signal::attachToSourceSignals(Composer& composer)
{
	for (const std::string& srcSig: dependsSigV_)
	{
		if(srcSig.find("/") == std::string::npos)
		{
			std::vector<std::shared_ptr<Signal>> observables = composer.searchSignals(srcSig);
			if(observables[0])
			{
				AFB_NOTICE("Attaching %s to %s", id_.c_str(), srcSig.c_str());
				observables[0]->addObserver(this);
				continue;
			}
			AFB_WARNING("Can't attach. Is %s exists ?", srcSig.c_str());
		}
	}
}

/// @brief Make an average over the last X 'seconds'
///
/// @param[in] seconds - period to calculate the average
///
/// @return Average value
double Signal::average(int seconds) const
{
	uint64_t begin = history_.begin()->first;
	uint64_t end = seconds ?
		begin+(seconds*MICRO) :
		history_.rbegin()->first;
	double total = 0.0;
	int nbElt = 0;

	for (const auto& val: history_)
	{
		if(val.first >= end)
			{break;}
		if(val.second.hasNum)
		{
			total += val.second.numVal;
			nbElt++;
		}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			val.second.boolVal,
			val.second.numVal,
			val.second.strVal.c_str());
			break;
		}
	}

	return total / nbElt;
}

/// @brief Find minimum in the recorded values
///
/// @param[in] seconds - period to find the minimum
///
/// @return Minimum value contained in the history
double Signal::minimum(int seconds) const
{
	uint64_t begin = history_.begin()->first;
	uint64_t end = seconds ?
	begin+(seconds*MICRO) :
	history_.rbegin()->first;

	double min = DBL_MAX;
	for (auto& v : history_)
	{
		if(v.first >= end)
			{break;}
		else if(v.second.hasNum && v.second.numVal < min)
			{min = v.second.numVal;}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			v.second.boolVal,
			v.second.numVal,
			v.second.strVal.c_str());
			break;
		}
	}
	return min;
}

/// @brief Find maximum in the recorded values
///
/// @param[in] seconds - period to find the maximum
///
/// @return Maximum value contained in the history
double Signal::maximum(int seconds) const
{
	uint64_t begin = history_.begin()->first;
	uint64_t end = seconds ?
	begin+(seconds*MICRO) :
	history_.rbegin()->first;

	double max = 0.0;
	for (auto& v : history_)
	{
		if(v.first >= end)
		{break;}
		else if(v.second.hasNum && v.second.hasNum > max)
			{max = v.second.numVal;}
		else
		{
			AFB_ERROR("There isn't numerical value to compare with in that signal '%s'. Stored value : bool %d, num %lf, str: %s",
			id_.c_str(),
			v.second.boolVal,
			v.second.numVal,
			v.second.strVal.c_str());
			break;
		}
	}
	return max;
}

/// @brief Return last value recorded
///
/// @return Last value
struct signalValue Signal::last() const
{
	if(history_.empty()) {return signalValue();}
	return history_.rbegin()->second;
}


/// @brief Recursion check to ensure that there is no infinite loop
/// in the Observers/Observables structure.
/// This will check that observer signal is not the same than itself
/// then trigger the check against the following eventuals observers
///
/// @return 0 if no infinite loop detected, -1 if not.
int Signal::initialRecursionCheck()
{
	for (auto& obs: observerList_)
	{
		if(obs == this)
			{return -1;}
		if(static_cast<Signal*>(obs)->recursionCheck(static_cast<Signal*>(obs)))
			{return -1;}
	}
	return 0;
}

/// @brief Inner recursion check. Argument is the Signal id coming
/// from the original Signal that made a recursion check.
///
/// @param[in] origId - name of the origine of the recursion check
///
/// @return 0 if no infinite loop detected, -1 if not.
int Signal::recursionCheck(Signal* parentSig)
{
	for (const auto& obs: observerList_)
	{
		Signal* obsSig = static_cast<Signal*>(obs);
		if(parentSig->id() == static_cast<Signal*>(obsSig)->id())
			{return -1;}
		if(! obsSig->recursionCheck(obsSig))
			{return -1;}
	}
	return 0;
}
