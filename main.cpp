#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "server.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>
#include <unistd.h>
#include <fstream>
#include <pthread.h>
#include <thread>
#include "shared_memory.h"
#include "concurrentbtle.h"
#include "singletonSM.h"
#include "powerController.h"

//using std::string;
using namespace std;
powerController FEScontrol;
ConcurrentBtle* btle;
float targetPower = 0;

ofstream powerControlFile;
string fileName;    // create a name for the file output

SingletonSM* SingletonSM::instancePtr = NULL;

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    FEScontrol.PidOFF();
    
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->detach_shared_memory();
    exit(signum);
    
}

void powerControl()
{
    qint16 totalPower;
    float powerPidOutput = 0;

    SingletonSM* singletonSM = SingletonSM::getInstance();
    shared_memory* shmem = singletonSM->get_SM();

    while(1)
    {
        if(btle->newLeftData && btle->newRightData)
        { 
            totalPower = btle->instantaneousLeftPower + btle->instantaneousRightPower;
            cout << "Total power (left + right) over one cycle:" << totalPower << endl;
            powerPidOutput = FEScontrol.PID(targetPower, totalPower);
            cout << "Pid coefficient:" << powerPidOutput << endl;

            powerControlFile << endl << powerPidOutput << "," << totalPower << "," << shmem->data->gear;

            btle->newLeftData = false;
            btle->newRightData = false;
        }
    }

}

int main(int argc, char *argv[]){
    std::cout << "Starting server...!\n";

    QCoreApplication a(argc, argv);

    QList<QBluetoothHostInfo> localAdapters;
    localAdapters = QBluetoothLocalDevice::allDevices();
    string myMAC = localAdapters.at(0).address().toString().toStdString();
    std::cout << myMAC << std::endl;

    QBluetoothAddress address(myMAC.c_str());
    
    ChatServer* chatServer;
    
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->init_SM();

    chatServer = new ChatServer();
    chatServer->startServer(address);

    cout << "Set a target total power:" << endl;
    cin >> targetPower;

    time_t t = time(nullptr);
    struct tm * now = localtime( & t );
    char buffer [80];

    // Log directory
    fileName = strftime (buffer,80,"/home/pi/Desktop/BT_CDR/FilePowerControlCamilla/AcquiredData-%Y-%m-%d-%H-%M-%S.csv",now);
    powerControlFile.open(buffer);
    // write the file headers
    if(powerControlFile.is_open())
    {
        powerControlFile << endl << "PID coefficient" << "," 
                                 << "Total Power" << "," 
                                 << "Gear" << endl;
    }
    else if (!powerControlFile.is_open()) {
        cout << "Error: Unable to open the file " << fileName << endl;
    }

    //ConcurrentBtle* btle;
    btle = new ConcurrentBtle();

    thread Thread(powerControl);
    FEScontrol.PidON();

    //TODO: crea due classi separate per polar e pedale sinistro
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
