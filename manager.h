#ifndef MANAGER_H
#define MANAGER_H

#include <QtCore/qobject.h>
#include "shared_memory.h"
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
    

public slots:
    void writeOnSM(const QString &sender, const QString &message);

signals:
    void sendToClient(const QString &message);

private:
    int cadence;
    shared_memory shmem;

};

#endif