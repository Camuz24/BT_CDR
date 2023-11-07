#ifndef MANAGER_H
#define MANAGER_H

#include <QtCore/qobject.h>
#include "shared_memory.h"
#include <string>
#include <vector>
#include <signal.h>
#include "singletonSM.h"
using std::string;
// #pragma once
QT_USE_NAMESPACE

class manager : public QObject
{
    Q_OBJECT

public:
    manager();
    ~manager();
    void threadReadFromSM();
    void startThread();
    bool stopThread;
    int pedals;
    int trike;
    bool first_time;
    

public slots:
    void writeOnSM(const QString &sender, const QString &message);

signals:
    void sendToClient(const QString &message);

private:
    string buildXMLMessage(const std::vector<string>& types, const std::vector<string>& payloads);
    bool stopSend;
    string getMotivation();
};

#endif