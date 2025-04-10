#pragma once

#include <QByteArray>
#include <QtGlobal>

struct BiKey
{
    quint32 length{0};
    quint32 byteLength{0};
    quint32 exponent{0};
    QByteArray m;
};
