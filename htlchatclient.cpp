#include "htlchatclient.h"
#include "ui_htlchatclient.h"
#include <QSslSocket>
#include <QTimer>
#include <QFileDialog>
#include <QTextBrowser>
#include <QMessageBox>
#include "htlfiletransfer.h"

htlChatClient::htlChatClient(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::htlChatClient)
{
    ui->setupUi(this);
    mSocket = new QSslSocket(this);
    mIn.setDevice(mSocket);
    mIn.setVersion(QDataStream::Qt_5_0);
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(mSocket, SIGNAL(connected()), this, SLOT(connected()));
    connect(mSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(encrypted()), this, SLOT(encrypted()));
    connect(mSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(bytesWritten(qint64)));
    connect(ui->editMessage, SIGNAL(returnPressed()), this, SLOT(on_btnSend_clicked()));
    mTextBrowser = ui->textData;
    ui->lblChatroom->setText("Chatroom: Public");
    mCurrentChatroom = "";
    handleUiState(STATE_DISCONNECTED);
}

void htlChatClient::encrypted() {
    qDebug() << "Connection encrypted.";
    QStringList command;

    command.append("setUsername");
    command.append(mUsername);
    sendCommandList(command);

    command.clear();
    command.append("getUserList");
    sendCommandList(command);

    handleUiState(STATE_CONNECTED);
}

void htlChatClient::error(QAbstractSocket::SocketError error) {
    qDebug() << error;
    mSocket->abort();
    ui->btnConnect->setChecked(false);
    handleUiState(STATE_DISCONNECTED);
}

htlChatClient::~htlChatClient()
{
    delete ui;
}

void htlChatClient::sslErrors(QList<QSslError> errors) {
    qDebug() << errors;
}

void htlChatClient::on_btnConnect_clicked()
{
    if(!ui->btnConnect->isChecked()) {
        mSocket->disconnectFromHost();
        mSocket->waitForDisconnected();
        handleUiState(STATE_DISCONNECTED);
    } else {
        mUsername = ui->editUsername->text();
        if(mUsername.isEmpty()) {
            QMessageBox::information(this, "Error", "You must set a username.");
            ui->btnConnect->setChecked(false);
            return;
        }
        if(ui->editHost->text().isEmpty()) {
            QMessageBox::information(this, "Error", "You must specify a server name.");
            ui->btnConnect->setChecked(false);
            return;
        }

        mSocket->abort();
        mSocket->connectToHost(ui->editHost->text(), ui->spinPort->value());
        handleUiState(STATE_CONNECTING);
    }
}

void htlChatClient::handleUiState(UiState state) {
    switch(state) {
    case STATE_CONNECTED:
        ui->editHost->setEnabled(false);
        ui->spinPort->setEnabled(false);
        ui->editUsername->setEnabled(false);

        ui->btnSend->setEnabled(true);
        ui->pushButton->setEnabled(true);
        ui->editMessage->setEnabled(true);
        ui->btnPublicChatroom->setEnabled(true);

        ui->btnConnect->setText("Disconnect");
        break;
    case STATE_DISCONNECTED:
        ui->editHost->setEnabled(true);
        ui->spinPort->setEnabled(true);
        ui->editUsername->setEnabled(true);

        ui->btnSend->setEnabled(false);
        ui->pushButton->setEnabled(false);
        ui->editMessage->setEnabled(false);
        ui->btnPublicChatroom->setEnabled(false);
        ui->listUserlist->clear();

        ui->btnConnect->setText("Connect");
        break;
    case STATE_CONNECTING:
        ui->btnConnect->setText("Connecting");
        break;
    }
}

void htlChatClient::sendCommandList(QStringList commands) {
    if(!mSocket)
        return;

    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);
    out << commands;
    mSocket->write(data);
}

void htlChatClient::on_btnSend_clicked()
{
    QStringList commands;
    QString message = ui->editMessage->text();
    commands.append("sendMessage");
    commands.append(message);
    if(!mCurrentChatroom.isEmpty()) {
        commands.append(mCurrentChatroom);
    }
    ui->textData->append("me: " + message);
    sendCommandList(commands);
    ui->editMessage->setText("");
}

void htlChatClient::rebuildUserList() {
    ui->listUserlist->clear();
    ui->listUserlist->addItems(mUserlist);

    // Add new text browsers for new users
    foreach(QString user, mUserlist) {
        if(!mUsermapToTextMapping.contains(user)) {
            mUsermapToTextMapping.insert(user, new QTextBrowser(this));
        }
    }

    // Check if the currently selected chatroom still exists
    if(!mUserlist.contains(mCurrentChatroom)) {
        on_btnPublicChatroom_clicked();
    }

    // Remove text browsers not needed anymore
    foreach(QString user, mUsermapToTextMapping.keys()) {
        if(!mUserlist.contains(user)) {
            delete mUsermapToTextMapping.value(user);
            mUsermapToTextMapping.remove(user);
        }
    }
}

void htlChatClient::handlePrivateMessage(QString sender, QString message)
{
    qDebug() << "handlePrivateMessage" << sender << message;
    if(!mUsermapToTextMapping.contains(sender)) {
        return;
    }
    mUsermapToTextMapping.value(sender)->append(sender + ": " + message);
    if(mCurrentChatroom != sender) {
        setUserBold(sender, true);
    }
    qDebug() << "appended.";
}

void htlChatClient::setUserBold(QString sender, bool bold) {
    QList<QListWidgetItem*> item = ui->listUserlist->findItems(sender, Qt::MatchExactly);
    if(item.count() > 0) {
        QFont font;
        font.setBold(bold);
        item.at(0)->setFont(font);
    }
}

void htlChatClient::handlePublicMessage(QString sender, QString message)
{
    mTextBrowser->append(sender + ": " + message);
}

void htlChatClient::readyRead()
{
    mIn.startTransaction();

    QStringList commands;
    mIn >> commands;

    if(!mIn.commitTransaction())
        return;

    if(commands.isEmpty())
        return;

    qDebug() << commands;

    QString command = commands.at(0);

    if(command == "newMessage") {
        QString sender = commands.at(1);
        QString message = commands.at(2);
        QString type = commands.at(3);

        if(type == "private") {
            handlePrivateMessage(sender, message);
        } else {
            handlePublicMessage(sender, message);
        }
    } else if(command == "userDisconnected") {
        QString user = commands.at(1);
        mUserlist.removeAll(user);
        rebuildUserList();
        abortFileTransfer(user);
    } else if(command == "newUserConnected") {
        QString user = commands.at(1);
        mUserlist.append(user);
        rebuildUserList();
    } else if(command == "sendUserList") {
        mUserlist = commands.mid(1);
        mUserlist.removeAll(mUsername);
        rebuildUserList();
    } else if(command == "startFileTransfer") {
        QString user = commands.at(1);
        QString name = commands.at(2);
        QString size = commands.at(3);
        handleIncomingFileTransfer(user, name, size);
    } else if(command == "fileTransferAccepted") {
        startFileTransfer(commands.at(1));
    } else if(command == "fileTransferRejected") {
        abortFileTransfer(commands.at(1));
    } else if(command == "finishFileTransfer") {
        finishFileTransfer(commands.at(1));
    } else if(command == "sendChunk") {
        sendChunk(commands.at(1), commands.at(2));
    } else if(command == "welcomeToServer") {
        QStringList command;

        command.append("startEncryption");
        sendCommandList(command);

        mSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
        mSocket->waitForBytesWritten();
        mSocket->flush();
        mSocket->startClientEncryption();
    } else {
        qDebug() << "Received unknown command: ";
        qDebug() << commands;
    }

    if(mSocket->bytesAvailable() > 0) {
        QTimer::singleShot(0, this, SLOT(readyRead()));
    }
}

void htlChatClient::connected()
{

}

void htlChatClient::on_pushButton_clicked()
{
    if(ui->listUserlist->currentRow() < 0) {
        QMessageBox::information(this, "Error", "You must select a recipient.");
        return;
    }
    QString user = ui->listUserlist->currentItem()->text();
    if(user.isEmpty()) {
        QMessageBox::information(this, "Error", "You must select a recipient.");
        return;
    }
    if(mFileTransfers.contains(user)) {
        QMessageBox::information(this, "Error", "A file transfer is already active.");
        return;
    }
    QString fn = QFileDialog::getOpenFileName(this, "Send File...");
    QFile file(fn);
    if(file.exists()) {
        QMessageBox::StandardButton res = QMessageBox::question(this,
                                                                "Confirmation",
                                                                "Really send " + file.fileName() + " to " + user + "?");
        if(res == QMessageBox::Yes) {
            htlFileTransfer *transfer = new htlFileTransfer();
            transfer->setFilename(fn);
            mFileTransfers.insert(user, transfer);
            QStringList command;
            command.append("startFileTransfer");
            command.append(user);
            command.append(transfer->getFilename());
            command.append(QString::number(transfer->getFilesize()));
            sendCommandList(command);
        }
    }
}

void htlChatClient::on_listUserlist_itemClicked(QListWidgetItem *item)
{
    QString username = item->text();
    qDebug() << username;
    QTextBrowser *textBrowser = mUsermapToTextMapping.value(username);
    ui->textData->hide();
    ui->centralwidget->layout()->replaceWidget(ui->textData, textBrowser);
    ui->textData = textBrowser;
    ui->textData->show();
    ui->lblChatroom->setText("Chatroom: " + username);
    setUserBold(username, false);
    mCurrentChatroom = username;
}

void htlChatClient::on_btnPublicChatroom_clicked()
{
    ui->textData->hide();
    ui->centralwidget->layout()->replaceWidget(ui->textData, mTextBrowser);
    ui->textData = mTextBrowser;
    ui->textData->show();
    ui->lblChatroom->setText("Chatroom: Public");
    ui->listUserlist->clearSelection();
    mCurrentChatroom = "";
}

void htlChatClient::bytesWritten(qint64)
{
    qDebug() << "bytesWritten";
    foreach(QString user, mFileTransfers.keys()) {
        if(mFileTransfers.value(user)->isTransferring()) {
            sendNextChunk(user);
        }
    }
}

void htlChatClient::handleIncomingFileTransfer(QString user, QString name, QString size)
{
    QStringList command;
    QMessageBox::StandardButton response = QMessageBox::question(this,
                                                                 "Incoming file transfer",
                                                                 "There is an incoming file transfer from "
                                                                 + user + ": " + name + " Size: " + size
                                                                 + ". Accept?");
    if(response == QMessageBox::Yes) {
        QString fn = QFileDialog::getSaveFileName(this, "Save file...", name);
        if(fn.isEmpty()) {
            command.append("rejectFileTransfer");
        } else {
            command.append("acceptFileTransfer");
            htlFileTransfer *transfer = new htlFileTransfer();
            transfer->setFilename(fn);
            transfer->startIncomingTransfer();
            mFileTransfers.insert(user, transfer);
        }
    } else {
        command.append("rejectFileTransfer");
    }
    command.append(user);
    sendCommandList(command);
}

void htlChatClient::startFileTransfer(QString user)
{
    qDebug() << "startFileTransfer";
    mFileTransfers.value(user)->startTransfer();
    sendNextChunk(user);
}

void htlChatClient::abortFileTransfer(QString user)
{
    qDebug() << "abortFileTransfer";
    delete mFileTransfers.value(user);
    mFileTransfers.remove(user);
}

void htlChatClient::finishFileTransfer(QString user)
{
    qDebug() << "finishFileTransfer";
    mFileTransfers.value(user)->finishTransfer();
    delete mFileTransfers.value(user);
    mFileTransfers.remove(user);
}

void htlChatClient::sendChunk(QString user, QString data)
{
    qDebug() << "sendChunk";
    mFileTransfers.value(user)->appendChunk(data);
}

void htlChatClient::sendNextChunk(QString user)
{
    QStringList command;
    if(mFileTransfers.value(user)->chunkAvailable()) {
        qDebug() << "sendChunk";
        QString chunk = mFileTransfers.value(user)->getNextChunk();
        command.append("sendChunk");
        command.append(user);
        command.append(chunk);
    } else {
        qDebug() << "finishFileTransfer";
        command.append("finishFileTransfer");
        command.append(user);
        mFileTransfers.value(user)->finishTransfer();
        delete mFileTransfers.value(user);
        mFileTransfers.remove(user);
    }
    sendCommandList(command);
}
