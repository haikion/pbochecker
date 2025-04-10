#pragma once

#include <QString>

class PboCheckerPrinter
{
public:
    virtual ~PboCheckerPrinter() {};

    virtual void printHeader(const QString& bikey, const QString& mod = {}) = 0;
    virtual void printResult(const QString& fileName, bool success, const QString& extra) = 0;
    virtual void println(const QString& line) = 0;
    virtual void printWarn(const QString& line) = 0;
    virtual void printSuccessMsg() = 0;
    virtual void printFailureMsg() = 0;
};
