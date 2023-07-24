#include "concurrentbtle.h"
#include <QDebug>
#include <QTimer>
#include <QtEndian>
#include <iostream>
#include <fstream>
#include <QtCore/QtAlgorithms>
#include <QtCore>
#include <math.h>

ofstream CSVfileL; //dato grezzo pedale sinistro ed efficiency
ofstream CSVfileR; //dato grezzo pedale destro ed efficiency
ofstream CSVfileC; //dato grezzo cardio
ofstream CSVfilePL; //dato po media e smoothness sinistro
ofstream CSVfilePR; //dato po media e smoothness destro
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

shared_memory global_shmem;

void write_heart_rate(double hr_value){
    global_shmem.data->heart_rate=hr_value;
}


// code to initialize btle function
ConcurrentBtle::ConcurrentBtle(shared_memory* shmem, QObject *parent) : QObject(parent)
{
    global_shmem = *shmem;
    global_shmem.init();
    desiredDevices << QBluetoothAddress(QStringLiteral("EE:5D:EE:37:DE:25")); /*Polar H10 8E5AB228*/

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
        temp_check = 1;
        // write2temp(temp_check);
        // qDebug() << "temp check" << temp_check;
        qDebug() << "Discovery finished";
        // add a boolean to check connection with shared memory

        for (auto desiredDevice: qAsConst(desiredDevices)) {

            bool found = false;

            for (auto foundDevice: foundDevices) {
                if (foundDevice.address() == desiredDevice) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                qDebug() << "Cannot find" << desiredDevice;
                return;
            }
        }
        qDebug() << "Found required device(s)" << desiredDevices;
        establishConnection();

    });

    startSearch();

    auto timer = new QTimer(this);
    timer->setInterval(10000);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (agent->isActive())
            return;
        if (!device3)
            establishConnection();
    });
    timer->start();
}

double findmax (double p_o, double p_o_o, double p){

    if((p<p_o)&&(p_o_o<p_o)){
        if (p_o>massimo){
        massimo=p_o; //solo se mi trovo in questa condizione altrimento max resta quello che ho già
             }
    else {
        massimo=massimo;
    }
    }
   return massimo;

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
        std::cout << "establish connection" << std::endl;
        for (int i=0;i<1;i++) {
            if (desiredDevices.at(i)==QBluetoothAddress(QStringLiteral("EE:5D:EE:37:DE:25")))
            device3 = new QLowEnergyController(desiredDevices.at(i));
        }
        
        device3->setParent(this);
        connect(device3, &QLowEnergyController::connected, this, [&](){
            ok_cardio=1;
            // write2tempcardio(ok_cardio);
            global_shmem.data->check_cardio = ok_cardio;

            qDebug() << "*********** Device 3 Polar H10 connected" << device3->remoteAddress();
            // global_shmem.data->check_cardio = ok_cardio;
            device3->discoverServices();
        });

        connect(device3, &QLowEnergyController::disconnected, this, [&](){
            ok_cardio=0;
            // write2tempcardio(ok_cardio);
            // hr_sconnesso=500;
            // write_heart_rate(hr_sconnesso);
            global_shmem.data->check_cardio = ok_cardio;
            qDebug() << "*********** Device 3 Disconnected";
            QTimer::singleShot(10000, this, [&](){
                qDebug() << "Reconnecting device 3";
                device3->connectToDevice();
                // device3 = nullptr;
                global_shmem.data->heart_rate = 0.0;
            });
        });

        connect(device3, &QLowEnergyController::discoveryFinished, this, [&](){
           qDebug() <<  "*********** Device 3 discovery finished";
           setupNotificationCardio(device3, QStringLiteral("Device 3"));
        });

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

