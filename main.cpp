#include <iostream>
#include <QCoreApplication>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include "chatserver.h"
#include <QBluetoothAddress>
#include <QtWidgets/qapplication.h>

int main(int argc, char *argv[]){
    std::cout << "Hello, from BT_CDR!\n";

    QCoreApplication a(argc, argv);
    //std::cout << "Hello, from BT_CDR 1!\n";
    QBluetoothAddress address("E4:5F:01:F5:E4:4A");
    ChatServer* chatServer;
    //std::cout << "Hello, from BT_CDR 2!\n";
    chatServer = new ChatServer();
    //std::cout << "Hello, from BT_CDR 3!\n";
    chatServer->startServer(address);
    //std::cout << "\n";
    return a.exec();

    /*ChatServer* server = new ChatServer();
    QList<QBluetoothHostInfo> localAdapters;
    localAdapters = QBluetoothLocalDevice::allDevices();
    QBluetoothLocalDevice adapter(localAdapters.at(0).address());
    adapter.setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    server->startServer();*/

}
