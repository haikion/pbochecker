#pragma once

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>
#include <QtGlobal>

#include "pboheader.h"

struct Pbo
{
    QString filePath;
    QMap<QByteArray, QByteArray> headerExtensions;
    QList<PboHeader> headers;
    QByteArray checksum;
    QByteArray nameHash;
    QByteArray fileHash;
};
