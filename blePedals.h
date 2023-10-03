#ifndef BLEPEDALS_H
#define BLEPEDALS_H

#pragma once

#include <QObject>
#include <QTimer>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qlowenergycontroller.h>
#include <iostream>
#include <QCoreApplication>
#include "shared_memory.h"
#include "singletonSM.h"

using namespace std;

void OpenFileLeft();
void writeFileLeft(qint16 parameter1);
//void OpenFileRight();
//void writeFileRight(qint16 parameter1);

class blePedals : public QObject
{
    Q_OBJECT

public:
    explicit blePedals(QObject *parent = nullptr);
    int num_left_data;
//    int num_right_data;
    qint16 instantaneousLeftPower;
//    qint16 instantaneousRightPower;

signals:

public slots:
    void startSearch();
    void establishConnection();
//    void setupNotificationRight(QLowEnergyController *device, const QString &name);
    void setupNotificationLeft(QLowEnergyController *device, const QString &name);
//    void handleDeviceDisconnection();
    void reconnectDevice();
    void getLeftPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
//    void getRightPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
    QList<QBluetoothAddress> desiredDevices;
    QList<QBluetoothDeviceInfo> foundDevices;
    QTimer *reconnectTimer1; // Timer for reconnection attempts
//    QTimer *reconnectTimer2; // Timer for reconnection attempts

    QLowEnergyController *device1 = nullptr;
//    QLowEnergyController *device2 = nullptr;

    int leftSumPowerVector;
//    int rightSumPowerVector;
    std::vector<qint16> leftPowerVector;
//    std::vector<qint16> rightPowerVector;

    qint16 averageLeftPower;
//    qint16 averageRightPower;
};

#endif