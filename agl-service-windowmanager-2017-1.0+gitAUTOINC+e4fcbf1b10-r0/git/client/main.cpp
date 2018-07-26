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


#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>
#include "communication.h"

#include "qlibwindowmanager.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    qDebug() << QCoreApplication::arguments();

    if (QCoreApplication::arguments().count() < 5) {
        qWarning() << "Wrong parameters specified for the application. "
                      "Please restart with correct parameters:"
                      "width, height, name, color [port] [token]:\n\n"
                      "/usr/bin/WindowManagerSampleApp/"
                      "qmlWindowManagerSampleApp width height name color\n";
        exit(EXIT_FAILURE);
    }

    QString label = QCoreApplication::arguments().at(3);

    QLibWindowmanager* qwm = new QLibWindowmanager();

    QString token = "wm";
    int port = 1700;
    if(QCoreApplication::arguments().count() == 7){
        bool ok;
        port = QCoreApplication::arguments().at(5).toInt(&ok);
        if(ok == false){
            port = 1700;
        }
        else{
            token = QCoreApplication::arguments().at(6);
        }
    }
    const char* ctoken = token.toLatin1().data();
    if (qwm->init(port, ctoken) != 0) {
        exit(EXIT_FAILURE);
    }

    if (qwm->requestSurface(
            QCoreApplication::arguments().at(3).toLatin1().data()) != 0) {
        exit(EXIT_FAILURE);
    }

    qwm->set_event_handler(QLibWindowmanager::Event_SyncDraw, [qwm](char const *label) {
        fprintf(stderr, "Surface %s got syncDraw!\n", label);
        qwm->endDraw(label);
    });
    qwm->set_event_handler(QLibWindowmanager::Event_Active, [](char const *label) {
        fprintf(stderr, "Surface %s got activated!\n", label);
    });
    qwm->set_event_handler(QLibWindowmanager::Event_Visible, [](char const *label) {
        fprintf(stderr, "Surface %s got visible!\n", label);
    });
    qwm->set_event_handler(QLibWindowmanager::Event_FlushDraw, [](char const *label) {
        fprintf(stderr, "Surface %s got flushDraw!\n", label);
    });

    communication comm;
    comm.setWidth(QCoreApplication::arguments().at(1).toUInt());
    comm.setHeight(QCoreApplication::arguments().at(2).toUInt());
    comm.setAppName(label);
    comm.setColor(QCoreApplication::arguments().at(4));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("COMM", &comm);

    engine.load(
        QUrl(QStringLiteral("qrc:/extra/WindowManagerSampleApp.qml")));
    QObject *root = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>(root);
    QObject::connect(root, SIGNAL(frameSwapped()), qwm, SLOT(slotActivateSurface()));

    return app.exec();
}
