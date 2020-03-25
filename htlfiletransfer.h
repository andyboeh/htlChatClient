#ifndef HTLFILETRANSFER_H
#define HTLFILETRANSFER_H

#include <QFile>

class htlFileTransfer
{
public:
    htlFileTransfer();
    void setFilename(QString filename);
    QString getNextChunk();
    bool appendChunk(QString chunk);
    void finishTransfer();
    bool chunkAvailable();
    bool startTransfer();
    bool startIncomingTransfer();
    bool isTransferring();
    qint64 getFilesize();
    QString getFilename();
private:
    QFile mFile;
    qint64 mChunkSize;
    bool mIsOutgoingTransfer;
    qint64 mFilePointer;
};

#endif // HTLFILETRANSFER_H
