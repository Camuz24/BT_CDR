#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "chatserver.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>
#include <unistd.h>
#include "shared_memory.h"

shared_memory shmem;

void myInterruptHandler (int signum) {

    printf ("ctrl-c has been pressed. Programs will be terminated in sequence.\n");
    
    shmem.detach_shared_memory();
    exit(signum);
    
}


int main(int argc, char *argv[]){
    std::cout << "Starting server...!\n";

    QCoreApplication a(argc, argv);
    QBluetoothAddress address("E4:5F:01:F5:E4:4A");
    
    ChatServer* chatServer;
    chatServer = new ChatServer(shmem);
    chatServer->startServer(address);
    
    signal(SIGINT, myInterruptHandler);

    return a.exec();
}
