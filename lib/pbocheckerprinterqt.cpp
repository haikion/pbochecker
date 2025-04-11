#include "pbocheckerprinterqt.h"

#include <QString>
#include <QStringLiteral>

using namespace Qt::StringLiterals;

void PboCheckerPrinterQt::printHeader(const QString& keysStr, const QString& mod)
{
    out_ << "\nVerification Report" << Qt::endl
         << "===================" << Qt::endl;
    if (!mod.isEmpty())
    {
        out_ << "Mod:  " << mod << Qt::endl;
    }
    // Keys: key1.bikey, key2.bikey
    // or
    // Key: key1.bikey
    out_  << keysStr << Qt::endl << Qt::endl;
}

void PboCheckerPrinterQt::printResult(const QString& fileName, bool success, const QString& extra)
{
    QString statusStr = success ? u" \033[32mOK\033[0m "_s : u"\033[31mFAIL\033[0m"_s;
    out_ << statusStr << " " << fileName;
    if (extra.isEmpty())
    {
        out_ << Qt::endl;
    }
    else
    {
        out_ << " (" << extra << ")" << Qt::endl;
    }
}

void PboCheckerPrinterQt::println(const QString& line) {
    out_ << line << Qt::endl;
}

void PboCheckerPrinterQt::printWarn(const QString& line) {
    out_ << "Warning: " << line << Qt::endl;
}

void PboCheckerPrinterQt::printSuccessMsg()
{
    out_ << "\nVerification successful\n";
}

void PboCheckerPrinterQt::printFailureMsg()
{
    out_ << "\nVerification failed\n";
}
