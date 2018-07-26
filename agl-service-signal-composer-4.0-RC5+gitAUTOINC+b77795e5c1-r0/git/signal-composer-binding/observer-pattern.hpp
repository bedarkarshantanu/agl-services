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

/* This is a classic observer design pattern a bit enhanced to be
 able to pull and push info in 2 ways */

#include <list>
#include <mutex>
#include <algorithm>

template<class T>
class Observable;

template<class T>
class Observer
{
private:
	std::mutex observableListMutex_;

protected:
	virtual ~Observer()
	{
		std::lock_guard<std::mutex> lock(observableListMutex_);
		for(const auto& sig: observableList_)
		{
				sig->delObserver(this);
		}
	}

	std::list<Observable<T>*> observableList_;
	typedef typename std::list<Observable<T>*>::iterator iterator_;
	typedef typename std::list<Observable<T>*>::const_iterator const_iterator_;

public:
	virtual void update(T* observable) = 0;

	void addObservable(Observable<T>* observable)
	{
		std::lock_guard<std::mutex> lock(observableListMutex_);
		for (auto& obs : observableList_)
		{
			if (obs == observable)
				{return;}
		}

		observableList_.push_back(observable);
	}

	void delObservable(Observable<T>* observable)
	{
		std::lock_guard<std::mutex> lock(observableListMutex_);
		const_iterator_ it = std::find(observableList_.cbegin(), observableList_.cend(), observable);
		if(it != observableList_.cend())
			{observableList_.erase(it);}
	}
};

template <class T>
class Observable
{
public:
	void addObserver(Observer<T>* observer)
	{
		std::lock_guard<std::mutex> lock(observerListMutex_);
		observerList_.push_back(observer);
		observer->addObservable(this);
	}

	void delObserver(Observer<T>* observer)
	{
		std::lock_guard<std::mutex> lock(observerListMutex_);
		const_iterator_ it = find(observerList_.cbegin(), observerList_.cend(), observer);
		if(it != observerList_.cend())
			{observerList_.erase(it);}
	}

	virtual int initialRecursionCheck() = 0;
	virtual int recursionCheck(T* obs) = 0;

	virtual ~Observable()
	{
		std::lock_guard<std::mutex> lock(observerListMutex_);
		for(const auto& obs: observerList_)
		{
			obs->delObservable(this);
		}
	}

protected:
	std::list<Observer<T>*> observerList_;
	typedef typename std::list<Observer<T>*>::iterator iterator_;
	typedef typename std::list<Observer<T>*>::const_iterator const_iterator_;

	void notify()
	{
		std::lock_guard<std::mutex> lock(observerListMutex_);
		for(const auto& obs: observerList_)
		{
			obs->update(static_cast<T*>(this));
		}
	}

private:
	std::mutex observerListMutex_;
};
