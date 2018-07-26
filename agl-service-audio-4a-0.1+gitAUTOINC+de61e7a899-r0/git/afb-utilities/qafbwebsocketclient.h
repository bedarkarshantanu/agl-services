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

#ifndef QAFBWEBSOCKETCLIENT_H
#define QAFBWEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonValue>
#include <functional>

/*!
 * \brief A WebSocket client to an Application Framework Binder.
 */
class QAfbWebsocketClient
    : public QObject
{
    Q_OBJECT

public:
    using closure_t = std::function<void(bool, const QJsonValue&)>;

    explicit QAfbWebsocketClient(QObject* parent = nullptr);

    QAbstractSocket::SocketError error() const;
    QString errorString() const;
    bool isValid() const;

    void call(const QString& api, const QString& verb, const QJsonValue& arg = QJsonValue(), closure_t closure = nullptr);

public slots:
    void open(const QUrl& u);
    void close();
    void sendTextMessage(QString msg);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError e);
    void onSocketTextReceived(QString msg);

signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError);
    void textReceived(QString msg);
    void textSent(QString msg);
    void eventReceived(QString eventName, const QJsonValue& data);

private:
    int m_nextCallId;
    QWebSocket m_socket;
    QMap<QString, closure_t> m_closures;
};

#endif // QAFBWEBSOCKETCLIENT_H
