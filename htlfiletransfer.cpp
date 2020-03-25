#include "htlfiletransfer.h"

htlFileTransfer::htlFileTransfer()
{
    mChunkSize = 16384;
    mIsOutgoingTransfer = false;
    mFilePointer = 0;
}

void htlFileTransfer::setFilename(QString filename)
{
    mFile.setFileName(filename);
}

QString htlFileTransfer::getNextChunk()
{
    if(!mFile.isOpen()) {
        return "";
    }
    QByteArray data = mFile.read(mChunkSize);
    mFilePointer += data.size();
    QString ret(data.toBase64());
    return ret;
}

bool htlFileTransfer::appendChunk(QString chunk)
{
    if(!mFile.isOpen())
        return false;
    QByteArray data = QByteArray::fromBase64(chunk.toLatin1());
    mFile.write(data);
    return true;
}

void htlFileTransfer::finishTransfer()
{
    mFile.close();
}

bool htlFileTransfer::chunkAvailable()
{
    if(mFile.exists()) {
        if(mFile.isOpen() && mIsOutgoingTransfer) {
            if(mFilePointer < mFile.size())
                return true;
            //if(mFile.bytesAvailable() > 0)
            //    return true;
        }
    }
    return false;
}

bool htlFileTransfer::startTransfer()
{
    if(mFile.exists()) {
        if(mFile.open(QIODevice::ReadOnly)) {
            mIsOutgoingTransfer = true;
            return true;
        }
    }
    return false;
}

bool htlFileTransfer::startIncomingTransfer()
{
    if(mFile.exists())
        return false;
    if(mFile.open(QIODevice::WriteOnly)) {
        mIsOutgoingTransfer = false;
        return true;
    }
    return false;
}

bool htlFileTransfer::isTransferring()
{
    if(mIsOutgoingTransfer && mFile.isOpen())
        return true;
    return false;
}

qint64 htlFileTransfer::getFilesize()
{
    return mFile.size();
}

QString htlFileTransfer::getFilename()
{
    return mFile.fileName();
}

