/*
 * Copyright (C) 2018 Iot.Bzh
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

#include "qafbwebsocketclient.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

/*!
 * \brief Default constructor.
 * \param parent Parent object.
 */
QAfbWebsocketClient::QAfbWebsocketClient(QObject* parent)
    : QObject{parent}
    , m_nextCallId{0}
{
    connect(&m_socket, SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(&m_socket, SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));
    connect(&m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    connect(&m_socket, SIGNAL(textMessageReceived(QString)), this, SLOT(onSocketTextReceived(QString)));
}

/*!
 * \brief Get last error code.
 * \return Return the last error code.
 */
QAbstractSocket::SocketError QAfbWebsocketClient::error() const
{
    return m_socket.error();
}

/*!
 * \brief Get last error as a string.
 * \return Return the last error as a string.
 */
QString QAfbWebsocketClient::errorString() const
{
    return m_socket.errorString();
}

/*!
 * \brief Check if connection is ready or not.
 * \return Return \c true if the connected is ready to read and write, \c false otherwise.
 */
bool QAfbWebsocketClient::isValid() const
{
    return m_socket.isValid();
}
/*!
 * \brief Open the connection.
 * \param u Url to connect to.
 */
void QAfbWebsocketClient::open(const QUrl& u)
{
    m_socket.open(u);
}

/*!
 * \brief Close the connection.
 */
void QAfbWebsocketClient::close()
{
    m_socket.close();
}

/*!
 * \brief Call an api's verb with an argument.
 * \param api Api to call.
 * \param verb Verb to call.
 * \param arg Argument to pass.
 */
void QAfbWebsocketClient::call(const QString& api, const QString& verb, const QJsonValue& arg, closure_t closure)
{
    QString callId = QString::number(m_nextCallId);
    m_closures[callId] = closure;

    QJsonArray msg;
    msg.append(2); // Call
    msg.append(callId);
    msg.append(api + "/" + verb);
    msg.append(arg);

    m_nextCallId++;

    QJsonDocument value;
    value.setArray(msg);

    sendTextMessage(value.toJson(QJsonDocument::Compact));
}

/*!
 * \brief Send a text message over the websocket.
 * \param msg Message to send.
 * This is use for test only, you should not use this method as
 * it sent text as-is, so you have to follow the binder's
 * protocol by your self.
 */
void QAfbWebsocketClient::sendTextMessage(QString msg)
{
    m_socket.sendTextMessage(msg);
    qDebug() << "WebSocket Text Sent: " << msg;
    emit textSent(msg);
}

/*!
 * \brief Called when socket signals to be connected.
 */
void QAfbWebsocketClient::onSocketConnected()
{
    emit connected();
}

/*!
 * \brief Called when socket signals to be disconnected.
 */
void QAfbWebsocketClient::onSocketDisconnected()
{
    emit disconnected();
}

/*!
 * \brief Called when socket signals an error.
 * \param e Error code.
 */
void QAfbWebsocketClient::onSocketError(QAbstractSocket::SocketError e)
{
    emit error(e);
}

/*!
 * \brief Called when socket signals a received text.
 * \param msg Message received.
 */
void QAfbWebsocketClient::onSocketTextReceived(QString msg)
{
    emit textReceived(msg);
    qDebug() << "WebSocket Text Received: " << msg;

    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8());
    QJsonArray arr = doc.array();

    switch(arr[0].toInt())
    {
    case 3: // RetOK
    case 4: // RetErr
    {
        auto it = m_closures.find(arr[1].toString());
        if (it != m_closures.end())
        {
            closure_t closure = *it;
            m_closures.erase(it);
            closure(arr[0].toInt() == 3, arr[2]);
        }
        break;
    }
    case 5: // Events
        emit eventReceived(arr[1].toString(), arr[2].toObject()["data"]);
        break;
    }
}
