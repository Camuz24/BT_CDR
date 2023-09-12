#ifndef CONCURRENTBTLE_H
#define CONCURRENTBTLE_H

#include <QObject>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <QTimer>
#include <iostream>
#include <QCoreApplication>
#include "shared_memory.h"
#include "singletonSM.h"

using namespace std;


class ConcurrentBtle : public QObject
{
    Q_OBJECT

public:
    explicit ConcurrentBtle(QObject *parent = nullptr);

signals:

public slots:
    void startSearch();
    void establishConnection();
    void setupNotificationCardio(QLowEnergyController *device, const QString &name);
    void reconnectDevice3();
    void handleDeviceDisconnection();

private:
    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
    QList<QBluetoothAddress> desiredDevices;
    QList<QBluetoothDeviceInfo> foundDevices;
    QTimer *reconnectTimer; // Timer for reconnection attempts
    QLowEnergyController *device3 = nullptr;
};

#endif