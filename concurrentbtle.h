#ifndef CONCURRENTBTLE_H
#define CONCURRENTBTLE_H

#include <QObject>
#include <QTimer>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <iostream>
#include <QCoreApplication>
#include "shared_memory.h"
#include "singletonSM.h"

using namespace std;

void OpenFileCardio();
void writeFileCardio(double parameter1);

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

//    void handleDeviceDisconnection();
    void reconnectDevice();

private:
    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
    QList<QBluetoothAddress> desiredDevices;
    QList<QBluetoothDeviceInfo> foundDevices;
    QTimer *reconnectTimer3; // Timer for reconnection attempts

    QLowEnergyController *device3 = nullptr;

};

#endif // CONCURRENTBTLE_H