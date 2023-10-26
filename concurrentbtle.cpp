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
#include <cstdlib>

#define NUM_CYCLES 5

SingletonSM* singletonSM = SingletonSM::getInstance();
shared_memory* shmem = singletonSM->get_SM();

ofstream CSVfileLeft; //dato grezzo pedale sinistro ed efficiency
ofstream CSVfileRight; //dato grezzo pedale destro ed efficiency
//ofstream CSVfileCardio; //dato grezzo cardio
ofstream CSVfileLeftPowerForce;
ofstream CSVfileRightPowerForce;

// create a name for the file output
std::string filenameLeft = "FileLeft.csv";
std::string filenameRight = "FileRight.csv";
//std::string filenameCardio = "FileCardio.csv";
std::string filenameLeftPowerForce = "FileLeftPowerForce.csv";
std::string filenameRightPowerForce = "FileRightPowerForce.csv";

std::vector<double> leftPowerVector;
std::vector<double> rightPowerVector;

int sumLeftPowerVector = 0;
int sumRightPowerVector = 0;

double Angle_old_left = 0.0;
double Angle_old_right = 0.0;

bool ok_pleft = 0;
bool ok_pright = 0;
// bool ok_cardio = 0;
// int hr_sconnesso = 0;

bool disconnected1 = true;
bool disconnected2 = true;
//bool disconnected3 = true;

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
    //std::system("hciconfig hci1 down");     //Disable the Internal Bluetooth Adapter
    //std::system("btattach -B hci0 -P public -S 115200 /dev/ttyUSB0");     //Set the USB Dongle as the Default Adapter
    desiredDevices << QBluetoothAddress(QStringLiteral("C6:21:8B:A7:24:5F")); /*SRM_XP_L_1818     Colombo*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("F6:D0:29:C5:60:4C")); /*SRM_XP_L_2623   Lecco*/
    desiredDevices << QBluetoothAddress(QStringLiteral("ED:86:C3:29:8A:05")); /*SRM_XP_R_1968     Colombo*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("D5:5E:63:D1:CE:BF")); /*SRM_XP_R_2971   Lecco*/
//    desiredDevices << QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:CC")); /*Polar H10 8E5AB228*/ //C8:75:75:F8:F1:FA

    agent = new QBluetoothDeviceDiscoveryAgent(this);
    agent->setLowEnergyDiscoveryTimeout(10000);
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
                //startSearch();
                //break;
            }
        }
        if(num_des_dev_found == 0)
        {
            qDebug() << "Cannot find any device";
            startSearch();      // keep looking for devices when non found
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

    // reconnectTimer3 = new QTimer(this);
    // reconnectTimer3->setInterval(5000); // Adjust the interval as needed (e.g., 5 seconds)
    // connect(reconnectTimer3, &QTimer::timeout, this, &ConcurrentBtle::reconnectDevice);

    startSearch();

    // OpenFileLeft();
    // OpenFileRight();
    // OpenFileCardio();
    // OpenFileLeftPowerForce();
    // OpenFileRightPowerForce();
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

    if (!device1) {
        std::cout << "establishing connection device 1" << std::endl;

        for (int i=0;i<2;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("C6:21:8B:A7:24:5F")))     //Colombo
//            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("F6:D0:29:C5:60:4C")))       //Lecco
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

        for (int i=0;i<2;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("ED:86:C3:29:8A:05")))     //Colombo
//            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("D5:5E:63:D1:CE:BF")))     //Lecco
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

    // if (!device3 && desiredDevices.count() >= 3) {
    //     std::cout << "establishing connection device 3" << std::endl;

    //     for (int i=0;i<3;i++) {
    //         if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("C8:75:75:F8:F1:CC")))   //C8:75:75:F8:F1:FA
    //         device3 = new QLowEnergyController(desiredDevices.at(i));
    //     }
        
    //     device3->setParent(this);
    //     connect(device3, &QLowEnergyController::connected, this, [&](){
    //         ok_cardio = 1;
    //         disconnected3 = false;

    //         SingletonSM* singletonSM = SingletonSM::getInstance();
    //         shared_memory* shmem = singletonSM->get_SM();
    //         shmem->data->check_cardio = ok_cardio;

    //         qDebug() << "*********** Device 3 Polar H10 connected" << device3->remoteAddress();
    //         device3->discoverServices();
    //         reconnectTimer3->stop();
    //     });

    //     connect(device3, &QLowEnergyController::disconnected, this, [&](){
    //         ok_cardio = 0;
    //         disconnected3 = true;

    //         SingletonSM* singletonSM = SingletonSM::getInstance();
    //         shared_memory* shmem = singletonSM->get_SM();
    //         shmem->data->check_cardio = ok_cardio;

    //         qDebug() << "Device 3 Disconnected";

    //         reconnectTimer3->start();
    //     });

    //     connect(device3, &QLowEnergyController::discoveryFinished, this, [&](){
    //        qDebug() <<  "*********** Device 3 discovery finished";
    //        setupNotificationCardio(device3, QStringLiteral("Device 3"));
    //     });

    //     device3->connectToDevice();
    // }
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

    // if(disconnected3)
    // {
    //     qDebug() << "Attempting to reconnect Device 3...";
    //     if (device3)
    //     {
    //         device3->disconnectFromDevice(); // Ensure the device is disconnected first
    //         device3->connectToDevice();
    //     }
    //     else
    //     {
    //         // If device3 is not initialized, create a new instance and connect
    //         device3 = new QLowEnergyController(desiredDevices.at(2));
    //         connect(device3, &QLowEnergyController::connected, this, [&](){
    //             ok_cardio = true;
    //             disconnected3 = false;

    //             SingletonSM* singletonSM = SingletonSM::getInstance();
    //             shared_memory* shmem = singletonSM->get_SM();
    //             shmem->data->check_cardio = ok_cardio;

    //             qDebug() << "*********** Device 3 Polar connected" << device3->remoteAddress();
    //             device3->discoverServices();
    //             reconnectTimer3->stop();

    //         });

    //         connect(device3, &QLowEnergyController::disconnected, this, [&](){
    //             qDebug() << "Device 3 disconnected";
    //             ok_cardio = false;
    //             disconnected3 = true;

    //             SingletonSM* singletonSM = SingletonSM::getInstance();
    //             shared_memory* shmem = singletonSM->get_SM();
    //             shmem->data->check_cardio = ok_cardio;

    //             reconnectTimer3->start();
    //         });

    //         device3->connectToDevice();
    //     }
    // }
}

void OpenFileLeft()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameLeft = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/DatiPedali_forcePower/LeftAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
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

void OpenFileLeftPowerForce()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameLeftPowerForce = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/DatiPedali_forcePower/LeftPowerForceAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileLeftPowerForce.open(buffer);
    // write the file headers
    if(CSVfileLeftPowerForce.is_open())
    {
        CSVfileLeftPowerForce << std::endl << "Left power from force" << std::endl;
    }
    else if (!CSVfileLeftPowerForce.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameLeftPowerForce << std::endl;
        return;
    }
}

void ConcurrentBtle::setupNotificationLeft(QLowEnergyController *device, const QString &name)
{
    if (!device)
        return;

    // hook up power sensor
    // QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::CyclingPower);
    // if (!service) {
    //     qDebug() << "***********" << name << "power service not found";
    //     return;
    // }

    // qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

    // connect(service, &QLowEnergyService::stateChanged,
    //         this, [name, service](QLowEnergyService::ServiceState s){
    //     if (s == QLowEnergyService::ServiceDiscovered) {
    //         qDebug() << "***********" << name << "power service discovered" << service->serviceUuid();
    //         const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::CyclingPowerMeasurement);

    //         if (!tempData.isValid()) {
    //             qDebug() << "***********" << name << "power char not valid";
    //             return;
    //         }

    //         const QLowEnergyDescriptor notification = tempData.descriptor(
    //                     QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));

    //         if (!notification.isValid()) {
    //             qDebug() << "***********" << name << "power notification not valid";
    //             return;
    //         }
    //         service->writeDescriptor(notification, QByteArray::fromHex("0100"));
    //     }
    // });

    // connect(service, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getLeftPower);
    // service->discoverDetails();

   // hook up force sensor
    QLowEnergyService *forceService = device->createServiceObject(QBluetoothUuid(QStringLiteral("7f510001-1b15-11e5-b60b-1697f925ec7b")));
    if (!forceService) {
        qDebug() << "***********" << name << "force service not found";
        return;
    }

    qDebug() << "#####" << name << forceService->serviceName() << forceService->serviceUuid();

    connect(forceService, &QLowEnergyService::stateChanged,
            this, [name, forceService](QLowEnergyService::ServiceState s){
        if (s == QLowEnergyService::ServiceDiscovered) {
            qDebug() << "***********" << name << "force service discovered" << forceService->serviceUuid();
            const QLowEnergyCharacteristic forceTempData = forceService->characteristic(QBluetoothUuid(QStringLiteral("7f510019-1b15-11e5-b60b-1697f925ec7b")));

            if (!forceTempData.isValid()) {
                qDebug() << "***********" << name << "force char not valid";
                return;
            }

            const QLowEnergyDescriptor forceNotification = forceTempData.descriptor(
                        QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));

            if (!forceNotification.isValid()) {
                qDebug() << "***********" << name << "force notification not valid";
                return;
            }
            forceService->writeDescriptor(forceNotification, QByteArray::fromHex("0100"));
        }
    });

    connect(forceService, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getLeftForce);
    forceService->discoverDetails();
}

void ConcurrentBtle::getLeftPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    newLeftData = true;
    const char *data = newValue.constData();

// Power characteristic
    // Extract the flags (byte 0)
    quint8 flags = newValue[0];
    //qDebug() << "Flags value:" << flags;

    // Extract the instantaneous power (bytes 2 and 3)
    instantaneousLeftPower = (static_cast<qint16>(newValue[2]) | (static_cast<qint16>(newValue[3]) << 8));
    leftPowerVector.push_back(instantaneousLeftPower);
    num_left_data++;
    //qDebug() << "Instantaneous Left Power:" << instantaneousLeftPower << "W";
    write_left_power(instantaneousLeftPower);

    if (num_left_data >= NUM_CYCLES) {
        leftSumPowerVector = 0;
        for (int element : leftPowerVector) {
            leftSumPowerVector += element;
        }
        averageLeftPower = leftSumPowerVector / leftPowerVector.size();
        //qDebug() << "Average left power in" << NUM_CYCLES << " cycles: " << averageLeftPower;
        writeFileLeft(averageLeftPower);
        leftPowerVector.clear();
        // Reset the counter of cycles done
        num_left_data = 0;
    }

}

void ConcurrentBtle::getLeftForce(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    const char *data = newValue.constData();

    // Force characteristic
    // valid data set count U8
    quint8 flags = data[3];
    //qDebug() << "Valid data set count" << flags;

 //prendo il primo valore che mi invia per calcolare il power output
 //prendo forza tangenziale e la moltiplica per la lunghezza della pedivella (ottengo la coppia) e poi lo moltiplico per la cadenza
        
    // Tangential force
    qint16 *TFpo = (qint16 *) &data[8];
    double TF_left = (double)(*TFpo)/10;
    //qDebug() << "Left TF value:" << TF_left << "N";
    //leftForceVector.push_back(TF_left);

    qint16 *cadpo = (qint16 *) &data[14];
    double cadence = (double)(*cadpo)/1024;
    double cadence_value=(double)(*cadpo)/1024*30/3.14;
    //qDebug() << "Cadence value:" << cadence_value <<"rpm";

    qint16 *angpo = (qint16 *) &data[12];
    double angle = (double)(*angpo);
    //qDebug() << "Angle left: " << angle << "degrees";

    int cranck_length = 155;
    double powerLeft = TF_left*(-cadence)*cranck_length/1000;
    //qDebug() << "Instantaneous Power Output Left:" << powerLeft <<"W";
    powerOutputLeft(powerLeft, angle);
    
}

void powerOutputLeft (double power,  double angle){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();
    //qDebug() << "Left instant power: " << power << "W";
    int averageLeftPower;

    double diff = (angle + 359) - Angle_old_left;

    if ((angle >= Angle_old_left) || ((angle < Angle_old_left) && (diff >= Angle_old_left)))
    {
        leftPowerVector.push_back(power);
        //qDebug() << "Numbers of power data: " << leftPowerVector.size();
    }
    else if(angle < Angle_old_left) 
    {
        //if new cycle
        if (diff < Angle_old_left)
        { 
            for (int element : leftPowerVector) {
                sumLeftPowerVector += element;
            }
            //qDebug() << "Sum elements power vector " << dec << sumLeftPowerVector << "W";
            //qDebug() << "Number elements power vector " << leftPowerVector.size();
            int size = (int)leftPowerVector.size();
            averageLeftPower = sumLeftPowerVector / size;
            //qDebug() << "Average left power in one cycle: " << averageLeftPower << "W";
            shmem->data->average_left_power = averageLeftPower;
            shmem->data->new_left_data = true;
            write_left_power(averageLeftPower);
            CSVfileLeftPowerForce << std::endl << averageLeftPower;
            leftPowerVector.clear();
            size = 0;
            sumLeftPowerVector = 0;
        }
    }
    Angle_old_left = angle;
}

void OpenFileRight()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameRight = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/DatiPedali_forcePower/RightAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileRight.open(buffer);
    // write the file headers
    if(CSVfileRight.is_open())
    {
        CSVfileRight << std::endl << "Right power" << std::endl;
    }
    else if (!CSVfileRight.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameRight << std::endl;
        return;
    }
}

void OpenFileRightPowerForce()
{
    // Get system  time
    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    filenameRightPowerForce = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/DatiPedali_forcePower/RightPowerForceAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    CSVfileRightPowerForce.open(buffer);
    // write the file headers
    if(CSVfileRightPowerForce.is_open())
    {
        CSVfileRightPowerForce << std::endl << "Right power from force" << std::endl;
    }
    else if (!CSVfileRightPowerForce.is_open()) {
        std::cout << "Error: Unable to open the file " << filenameRightPowerForce << std::endl;
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

    // // hook up power sensor
    // QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::CyclingPower);
    // if (!service) {
    //     qDebug() << "***********" << name << "power service not found";
    //     return;
    // }

    // qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

    // connect(service, &QLowEnergyService::stateChanged,
    //         this, [name, service](QLowEnergyService::ServiceState s){
    //     if (s == QLowEnergyService::ServiceDiscovered) {
    //         qDebug() << "***********" << name << "power service discovered" << service->serviceUuid();
    //         const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::CyclingPowerMeasurement);     

    //         if (!tempData.isValid()) {
    //             qDebug() << "***********" << name << "power char not valid";
    //             return;
    //         }

    //         const QLowEnergyDescriptor notification = tempData.descriptor(
    //                     QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));
    //         if (!notification.isValid()) {
    //             qDebug() << "***********" << name << "power notification not valid";
    //             return;
    //         }

    //         service->writeDescriptor(notification, QByteArray::fromHex("0100"));

    //     }
    // });

    // connect(service, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getRightPower);
    // service->discoverDetails();

    // hook up force sensor
    QLowEnergyService *forceService = device->createServiceObject(QBluetoothUuid(QStringLiteral("7f510001-1b15-11e5-b60b-1697f925ec7b")));
    if (!forceService) {
        qDebug() << "***********" << name << "force service not found";
        return;
    }

    qDebug() << "#####" << name << forceService->serviceName() << forceService->serviceUuid();

    connect(forceService, &QLowEnergyService::stateChanged,
            this, [name, forceService](QLowEnergyService::ServiceState s){
        if (s == QLowEnergyService::ServiceDiscovered) {
            qDebug() << "***********" << name << "force service discovered" << forceService->serviceUuid();
            const QLowEnergyCharacteristic forceTempData = forceService->characteristic(QBluetoothUuid(QStringLiteral("7f510019-1b15-11e5-b60b-1697f925ec7b")));

            if (!forceTempData.isValid()) {
                qDebug() << "***********" << name << "force char not valid";
                return;
            }

            const QLowEnergyDescriptor forceNotification = forceTempData.descriptor(
                        QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));

            if (!forceNotification.isValid()) {
                qDebug() << "***********" << name << "force notification not valid";
                return;
            }
            forceService->writeDescriptor(forceNotification, QByteArray::fromHex("0100"));
        }
    });

    connect(forceService, &QLowEnergyService::characteristicChanged, this, &ConcurrentBtle::getRightForce);
    forceService->discoverDetails();
}

void ConcurrentBtle::getRightPower(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    newRightData = true;
    const char *data = newValue.constData();

// Power characteristic
    // Extract the flags (byte 0)
    quint8 flags = newValue[0];
    //qDebug() << "Flags value:" << flags;

    // Extract the instantaneous power (bytes 2 and 3)
    instantaneousRightPower = (static_cast<qint16>(newValue[2]) | (static_cast<qint16>(newValue[3]) << 8));
    rightPowerVector.push_back(instantaneousRightPower);
    num_right_data++;
    //qDebug() << "Instantaneous Right Power:" << instantaneousRightPower << "W";
    write_right_power(instantaneousRightPower);

    if (num_right_data >= NUM_CYCLES) {
        rightSumPowerVector = 0;
        for (int element : rightPowerVector) {
            rightSumPowerVector += element;
        }
        averageRightPower = rightSumPowerVector / rightPowerVector.size();
        //qDebug() << "Average right power in" << NUM_CYCLES << " cycles: " << averageRightPower;
        writeFileRight(averageRightPower);
        rightPowerVector.clear();
        // Reset the counter of cyles done
        num_right_data = 0;
    }
}

void ConcurrentBtle::getRightForce(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue)
{
    const char *data = newValue.constData();

    // Force characteristic
    // valid data set count U8
    quint8 flags = data[3];
    //qDebug() << "Valid data set count" << flags;

 //prendo il primo valore che mi invia per calcolare il power output
 //prendo forza tangenziale e la moltiplica per la lunghezza della pedivella (ottengo la coppia) e poi lo moltiplico per la cadenza
        
    // Tangential force
    qint16 *TFpo = (qint16 *) &data[8];
    double TF_right = (double)(*TFpo)/10;
    //qDebug() << "Right TF value:" << TF_right << "N";
    //rightForceVector.push_back(TF_right);

    qint16 *cadpo = (qint16 *) &data[14];
    double cadence = (double)(*cadpo)/1024;
    //qDebug() << "Cadence value:" << (double)(*cadpo)/1024*30/3.14 <<"rpm";

    qint16 *angpo = (qint16 *) &data[12];
    double angle = (double)(*angpo);

    int cranck_length = 155;
    double powerRight = TF_right*cadence*cranck_length/1000;
    //qDebug() << "Instantaneous Power Output Right:" << powerRight <<"W";
    powerOutputRight(powerRight, angle);
    
}

void powerOutputRight (double power, double angle){

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    int averageRightPower;

    double diff = (angle + 359) - Angle_old_right;

    if ((angle >= Angle_old_right) || ((angle < Angle_old_right) && (diff >= Angle_old_right)))
    {
        rightPowerVector.push_back(power);
    }
    else if(angle < Angle_old_right) 
    {
        //if new cycle
        if (diff < Angle_old_right)
        { 
            for (int element : rightPowerVector) {
                sumRightPowerVector += element;
            }
            int size = (int)rightPowerVector.size();
            averageRightPower = sumRightPowerVector / size;
            shmem->data->average_right_power = averageRightPower;
            //qDebug() << "Average right power in one cycle: " << averageRightPower << "W";
            shmem->data->new_right_data = true;
            write_right_power(averageRightPower);
            CSVfileRightPowerForce << std::endl << averageRightPower;
            rightPowerVector.clear();
            size = 0;
            sumRightPowerVector = 0;
        }
    }
    Angle_old_right = angle;
}

// void OpenFileCardio()
// {
//     // Get system  time
//     time_t t = time(nullptr);
//     struct tm * now = localtime( & t );
//     char buffer [80];

//     // Log directory
//     filenameCardio = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/DatiPedali_forcePower/CardioAcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
//     CSVfileCardio.open(buffer);
//     // write the file headers
//     if(CSVfileCardio.is_open())
//     {
//         CSVfileCardio << std::endl << "Heart Rate" << std::endl;
//     }
//     else if (!CSVfileCardio.is_open()) {
//         std::cout << "Error: Unable to open the file " << filenameCardio << std::endl;
//         return;
//     }
// }

// void writeFileCardio(double parameter1){
//     // clock_gettime ( CLOCK_MONOTONIC, &timeLoop);
//     CSVfileCardio << std::endl << parameter1;

// }

// void ConcurrentBtle::setupNotificationCardio(QLowEnergyController *device, const QString &name)
// {

//     if (!device)
//         return;

//     QLowEnergyService *service = device->createServiceObject(QBluetoothUuid::HeartRate);
//     if (!service) {
//         qDebug() << "***********" << name << "cardio service not found";
//         return;
//     }

//     qDebug() << "#####" << name << service->serviceName() << service->serviceUuid();

//     connect(service, &QLowEnergyService::stateChanged,
//             this, [name, service](QLowEnergyService::ServiceState s){
//         if (s == QLowEnergyService::ServiceDiscovered) {
//             qDebug() << "***********" << name << "cardio service discovered" << service->serviceUuid();
//             const QLowEnergyCharacteristic tempData = service->characteristic(QBluetoothUuid::HeartRateMeasurement);

//             if (!tempData.isValid()) {
//                 qDebug() << "***********" << name << "cardio char not valid";
//                 return;
//             }

//             const QLowEnergyDescriptor notification = tempData.descriptor(
//                         QBluetoothUuid(QBluetoothUuid::ClientCharacteristicConfiguration));
//             if (!notification.isValid()) {
//                 qDebug() << "***********" << name << "cardio notification not valid";
//                 return;
//             }

//             service->writeDescriptor(notification, QByteArray::fromHex("0100"));

//         }
//     });


//     connect(service, &QLowEnergyService::characteristicChanged,
//             this, [name](const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue) {

//         int a = (newValue.size());
//         // qDebug() << "Size:" << a;

//         const char *data = newValue.constData();
//         quint8 flags = data[0];

//         //HR 8bit
//         quint8 *heartrate= (quint8 *) &data[1];
//         qDebug() << "HR value" << name << *heartrate <<"bpm" ;
//         double hr_value = *heartrate;
//         write_heart_rate(hr_value);
//         writeFileCardio(hr_value);

//     });
//    service->discoverDetails();
// }

