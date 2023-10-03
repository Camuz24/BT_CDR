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

#define NUM_CYCLES 5

ofstream CSVfileLeft; //dato grezzo pedale sinistro ed efficiency
ofstream CSVfileRight; //dato grezzo pedale destro ed efficiency
ofstream CSVfileCardio; //dato grezzo cardio

// create a name for the file output
std::string filenameLeft = "FileLeft.csv";
std::string filenameRight = "FileRight.csv";
std::string filenameCardio = "FileCardio.csv";

double Angle_old_left = 0.0;
double Angle_old_right = 0.0;
double sumpo_right = 0.0;
double sumpo_left = 0.0;
int countpo_right=0;
int countpo_left=0;
double po_old_right=0.0;
double po_old_left=0.0;
double po_old_old_left=0.0;
double po_old_old_right=0.0;
double massimo_right=0.0;
double massimo_left=0.0;
bool temp_check = 0;
double massimo = 0.0;

bool ok_pleft = 0;
bool ok_pright = 0;
bool ok_cardio = 0;
int hr_sconnesso = 0;

bool disconnected1 = true;
bool disconnected2 = true;
bool disconnected3 = true;

double sumtf_left=0.0;
double medciclotf_left=0.0;
int cicli_left=0;
double tfmean_left=0.0;
double sumtf_right=0.0;
double medciclotf_right=0.0;
int cicli_right=0;
double tfmean_right=0.0;


double sum_force_right = 0.0;
int count_force_right = 0;
double sum_meanforce_right = 0.0;
bool flag_ciclo_right = 0;

double sum_force_left = 0.0;
int count_force_left = 0;
double sum_meanforce_left= 0.0;
bool flag_ciclo_left = 0;

void write_heart_rate(double hr_value){
    // std::cout << "writing HR" << std::endl;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->heart_rate = hr_value;
}

void write_left_power(qint16 inst_left_power){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->power_left = inst_left_power;
}
void write_right_power(qint16 inst_right_power){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    shmem->data->power_right = inst_right_power;
}

// code to initialize btle function
ConcurrentBtle::ConcurrentBtle(QObject *parent) : QObject(parent)
{
    desiredDevices << QBluetoothAddress(QStringLiteral("C6:21:8B:A7:24:5F")); /*SRM_XP_L_1818*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("F6:D0:29:C5:60:4C")); /*SRM_XP_L_2623*/
    desiredDevices << QBluetoothAddress(QStringLiteral("ED:86:C3:29:8A:05")); /*SRM_XP_R_1968*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("D5:5E:63:D1:CE:BF")); /*SRM_XP_R_2971*/
    desiredDevices << QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:FA")); /*Polar H10 8E5AB228*/

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
        temp_check = 1;
        // write2temp(temp_check);
        // qDebug() << "temp check" << temp_check;
        qDebug() << "Discovery finished";
        // add a boolean to check connection with shared memory

        int num_des_dev_found = 0;

        for (auto desiredDevice: qAsConst(desiredDevices)) {

            bool found = false;

            for (auto foundDevice: foundDevices) {
                if (foundDevice.address() == desiredDevice) {
                    // desiredDevicesFound.append(foundDevice.address());
                    found = true;
                    num_des_dev_found++;
                    //break;
                }
            }

            if (!found) {
                qDebug() << "Cannot find" << desiredDevice;
                startSearch();
                break;
            }
        }
        if(num_des_dev_found == 0)
        {
            qDebug() << "Cannot find any device";
            startSearch();
        }
        else 
        {
            // qDebug() << "Found required devices" << desiredDevicesFound;
            establishConnection();
        }

    });

    reconnectTimer1 = new QTimer(this);
    reconnectTimer1->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer1, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice);

    reconnectTimer2 = new QTimer(this);
    reconnectTimer2->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer2, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice);

    reconnectTimer3 = new QTimer(this);
    reconnectTimer3->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    connect(reconnectTimer3, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice);

    startSearch();

    OpenFileLeft();
    OpenFileRight();
    OpenFileCardio();
}

/* double findmax (double p_o, double p_o_o, double p){

    if((p<p_o)&&(p_o_o<p_o)){
        if (p_o>massimo){
        massimo=p_o; //solo se mi trovo in questa condizione altrimento max resta quello che ho già
             }
    else {
        massimo=massimo;
    }
    }
   return massimo;

} */

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

    if (!device2 && desiredDevices.count() >= 2) {
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
    }

    if (!device3 && desiredDevices.count() >= 3) {
        std::cout << "establishing connection device 3" << std::endl;

        for (int i=0;i<3;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:FA")))
            device3 = new QLowEnergyController(desiredDevices.at(i));
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
            /*  QTimer::singleShot(10000, this, [&](){
                qDebug() << "Reconnecting device 3";
                device3->connectToDevice();
                // device3 = nullptr;
                SingletonSM* singletonSM = SingletonSM::getInstance();
                shared_memory* shmem = singletonSM->get_SM();
                shmem->data->heart_rate = 0.0; 
            }); */
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

    if(disconnected2)
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
    }

    if(disconnected3)
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

void ConcurrentBtle::setupNotificationLeft(QLowEnergyController *device, const QString &name)
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

    connect(service, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getLeftPower);
   service->discoverDetails();
}

void ConcurrentBtle::getLeftPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
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

void OpenFileRight()
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

void ConcurrentBtle::setupNotificationRight(QLowEnergyController *device, const QString &name)
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

void ConcurrentBtle::getRightPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
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

