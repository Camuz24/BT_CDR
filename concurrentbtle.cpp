#include "concurrentbtle.h"
#include <QDebug>
#include <QTimer>
#include <QtEndian>
#include <iostream>
#include <fstream>
#include <QtCore/QtAlgorithms>
#include <QtCore>
#include <math.h>
#include <vector>

ofstream CSVfileCardio; //dato grezzo cardio

// create a name for the file output
std::string filenameCardio = "FileCardio.csv";

bool ok_cardio = 0;
int hr_sconnesso = 0;

bool disconnected3 = true;

void write_heart_rate(double hr_value){
    // std::cout << "writing HR" << std::endl;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->heart_rate = hr_value;
}

// code to initialize btle function
ConcurrentBtle::ConcurrentBtle(QObject *parent) : QObject(parent)
{
    desiredDevices << QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:FA")); /*Polar H10 8E5AB228*/

    agent = new QBluetoothDeviceDiscoveryAgent(this);
    agent->setLowEnergyDiscoveryTimeout(8000);
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
        // add a boolean to check connection with shared memory

        for (auto desiredDevice: qAsConst(desiredDevices)) {

            bool found = false;

            for (auto foundDevice: foundDevices) {
                if (foundDevice.address() == desiredDevice) {
                    // desiredDevicesFound.append(foundDevice.address());
                    found = true;
                    establishConnection();
                    //break;
                }
            }

            if (!found) {
                qDebug() << "Cannot find" << desiredDevice;
                startSearch();
                break;
            }
        }

    });

    reconnectTimer3 = new QTimer(this);
    reconnectTimer3->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer3, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice);

    startSearch();

    OpenFileCardio();
}

void ConcurrentBtle::startSearch()
{
    if (agent->isActive())
        agent->stop();

    foundDevices.clear();
    //desiredDevicesFound.clear();

    agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}


void ConcurrentBtle::establishConnection()
{

    if (!device3) {
        std::cout << "establishing connection device 3" << std::endl;

        for (int i=0;i<1;i++) {
            if (desiredDevices.at(0)==QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:FA")))
            device3 = new QLowEnergyController(desiredDevices.at(0));
        }
        
        device3->setParent(this);
        connect(device3, &QLowEnergyController::connected, this, [&](){
            ok_cardio = 1;
            disconnected3 = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            qDebug() << "*********** Device 3 Polar H10 connected" << device3->remoteAddress();
            device3->discoverServices();
            reconnectTimer3->stop();
        });

        connect(device3, &QLowEnergyController::disconnected, this, [&](){
            ok_cardio = 0;
            disconnected3 = true;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            qDebug() << "Device 3 Disconnected";

            reconnectTimer3->start();
        });

        connect(device3, &QLowEnergyController::discoveryFinished, this, [&](){
           qDebug() <<  "*********** Device 3 discovery finished";
           setupNotificationCardio(device3, QStringLiteral("Device 3"));
        });

        device3->connectToDevice();
    }
}

void ConcurrentBtle::reconnectDevice()
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
        device3 = new QLowEnergyController(desiredDevices.at(2));
        connect(device3, &QLowEnergyController::connected, this, [&](){
            ok_cardio = true;
            disconnected3 = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            qDebug() << "*********** Device 3 Polar connected" << device3->remoteAddress();
            device3->discoverServices();
            reconnectTimer3->stop();

        });

        connect(device3, &QLowEnergyController::disconnected, this, [&](){
            qDebug() << "Device 3 disconnected";
            ok_cardio = false;
            disconnected3 = true;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_cardio = ok_cardio;

            reconnectTimer3->start();
        });

        device3->connectToDevice();
    }
}

void OpenFileCardio()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameCardio = strftime (buffer,80,"/home/pi/Desktop/FilePedaliCamilla/CardioAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileCardio.open(buffer);
    // write the file headers
    if(CSVfileCardio.is_open())
    {
        CSVfileCardio << std::endl << "Heart Rate" << std::endl;
    }
    else if (!CSVfileCardio.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameCardio << std::endl;
        return;
    }
}

void writeFileCardio(double parameter1){
    // clock_gettime ( CLOCK_MONOTONIC, &timeLoop);
    CSVfileCardio << std::endl << parameter1;

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
        qDebug() << "HR value" << name << *heartrate <<"bpm" ;
        double hr_value = *heartrate;
        write_heart_rate(hr_value);
        writeFileCardio(hr_value);

    });
   service->discoverDetails();
}

