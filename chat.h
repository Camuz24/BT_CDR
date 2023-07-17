#include <QtWidgets/qdialog.h>
#include <QtBluetooth/qbluetoothhostinfo.h>

QT_USE_NAMESPACE

class ChatServer;

//! [declaration]
class Chat : public QDialog
{
    Q_OBJECT

public:
    explicit Chat(QWidget *parent = nullptr);
    ~Chat();

signals:
    void sendMessage(const QString &message);

private slots:
    void sendClicked();

    void showMessage(const QString &sender, const QString &message);

    void clientConnected(const QString &name);
    void clientDisconnected(const QString &name);
    void reactOnSocketError(const QString &error);

private:

    ChatServer *server;
    QList<QBluetoothHostInfo> localAdapters;

    QString localName;
};
//! [declaration]