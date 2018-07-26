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

#include "clientApp.hpp"

clientAppCtx::clientAppCtx(const char* uuid)
: uuid_{uuid},
  event_{nullptr,nullptr}
{}

void clientAppCtx::update(Signal* sig)
{
	json_object* sigJ = sig->toJSON();
	if(afb_event_push(event_, sigJ) == 0)
		{sig->delObserver(this);}
	return;
}

void clientAppCtx::appendSignals(std::vector<std::shared_ptr<Signal>>& sigV)
{
	bool set = false;
	// Clean up already subscribed signals to avoid duplicata
	for (std::vector<std::shared_ptr<Signal>>::const_iterator it = sigV.cbegin();
	it != sigV.cend(); ++it)
	{
		for (auto& ctxSig: subscribedSignals_)
			{if(*it == ctxSig) {set = true;}}
		if (set)
		{
			set = false;
			sigV.erase(it);
			continue;
		}
		std::shared_ptr<Signal> sig = *it;
		sig->addObserver(this);
	}

	subscribedSignals_.insert(subscribedSignals_.end(), sigV.begin(), sigV.end());
}

void clientAppCtx::subtractSignals(std::vector<std::shared_ptr<Signal>>& sigV)
{
	// Clean up already subscribed signals to avoid duplicata
	for (std::vector<std::shared_ptr<Signal>>::const_iterator it = sigV.cbegin();
	it != sigV.cend(); ++it)
	{
		for (auto ctxSig = subscribedSignals_.cbegin(); ctxSig != subscribedSignals_.cend();ctxSig++)
		{
			if(*it == *ctxSig)
			{
				subscribedSignals_.erase(ctxSig);
				break;
			}
		}
		std::shared_ptr<Signal> sig = *it;
		sig->delObserver(this);
		AFB_NOTICE("Signal %s delete from subscription", sig->id().c_str());
	}
}

int clientAppCtx::makeSubscription(struct afb_req request)
{
	event_ = afb_event_is_valid(event_) ?
		event_ : afb_daemon_make_event(uuid_.c_str());
	return afb_req_subscribe(request, event_);
}

int clientAppCtx::makeUnsubscription(struct afb_req request)
{
	if(subscribedSignals_.empty())
	{
		AFB_NOTICE("No more signals subscribed, releasing.");
		return afb_event_is_valid(event_) ?
			afb_req_unsubscribe(request, event_) : -1;
	}
	return 0;
}
