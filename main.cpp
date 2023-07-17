#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "chatserver.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>
#include <unistd.h>

int main(int argc, char *argv[]){
    std::cout << "Hello, from BT_CDR!\n";

    QCoreApplication a(argc, argv);
    QBluetoothAddress address("E4:5F:01:F5:E4:4A");
    ChatServer* chatServer;
    chatServer = new ChatServer();
    chatServer->startServer(address);

    return a.exec();
}
