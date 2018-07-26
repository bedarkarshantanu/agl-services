/*
 * Copyright (c) 2017 TOYOTA MOTOR CORPORATION
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "qlibwindowmanager.h"
#include <unistd.h>

int QLibWindowmanager::init(int port, char const *token) {
    return this->wm->init(port, token);
}

int QLibWindowmanager::requestSurface(const char *label) {
    applabel = label;
    return this->wm->requestSurface(label);
}

int QLibWindowmanager::activateSurface(const char *label) {
    return this->wm->activateSurface(label);
}

int QLibWindowmanager::deactivateSurface(const char *label) {
    return this->wm->deactivateSurface(label);
}

int QLibWindowmanager::endDraw(const char *label) { return this->wm->endDraw(label); }

void QLibWindowmanager::set_event_handler(enum QEventType et,
                                  std::function<void(char const *label)> f) {
    LibWindowmanager::EventType wet = (LibWindowmanager::EventType)et;
    return this->wm->set_event_handler(wet, std::move(f));
}

void QLibWindowmanager::slotActivateSurface(){
    qDebug("%s",__FUNCTION__);
    this->activateSurface(applabel.c_str());
}

QLibWindowmanager::QLibWindowmanager(QObject *parent) 
    :QObject(parent) 
{
    wm = new LibWindowmanager();
}

QLibWindowmanager::~QLibWindowmanager() { }
