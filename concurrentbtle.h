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

void OpenFileLeft();
void writeFileLeft(qint16 parameter1);
void OpenFileRight();
void writeFileRight(qint16 parameter1);
// void OpenFileCardio();
// void writeFileCardio(double parameter1);
void OpenFileLeftPowerForce();
void OpenFileRightPowerForce();
void powerOutputRight (double power,  double angle);
void powerOutputLeft (double power,  double angle);


class ConcurrentBtle : public QObject
{
    Q_OBJECT

public:
    explicit ConcurrentBtle(QObject *parent = nullptr);
    ~ConcurrentBtle();
    int num_left_data;
    int num_right_data;
    qint16 instantaneousLeftPower;
    qint16 instantaneousRightPower;
    bool newLeftData = false;
    bool newRightData = false;

    vector<double> leftForceVector;
    vector<double> rightForceVector;


signals:

public slots:
    void startSearch();
    void establishConnection();
//    void setupNotificationCardio(QLowEnergyController *device, const QString &name);
    void setupNotificationRight(QLowEnergyController *device, const QString &name);
    void setupNotificationLeft(QLowEnergyController *device, const QString &name);
    void reconnectDevice();
    void getLeftPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void getRightPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void getLeftForce(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);
    void getRightForce(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue);

private:
    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
    QList<QBluetoothAddress> desiredDevices;
    QList<QBluetoothDeviceInfo> foundDevices;
    QTimer *reconnectTimer1; // Timer for reconnection attempts
    QTimer *reconnectTimer2; // Timer for reconnection attempts
//    QTimer *reconnectTimer3; // Timer for reconnection attempts

    QLowEnergyController *device1 = nullptr;
    QLowEnergyController *device2 = nullptr;
//    QLowEnergyController *device3 = nullptr;

    int leftSumPowerVector;
    int rightSumPowerVector;
    std::vector<qint16> leftPowerVector;
    std::vector<qint16> rightPowerVector;

    qint16 averageLeftPower;
    qint16 averageRightPower;
};

#endif // CONCURRENTBTLE_H