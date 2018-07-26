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

#include "signal-composer.hpp"

class clientAppCtx: public Observer<Signal>
{
private:
	std::string uuid_;
	std::vector<std::shared_ptr<Signal>> subscribedSignals_;
	struct afb_event event_;
public:
	explicit clientAppCtx(const char* uuid);

	void update(Signal* sig);
	void appendSignals(std::vector<std::shared_ptr<Signal>>& sigV);
	void subtractSignals(std::vector<std::shared_ptr<Signal>>& sigV);
	int makeSubscription(struct afb_req request);
	int makeUnsubscription(struct afb_req request);
};
