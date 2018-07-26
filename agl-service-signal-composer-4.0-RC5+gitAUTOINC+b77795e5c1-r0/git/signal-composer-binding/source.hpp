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
#pragma once

#include <memory>
#include "signal.hpp"

struct signalsDefault {
	CtlActionT* onReceived;
	int retention;
};

class SourceAPI {
private:
	std::string uid_;
	std::string api_;
	std::string info_;
	CtlActionT* init_;
	CtlActionT* getSignals_;
	// Parameters inherited by source's signals if none defined for it
	struct signalsDefault signalsDefault_;

	std::map<std::string, std::shared_ptr<Signal>> newSignalsM_;
	std::map<std::string, std::shared_ptr<Signal>> signalsM_;

public:
	SourceAPI();
	SourceAPI(const std::string& uid_, const std::string& api, const std::string& info, CtlActionT* init, CtlActionT* getSignal, CtlActionT* onReceived, int retention);

	bool operator==(const SourceAPI& other) const;
	bool operator==(const std::string& aName) const;

	void init();
	std::string api() const;
	const struct signalsDefault& signalsDefault() const;
	void addSignal(const std::string& id, const std::string& event, std::vector<std::string>& sources, int retention, const std::string& unit, double frequency, CtlActionT* onReceived, json_object* getSignalsArgs);

	void initSignals();
	std::vector<std::shared_ptr<Signal>> getSignals() const;
	std::vector<std::shared_ptr<Signal>> searchSignals(const std::string& name);

	void makeSubscription();
};
