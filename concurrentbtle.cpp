#include "concurrentbtle.h"
#include <QDebug>
#include <QTimer>
#include <iostream>
#include <fstream>
#include <QtCore/QtAlgorithms>
#include <QtCore>
#include <math.h>

bool ok_cardio = false;

void write_heart_rate(double hr_value){
    // std::cout << "writing HR" << std::endl;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->heart_rate=hr_value;
}


// code to initialize btle function
ConcurrentBtle::ConcurrentBtle(QObject *parent) : QObject(parent)
{
    desiredDevices << QBluetoothAddress(QStringLiteral("F0:87:17:F9:0F:03")); /*Polar H10 8E5AB228 F0:87:17:F9:0F:03*/

    agent = new QBluetoothDeviceDiscoveryAgent(this);
    agent->setLowEnergyDiscoveryTimeout(20000);
    connect(agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, [this](const QBluetoothDeviceInfo &info){
        // qDebug() << "Found device: " << info.address();

        foundDevices.append(info);
    });


    connect(agent,
            QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
            this, [](QBluetoothDeviceDiscoveryAgent::Error error){
        qDebug() << "Discovery error" << error;
    });

    connect(agent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, [this](){
        qDebug() << "Discovery finished";

        for (auto desiredDevice: qAsConst(desiredDevices)) {

            bool found = false;

            for (auto foundDevice: foundDevices) {
                if (foundDevice.address() == desiredDevice) {
                    found = true;
                    qDebug() << "Found required device(s)" << desiredDevices;
                    establishConnection();
                    break;
                }
            }

            if (!found) {
                qDebug() << "Cannot find" << desiredDevice;
                startSearch();
            }
        }
    });
    
    reconnectTimer = new QTimer(this);
    reconnectTimer->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice3);
    startSearch();
}

void ConcurrentBtle::startSearch()
{
    if (agent->isActive())
        agent->stop();

    foundDevices.clear();

    agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}


void ConcurrentBtle::establishConnection()
{
    if (!device3) {
        std::cout << "establishing connection" << std::endl;
        
        if (desiredDevices.at(0)==QBluetoothAddress(QStringLiteral("F0:87:17:F9:0F:03")))
        device3 = new QLowEnergyController(desiredDevices.at(0));
        
        
        device3->setParent(this);
        connect(device3, &QLowEnergyController::connected, this, [&](){
            ok_cardio=1;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            qDebug() << "*********** Device 3 Polar H10 connected" << device3->remoteAddress();
            device3->discoverServices();
            reconnectTimer->stop();
        });

        connect(device3, &QLowEnergyController::disconnected, this, [&](){
            handleDeviceDisconnection();
        });

        connect(device3, &QLowEnergyController::discoveryFinished, this, [&](){
           qDebug() <<  "*********** Device 3 discovery finished";
           setupNotificationCardio(device3, QStringLiteral("Device 3"));
        });

        device3->connectToDevice();
    }
}

void ConcurrentBtle::handleDeviceDisconnection()
{
    qDebug() << "Device 3 disconnected";
    ok_cardio = false;
    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();
    shmem->data->check_cardio = ok_cardio;

    // Start the reconnect timer after a disconnection event
    reconnectTimer->start();
}

void ConcurrentBtle::reconnectDevice3()
{
    qDebug() << "Attempting to reconnect Device 3...";
    if (device3)
    {
        device3->disconnectFromDevice(); // Ensure the device is disconnected first
        device3->connectToDevice();
    }
    else
    {
        // If device3 is not initialized, create a new instance and connect
        device3 = new QLowEnergyController(desiredDevices.at(0));
        connect(device3, &QLowEnergyController::connected, this, [&](){
            ok_cardio = true;
            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            qDebug() << "*********** Device 3 Polar H10 connected" << device3->remoteAddress();
            device3->discoverServices();
            reconnectTimer->stop();
        });

        connect(device3, &QLowEnergyController::disconnected, this, &ConcurrentBtle::handleDeviceDisconnection);

        device3->connectToDevice();
    }
}


void ConcurrentBtle::setupNotificationCardio(QLowEnergyController *device, const QString &name)
{

    if (!device)
        return;

    QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::HeartRate);
    if (!service) {
        qDebug() << "***********" << name << "cardio service not found";
        return;
    }

    qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

    connect(service, &QLowEnergyService::stateChanged,
            this, [name, service](QLowEnergyService::ServiceState s){
        if (s == QLowEnergyService::ServiceDiscovered) {
            qDebug() << "***********" << name << "cardio service discovered" << service->serviceUuid();
            const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::HeartRateMeasurement);

            if (!tempData.isValid()) {
                qDebug() << "***********" << name << "cardio char not valid";
                return;
            }

            const QLowEnergyDescriptor notification = tempData.descriptor(
                        QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));
            if (!notification.isValid()) {
                qDebug() << "***********" << name << "cardio notification not valid";
                return;
            }

            service->writeDescriptor(notification, QByteArray::fromHex("0100"));

        }
    });


    connect(service, &QLowEnergyService::characteristicChanged,
            this, [name](const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue) {

        int a = (newValue.size());
        // qDebug() << "Size:" << a;

        const char *data = newValue.constData();

        quint8 flags = data[0];

        //HR 8bit
        quint8 *heartrate= (quint8 *) &data[1];
        // qDebug() << "HR value" << name << *heartrate <<"bpm" ;
        double hr_value=*heartrate;
        write_heart_rate(hr_value);

    });
    service->discoverDetails();
}

