#pragma once

#include <functional>
#include <memory>
#include <tuple>

#include <QString>
#include <QStringList>
#include <QThread>

#include <openssl/bn.h>

#include "pbo.h"
#include "bikey.h"
#include "bisign.h"
#include "pbocheckerprinter.h"
#include "pbocheckerprinterqt.h"

class QFile;

class PboChecker
{
public:
    PboChecker();
    virtual ~PboChecker();

    std::tuple<bool, QString> checkPbo(const QString& pboPath,
                                 QString bikeyPath = {},
                                 QString bisignPath = {}) const;
    std::tuple<bool, QString> checkPbo(Pbo& pbo, const QString& bikeyPath, const QString& bisignPath) const;
    QString findBikeyPath(const QString& modPath) const;
    QString findBikeyPathWithPbo(const QString& pboFilePath) const;
    QString findBisignFilePath(const QString& pboFilePath) const;
    /**
    * @brief Validates all PBO files in a mod directory against their signatures
    * @param path Path to the mod directory containing addons and keys folders
    * @return A tuple containing a boolean indicating overall success (true if all valid) and a list of paths to corrupted PBOs
    */
    std::tuple<bool, QStringList> checkMod(const QString& path);
    bool checkModSet(const QString& path);
    void checkModAsync(const QString& path, std::function<void(bool, QStringList)> callback);
    void setVerbose(bool verbose);
    void setPrinter(std::unique_ptr<PboCheckerPrinter>&& printer);

private:
    QThread thread_;
    std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)> ctx_{BN_CTX_new(), &BN_CTX_free};
    std::unique_ptr<PboCheckerPrinter> printer_{std::make_unique<PboCheckerPrinterQt>()};
    std::atomic<bool> verbose_{false};
    QObject worker_;

    QByteArray nameHash(const QList<PboHeader>& headers) const;
    std::tuple<bool, QString> checkPboPriv(const QString& pboPath,
                                 QString bikeyPath = {},
                                 QString bisignPath = {}) const;
    static QString casedPath(const QString& path);
    QByteArray fileHash(QList<PboHeader>& headers, quint32 version, QFile& pboFile) const;
    std::optional<BiSign> readBisign(const QString& filePath) const;
    std::optional<QByteArray> readPboChecksum(QFile& file) const;
    std::optional<BiKey> readBiPublicKey(const QString& filePath) const;
};
