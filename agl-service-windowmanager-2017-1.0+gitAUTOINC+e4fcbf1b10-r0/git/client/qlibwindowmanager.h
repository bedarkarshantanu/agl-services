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

#ifndef QLIBWINDOWMANAGER_H
#define QLIBWINDOWMANAGER_H
#include <libwindowmanager.h>
#include <functional>
 #include <QObject>
 #include <QUrl>
 #include <QVariant>
 #include <string>
 #include <vector>

class QLibWindowmanager : public QObject{
Q_OBJECT
public:
    explicit QLibWindowmanager(QObject *parent = nullptr);
    ~QLibWindowmanager();

    QLibWindowmanager(const QLibWindowmanager &) = delete;
    QLibWindowmanager &operator=(const QLibWindowmanager &) = delete;

public:
    using handler_fun = std::function<void(const char *)>;

    enum QEventType {
       Event_Active = 1,
       Event_Inactive,

       Event_Visible,
       Event_Invisible,

       Event_SyncDraw,
       Event_FlushDraw,
    };

    static QLibWindowmanager &instance();

    int init(int port, char const *token);

    // WM API
    int requestSurface(const char *label);
    int activateSurface(const char *label);
    int deactivateSurface(const char *label);
    int endDraw(const char *label);
    void set_event_handler(enum QEventType et, handler_fun f);

public slots:
    void slotActivateSurface();
    
private:
    LibWindowmanager* wm;
    std::string applabel;
    std::vector<int> surfaceIDs; 
};
#endif // LIBWINDOWMANAGER_H
