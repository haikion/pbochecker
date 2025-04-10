#pragma once

#include <QByteArray>
#include <QtGlobal>

struct BiSign
{
    quint32 version{0};
    QByteArray sig1{128, 0};
    QByteArray sig2{128, 0};
    QByteArray sig3{128, 0};
};
