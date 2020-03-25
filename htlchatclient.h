#ifndef HTLCHATCLIENT_H
#define HTLCHATCLIENT_H

#include <QMainWindow>
#include <QDataStream>
#include <QList>
#include <QSslError>
#include <QAbstractSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class htlChatClient; }
QT_END_NAMESPACE

class QSslSocket;
class QListWidgetItem;
class QTextBrowser;
class htlFileTransfer;

class htlChatClient : public QMainWindow
{
    Q_OBJECT

public:
    htlChatClient(QWidget *parent = nullptr);
    ~htlChatClient();

private slots:
    void on_btnConnect_clicked();

    void on_btnSend_clicked();

    void readyRead();

    void connected();

    void sslErrors(QList<QSslError> errors);
    void error(QAbstractSocket::SocketError error);
    void encrypted();
    void on_pushButton_clicked();

    void on_listUserlist_itemClicked(QListWidgetItem *item);

    void on_btnPublicChatroom_clicked();
    void bytesWritten(qint64);
private:
    Ui::htlChatClient *ui;
    QSslSocket *mSocket;
    QMap<QString,QTextBrowser*> mUsermapToTextMapping;
    QDataStream mIn;
    QStringList mUserlist;
    QString mUsername;
    QTextBrowser *mTextBrowser;
    QString mCurrentChatroom;
    QMap<QString, htlFileTransfer*> mFileTransfers;
    void handleIncomingFileTransfer(QString user, QString name, QString size);
    void startFileTransfer(QString user);
    void abortFileTransfer(QString user);
    void finishFileTransfer(QString user);
    void sendChunk(QString user, QString data);
    void sendNextChunk(QString user);
    void rebuildUserList();
    void sendCommandList(QStringList commands);
    void handlePrivateMessage(QString sender, QString message);
    void handlePublicMessage(QString sender, QString message);
};
#endif // HTLCHATCLIENT_H
