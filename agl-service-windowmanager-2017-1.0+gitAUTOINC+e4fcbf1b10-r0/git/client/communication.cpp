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

#include "communication.h"

#include <QGuiApplication>
#include <QDebug>

communication::communication(QObject *parent) : QObject(parent)
{
    this->quit = false;
}

communication::~communication()
{
}

void communication::setWidth(const unsigned int &w)
{
    this->width = w;
    emit widthChanged();
}

void communication::setHeight(const unsigned int &h)
{
    this->height = h;
    emit heightChanged();
}

void communication::setColor(const QString &c)
{
    this->color = c;
    emit colorChanged();
}

void communication::setAppName(const QString &a)
{
    this->appName = a;
    emit appNameChanged();
}

void communication::setQuit(const bool &q)
{
    this->quit = q;
    emit quitChanged();
    if(q)
        exit(EXIT_SUCCESS);
}

unsigned int communication::getWidth() const
{
    return this->width;
}

unsigned int communication::getHeight() const
{
    return this->height;
}

QString communication::getColor() const
{
    return this->color;
}

QString communication::getAppName() const
{
    return this->appName;
}

bool communication::getQuit() const
{
    return this->quit;
}
