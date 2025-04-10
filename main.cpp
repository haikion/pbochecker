#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringLiteral>

#include "pbochecker.h"

using namespace Qt::StringLiterals;

void printHelp(const QString& appName)
{
    QTextStream(stdout) << "Usage: " << appName << " <pbo> [bikey] [bisign]\n"
                        << "       " << appName << " <mod_directory>\n"
                        << "       " << appName << " <mod_set_directory>\n\n"
                        << "Arguments:\n"
                        << "  <pbo>               Path to the PBO file\n"
                        << "  [bikey]             Path to the public key file (optional)\n"
                        << "  [bisign]            Path to the bisign file (optional)\n"
                        << "  <mod_directory>     Path to the mod directory (starting with @)\n"
                        << "  <mod_set_directory> Path to a parent directory containing mod directories\n\n"
                        << "Options:\n"
                        << "  --help          Show this help message\n"
                        << "  --verbose       Enable verbose output\n"
                        << "  --version       Print version information\n";
}

int main(int argc, char *argv[])
{
    using std::get;

    QCoreApplication app{argc, argv};
    PboChecker pboChecker;
    const auto exeName = QFileInfo{QCoreApplication::applicationFilePath()}.fileName();
    app.setApplicationVersion(u"0.1"_s);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addOption(QCommandLineOption(u"verbose"_s, u"Enable verbose output"_s));
    parser.addOption(QCommandLineOption(u"help"_s, u"Show help"_s));
    parser.process(app);

    if (parser.isSet(u"verbose"_s)) {
        pboChecker.setVerbose(true);
    }
    if (parser.isSet(u"help"_s)) {
        printHelp(exeName);
        return 0;
    }

    const QStringList args = parser.positionalArguments();
    if (args.size() == 0)
    {
        printHelp(exeName);
        return 1;
    }
    if (args.size() == 1)
    {
        const auto path = args.at(0);
        QFileInfo fileInfo{path};
        if (fileInfo.isFile() && fileInfo.suffix().toLower() == "pbo"_L1)
        {
            return get<0>(pboChecker.checkPbo(path)) ? 0 : 1;
        }
        if (fileInfo.isDir())
        {
            if (fileInfo.fileName().startsWith('@'_L1))
            {
                return get<0>(pboChecker.checkMod(path));
            }
            else
            {
                return pboChecker.checkModSet(path) ? 0 : 1;;
            }
        }
        QTextStream(stderr) << "Error: " << path << " is neither a PBO file nor a mod directory" << Qt::endl;
        printHelp(exeName);
        return 1;
    }
    if (args.size() == 2)
    {
        const QString bikeyPath = args.at(0);
        const QString pboPath = args.at(1);
        return get<0>(pboChecker.checkPbo(pboPath, bikeyPath)) ? 0 : 1;
    }
    if (args.size() == 3)
    {
        const QString pboPath = args.at(0);
        const QString bikeyPath = args.at(1);
        const QString bisignPath = args.at(2);
        return get<0>(pboChecker.checkPbo(pboPath, bikeyPath, bisignPath))? 0 : 1;
    }
    QTextStream(stderr) << "Error: Too many arguments" << Qt::endl;
    printHelp(exeName);
    return 1;
}
