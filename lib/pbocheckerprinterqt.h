#pragma once

#include <QString>
#include <QTextStream>

#include "pbocheckerprinter.h"

class PboCheckerPrinterQt: public PboCheckerPrinter
{
public:
    PboCheckerPrinterQt() = default;

    void printHeader(const QString& keysStr, const QString& mod) override;
    void printResult(const QString& fileName, bool success, const QString& reason = {}) override;
    void println(const QString& line) override;
    void printWarn(const QString& line) override;
    void printSuccessMsg() override;
    void printFailureMsg() override;

private:
    QTextStream out_{stdout};
};
