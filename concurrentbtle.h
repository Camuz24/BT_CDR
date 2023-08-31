#ifndef CONCURRENTBTLE_H
#define CONCURRENTBTLE_H

#include <QObject>
#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
#include <QtBluetooth/qlowenergycontroller.h>
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

private:
    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
    QList<QBluetoothAddress> desiredDevices;
    QList<QBluetoothDeviceInfo> foundDevices;

    QLowEnergyController *device1 = nullptr;
    QLowEnergyController *device2 = nullptr;
    QLowEnergyController *device3 = nullptr;

    
};

#endif // CONCURRENTBTLE_H


//#ifndef CONCURRENTBTLE_H
//#define CONCURRENTBTLE_H

//#include <QObject>
//#include <QtBluetooth/qbluetoothdevicediscoveryagent.h>
//#include <QtBluetooth/qlowenergycontroller.h>
//#include <iostream>
//#include <QCoreApplication>
//#include "shared_memory.h"
//#include "temp.h"

//using namespace std;


//void OpenFileLeft();
//void writeFileLeft(float parameter1, double parameter2, double parameter3, double parameter4, double parameter5);
//void OpenFileRight();
//void writeFileRight(float parameter1, double parameter2, double parameter3, double parameter4, double parameter5);
//void OpenFileCardio();
//void writeFileCardio(double parameter1);
////void OpenFilePO();
////void writeFilePO(double parameter1);
//void poweroutputL(double tf, double anglevel, double angle);
//void poweroutputR(double tf, double anglevel, double angle);




//class ConcurrentBtle : public QObject
//{
//    Q_OBJECT
//public:
//    explicit ConcurrentBtle(QObject *parent = nullptr);


//    void init_sm();


//signals:

//public slots:
//    void startSearch();
//    void establishConnection();
//    void setupNotificationLeft(QLowEnergyController *device, const QString &name);
//    void setupNotificationRight(QLowEnergyController *device, const QString &name);
//    void setupNotificationCardio(QLowEnergyController *device, const QString &name);

//private:
//    QBluetoothDeviceDiscoveryAgent *agent = nullptr;
//    QList<QBluetoothAddress> desiredDevices;
//    QList<QBluetoothDeviceInfo> foundDevices;

//    QLowEnergyController *device1 = nullptr;
//    QLowEnergyController *device2 = nullptr;
//    QLowEnergyController *device3 = nullptr;
//};


//class manager{

//public:
//  double temp_angle = 0.0 ;



//};

//#endif // CONCURRENTBTLE_H
