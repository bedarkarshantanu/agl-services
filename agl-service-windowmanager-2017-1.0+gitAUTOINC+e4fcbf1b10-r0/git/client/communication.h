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

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QObject>

class communication : public QObject
{
    Q_OBJECT

    Q_PROPERTY(unsigned int width WRITE setWidth READ getWidth NOTIFY widthChanged)
    Q_PROPERTY(unsigned int height WRITE setHeight READ getHeight NOTIFY heightChanged)
    Q_PROPERTY(QString color READ getColor WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString appName READ getAppName WRITE setAppName NOTIFY appNameChanged)
    Q_PROPERTY(bool quit READ getQuit WRITE setQuit NOTIFY quitChanged)

public:
    explicit communication(QObject *parent = 0);
    virtual ~communication();

public slots:
    void setWidth(const unsigned int &);
    void setHeight(const unsigned int &);
    void setColor(const QString&);
    void setAppName(const QString&);
    void setQuit(const bool&);

    unsigned int getWidth() const;
    unsigned int getHeight() const;
    QString getColor() const;
    QString getAppName() const;
    bool getQuit() const;

signals:
    void widthChanged();
    void heightChanged();
    void colorChanged();
    void appNameChanged();
    void quitChanged();

private:
    unsigned int width;
    unsigned int height;
    QString color;
    QString appName;
    bool quit;
};

#endif // COMMUNICATION_H
