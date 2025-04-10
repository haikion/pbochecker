#pragma once

#include <QByteArray>
#include <QtGlobal>

struct PboHeader
{
    QByteArray fileName;
    // Start of the data block
    qint64 dataStart{-1};
    // Size of the data block
    quint32 dataSize{0};
};

