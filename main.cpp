#include "htlchatclient.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    htlChatClient w;
    w.show();
    return a.exec();
}
