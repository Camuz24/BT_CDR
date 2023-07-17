#include "chat.h"
#include "chatserver.h"
#include <iostream>

#include <QtCore/qdebug.h>

#include <QtBluetooth/qbluetoothdeviceinfo.h>
#include <QtBluetooth/qbluetoothlocaldevice.h>
#include <QtBluetooth/qbluetoothuuid.h>
#include <QDialog>

using namespace std;

#ifdef Q_OS_ANDROID
#include <QtAndroidExtras/QtAndroid>
#endif

static const QLatin1String serviceUuid("e8e10f95-1a70-4b27-9ccf-02010264e9c8");
#ifdef Q_OS_ANDROID
static const QLatin1String reverseUuid("c8e96402-0102-cf9c-274b-701a950fe1e8");
#endif

Chat::Chat(QWidget *parent)
    : QDialog(parent)
{

    localAdapters = QBluetoothLocalDevice::allDevices();
    
    QBluetoothLocalDevice adapter(localAdapters.at(0).address());
    adapter.setHostMode(QBluetoothLocalDevice::HostDiscoverable);
    

    //! [Create Chat Server]
    server = new ChatServer(this);
    cout << "Server created";
    connect(server, QOverload<const QString &>::of(&ChatServer::clientConnected),
            this, &Chat::clientConnected);
    connect(server, QOverload<const QString &>::of(&ChatServer::clientDisconnected),
            this,  QOverload<const QString &>::of(&Chat::clientDisconnected));
    connect(server, &ChatServer::messageReceived,
            this,  &Chat::showMessage);
    connect(this, &Chat::sendMessage, server, &ChatServer::sendMessage);
    server->startServer();
    //! [Create Chat Server]

    //! [Get local device name]
    localName = QBluetoothLocalDevice().name();
    //! [Get local device name]
}

Chat::~Chat()
{
    delete server;
}

//! [clientConnected clientDisconnected]
void Chat::clientConnected(const QString &name)
{
    //ui->chat->insertPlainText(QString::fromLatin1("%1 has joined chat.\n").arg(name));
    cout << "Client connected!\n";
}

void Chat::clientDisconnected(const QString &name)
{
    //ui->chat->insertPlainText(QString::fromLatin1("%1 has left.\n").arg(name));
    cout << "Client disconnected!\n";
}
//! [clientConnected clientDisconnected]

void Chat::reactOnSocketError(const QString &error)
{
    // ui->chat->insertPlainText(error);
    cout << "error";
}

//! [sendClicked]
void Chat::sendClicked()
{
    QString message = "chat.cpp sendClicked";

    emit sendMessage(message);
}
//! [sendClicked]

//! [showMessage]
void Chat::showMessage(const QString &sender, const QString &message)
{
    cout << "Received message from ";
}
//! [showMessage]
