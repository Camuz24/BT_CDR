#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "server.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>
#include <unistd.h>
#include "shared_memory.h"

using std::string;

shared_memory shmem;

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    
    shmem.detach_shared_memory();
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
    chatServer = new ChatServer(shmem);
    chatServer->startServer(address);
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
