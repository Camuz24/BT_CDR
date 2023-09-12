#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "server.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>
#include <unistd.h>
#include "shared_memory.h"
#include "concurrentbtle.h"
#include "singletonSM.h"

using std::string;

SingletonSM* SingletonSM::instancePtr = NULL;

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    
    SingletonSM* singletonSM = SingletonSM::getInstance();
    singletonSM->detach_shared_memory();
    exit(signum);
    
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
    ConcurrentBtle* btle;
    // btle = new ConcurrentBtle(); 
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
