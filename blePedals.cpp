#include "blePedals.h"
#include <QDebug>
#include <QTimer>
#include <QtEndian>
#include <iostream>
#include <fstream>
#include <QtCore/QtAlgorithms>
#include <QtCore>
#include <math.h>
#include <vector>

#define NUM_CYCLES 5

ofstream CSVfileLeft; //dato grezzo pedale sinistro ed efficiency
//ofstream CSVfileRight; //dato grezzo pedale destro ed efficiency

// create a name for the file output
std::string filenameLeft = "FileLeft.csv";
//std::string filenameRight = "FileRight.csv";

bool ok_pleft = 0;
//bool ok_pright = 0;

bool disconnected1 = true;
//bool disconnected2 = true;

void write_left_power(qint16 inst_left_power){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->power_left = inst_left_power;
}
/* void write_right_power(qint16 inst_right_power){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->power_right = inst_right_power;
} */

// code to initialize btle function
blePedals::blePedals(QObject *parent) : QObject(parent)
{
    desiredDevices << QBluetoothAddress(QStringLiteral("C6:21:8B:A7:24:5F")); /*SRM_XP_L_1818*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("F6:D0:29:C5:60:4C")); /*SRM_XP_L_2623*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("ED:86:C3:29:8A:05")); /*SRM_XP_R_1968*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("D5:5E:63:D1:CE:BF")); /*SRM_XP_R_2971*/

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

    reconnectTimer1 = new QTimer(this);
    reconnectTimer1->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer1, &QTimer::timeout, this, &blePedals::reconnectDevice);

/*     reconnectTimer2 = new QTimer(this);
    reconnectTimer2->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer2, &QTimer::timeout, this, &blePedals::reconnectDevice); */

    startSearch();

    OpenFileLeft();
    //OpenFileRight();
}

void blePedals::startSearch()
{
    if (agent->isActive())
        agent->stop();

    foundDevices.clear();

    agent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}


void blePedals::establishConnection()
{

    if (!device1) {
        std::cout << "establishing connection device 1" << std::endl;

        for (int i=0;i<3;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("C6:21:8B:A7:24:5F")))
//            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("F6:D0:29:C5:60:4C")))
                device1 = new QLowEnergyController(desiredDevices.at(i));
        }
        device1->setParent(this);
        connect(device1, &QLowEnergyController::connected, this, [&](){
            ok_pleft = true;
            disconnected1 = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_pedal_left = ok_pleft;

            qDebug() << "*********** Device 1 SRM_XP_L_1818 connected" << device1->remoteAddress();
            device1->discoverServices();
            reconnectTimer1->stop();
        });

        connect(device1, &QLowEnergyController::disconnected, this, [&](){
            disconnected1 = true;
            qDebug() << "Device 1 disconnected";
            ok_pleft = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_pedal_left = ok_pleft;

            reconnectTimer1->start();
        });

        connect(device1, &QLowEnergyController::discoveryFinished, this, [&](){
           qDebug() <<  "*********** Device 1 discovery finished";
           setupNotificationLeft(device1, QStringLiteral("Device 1"));
        });

        device1->connectToDevice();
    }

/*     if (!device2 && desiredDevices.count() >= 2) {
        std::cout << "establishing connection device 2" << std::endl;

        for (int i=0;i<3;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("ED:86:C3:29:8A:05")))
//            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("D5:5E:63:D1:CE:BF")))
            device2 = new QLowEnergyController(desiredDevices.at(i));
        }
        device2->setParent(this);

        connect(device2, &QLowEnergyController::connected, this, [&](){
            ok_pright=1;
            disconnected2 = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_pedal_right = ok_pright;

            qDebug() << "*********** Device 2 SRM_XP_R_1968 connected" << device2->remoteAddress();
            device2->discoverServices();
            reconnectTimer2->stop();
        });

        connect(device2, &QLowEnergyController::disconnected, this, [&](){
            disconnected2 = true;
            qDebug() << "Device 2 disconnected";
            ok_pright = false;

            SingletonSM* singletonSM = SingletonSM::getInstance();
            shared_memory* shmem = singletonSM->get_SM();
            shmem->data->check_pedal_right = ok_pright;

            reconnectTimer2->start();
        });

        connect(device2, &QLowEnergyController::discoveryFinished, this, [&](){
           qDebug() <<  "*********** Device 2 discovery finished";
           setupNotificationRight(device2, QStringLiteral("Device 2"));
        });

        device2->connectToDevice();
    } */
}

void blePedals::reconnectDevice()
{
    if(disconnected1)
    {
        qDebug() << "Attempting to reconnect Device 1...";
        if (device1)
        {
            device1->disconnectFromDevice(); // Ensure the device is disconnected first
            device1->connectToDevice();
        }
        else
        {
            // If device1 is not initialized, create a new instance and connect
            device1 = new QLowEnergyController(desiredDevices.at(0));
            connect(device1, &QLowEnergyController::connected, this, [&](){
                ok_pleft = true;
                disconnected1 = false;

                SingletonSM* singletonSM = SingletonSM::getInstance();
                shared_memory* shmem = singletonSM->get_SM();
                shmem->data->check_pedal_left = ok_pleft;

                qDebug() << "*********** Device 1 Left Pedal connected" << device1->remoteAddress();
                device1->discoverServices();
                reconnectTimer1->stop();
            });

            connect(device1, &QLowEnergyController::disconnected, this, [&](){
                qDebug() << "Device 1 disconnected";
                ok_pleft = false;
                disconnected1 = true;

                SingletonSM* singletonSM = SingletonSM::getInstance();
                shared_memory* shmem = singletonSM->get_SM();
                shmem->data->check_pedal_left = ok_pleft;

                reconnectTimer1->start();
            });

            device1->connectToDevice();
        }
    }

    /* if(disconnected2)
    {
        qDebug() << "Attempting to reconnect Device 2...";
        if (device2)
        {
            device2->disconnectFromDevice(); // Ensure the device is disconnected first
            device2->connectToDevice();
        }
        else
        {
            // If device2 is not initialized, create a new instance and connect
            device2 = new QLowEnergyController(desiredDevices.at(1));
            connect(device2, &QLowEnergyController::connected, this, [&](){
                ok_pright = true;
                disconnected2 = false;

                SingletonSM* singletonSM = SingletonSM::getInstance();
                shared_memory* shmem = singletonSM->get_SM();
                shmem->data->check_pedal_right = ok_pright;

                qDebug() << "*********** Device 2 Right Pedal connected" << device2->remoteAddress();
                device2->discoverServices();
                reconnectTimer2->stop();
            });

            connect(device2, &QLowEnergyController::disconnected, this, [&](){
                qDebug() << "Device 2 disconnected";
                ok_pright = false;

                SingletonSM* singletonSM = SingletonSM::getInstance();
                shared_memory* shmem = singletonSM->get_SM();
                shmem->data->check_pedal_right = ok_pright;

                disconnected2 = true;
                reconnectTimer2->start();
            });

            device2->connectToDevice();
        }
    } */
}

void OpenFileLeft()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameLeft = strftime (buffer,80,"/home/pi/Desktop/FilePedaliCamilla/LeftAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileLeft.open(buffer);
    // write the file headers
    if(CSVfileLeft.is_open())
    {
        CSVfileLeft << std::endl << "Mean power" << std::endl;
    }
    else if (!CSVfileLeft.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameLeft << std::endl;
        return;
    }
}

void writeFileLeft(qint16 parameter1){
    // clock_gettime ( CLOCK_MONOTONIC, &timeLoop);
    CSVfileLeft << std::endl << parameter1;

}

void blePedals::setupNotificationLeft(QLowEnergyController *device, const QString &name)
{
    if (!device)
        return;

    // hook up power sensor
    QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::CyclingPower);
    if (!service) {
        qDebug() << "***********" << name << "force service not found";
        return;
    }

    qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

    connect(service, &QLowEnergyService::stateChanged,
            this, [name, service](QLowEnergyService::ServiceState s){
        if (s == QLowEnergyService::ServiceDiscovered) {
            qDebug() << "***********" << name << "force service discovered" << service->serviceUuid();
            const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::CyclingPowerMeasurement);

            if (!tempData.isValid()) {
                qDebug() << "***********" << name << "force char not valid";
                return;
            }

            const QLowEnergyDescriptor notification = tempData.descriptor(
                        QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));

            if (!notification.isValid()) {
                qDebug() << "***********" << name << "force notification not valid";
                return;
            }
            service->writeDescriptor(notification, QByteArray::fromHex("0100"));
        }
    });

    connect(service, &QLowEnergyService::characteristicChanged, this, &blePedals::getLeftPower);
   service->discoverDetails();
}

void blePedals::getLeftPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
        const char *data = newValue.constData();

// Power characteristic
        // Extract the flags (byte 0)
        quint8 flags = newValue[0];
        //qDebug() << "Flags value:" << flags;

        // Extract the instantaneous power (bytes 2 and 3)
        instantaneousLeftPower = (static_cast<qint16>(newValue[2]) | (static_cast<qint16>(newValue[3]) << 8));
        leftPowerVector.push_back(instantaneousLeftPower);
        num_left_data++;
        qDebug() << "Instantaneous Left Power:" << instantaneousLeftPower << "W";
        write_left_power(instantaneousLeftPower);

    if (num_left_data >= NUM_CYCLES) {
        leftSumPowerVector = 0;
        for (int element : leftPowerVector) {
            leftSumPowerVector += element;
        }
        averageLeftPower = leftSumPowerVector / leftPowerVector.size();
        qDebug() << "Average left power in" << NUM_CYCLES << " cycles: " << averageLeftPower;
        writeFileLeft(averageLeftPower);
        leftPowerVector.clear();
        // Reset the counter of cycles done
        num_left_data = 0;
    }

}

/* void OpenFileRight()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameRight = strftime (buffer,80,"/home/pi/Desktop/FilePedaliCamilla/RightAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileRight.open(buffer);
    // write the file headers
    if(CSVfileRight.is_open())
    {
        CSVfileRight << std::endl << "Mean power" << std::endl;
    }
    else if (!CSVfileRight.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameRight << std::endl;
        return;
    }
}

void writeFileRight(qint16 parameter1){
    // clock_gettime ( CLOCK_MONOTONIC, &timeLoop);
    CSVfileRight << std::endl << parameter1;

}

void blePedals::setupNotificationRight(QLowEnergyController *device, const QString &name)
{

    if (!device)
        return;

    // hook up force sensor
    QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::CyclingPower);
    if (!service) {
        qDebug() << "***********" << name << "force service not found";
        return;
    }

    qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

    connect(service, &QLowEnergyService::stateChanged,
            this, [name, service](QLowEnergyService::ServiceState s){
        if (s == QLowEnergyService::ServiceDiscovered) {
            qDebug() << "***********" << name << "force service discovered" << service->serviceUuid();
            const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::CyclingPowerMeasurement);     

            if (!tempData.isValid()) {
                qDebug() << "***********" << name << "force char not valid";
                return;
            }

            const QLowEnergyDescriptor notification = tempData.descriptor(
                        QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));
            if (!notification.isValid()) {
                qDebug() << "***********" << name << "force notification not valid";
                return;
            }

            service->writeDescriptor(notification, QByteArray::fromHex("0100"));

        }
    });

    connect(service, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getRightPower);
    service->discoverDetails();
}

void blePedals::getRightPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{

    const char *data = newValue.constData();
    //valid data set count U8
    //quint8 flags = data[3];
//    qDebug() << "Valid data set count" << flags;

// Power characteristic
    // Extract the flags (byte 0)
    quint8 flags = newValue[0];
    //qDebug() << "Flags value:" << flags;

    // Extract the instantaneous power (bytes 2 and 3)
    instantaneousRightPower = (static_cast<qint16>(newValue[2]) | (static_cast<qint16>(newValue[3]) << 8));
    rightPowerVector.push_back(instantaneousRightPower);
    num_right_data++;
    qDebug() << "Instantaneous Right Power:" << instantaneousRightPower << "W";
    write_right_power(instantaneousRightPower);

    if (num_right_data >= NUM_CYCLES) {
        rightSumPowerVector = 0;
        for (int element : rightPowerVector) {
            rightSumPowerVector += element;
        }
        averageRightPower = rightSumPowerVector / rightPowerVector.size();
        qDebug() << "Average right power in" << NUM_CYCLES << " cycles: " << averageRightPower;
        writeFileRight(averageRightPower);
        rightPowerVector.clear();
        // Reset the counter of cyles done
        num_right_data = 0;
    }

} */