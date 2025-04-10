#include "pbochecker.h"

#include <QByteArray>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QStringLiteral>
#include <QTextStream>
#include <QtAssert>
#include <QtTypes>

#include <algorithm>
#include <functional>
#include <optional>

#include "bikey.h"
#include "bisign.h"
#include "pbo.h"

using std::function;
using std::tuple;
using std::get;

namespace
{
using namespace Qt::StringLiterals;

using std::nullopt;
using std::optional;
using std::pair;
using std::reverse;
using std::sort;
using std::unique_ptr;

unique_ptr<BIGNUM, decltype(&BN_free)> toBIGNUM(const QByteArray& byteArray)
{
    Q_ASSERT(byteArray.size() > 0);
    BIGNUM* bn = BN_bin2bn(reinterpret_cast<const unsigned char*>(byteArray.constData()), byteArray.size(), nullptr);
    return unique_ptr<BIGNUM, decltype(&BN_free)>{bn, &BN_free};
}

unique_ptr<BIGNUM, decltype(&BN_free)> toBIGNUM(quint32 value)
{
    auto bn = unique_ptr<BIGNUM, decltype(&BN_free)>{BN_new(), &BN_free};
    BN_set_word(bn.get(), value);
    return bn;
}

QString fillHeaders(Pbo& pbo, QFile& file)
{
    file.seek(1);
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    const QByteArray buffer = file.read(4);
    if (buffer == "sreV")
    {
        file.seek(21);
        while (!file.atEnd())
        {
            QByteArray key;
            char ch;
            while (file.getChar(&ch) && ch != '\0')
            {
                key.append(ch);
            }
            if (key.isEmpty())
            {
                break;
            }
            QByteArray value;
            while (file.getChar(&ch) && ch != '\0')
            {
                value.append(ch);
            }
            pbo.headerExtensions[key] = value;
        }
    }
    else
    {
        file.seek(0);
    }
    // Read headers
    while (!file.atEnd())
    {
        QByteArray name;
        char ch;
        while (file.getChar(&ch) && ch != '\0')
        {
            name.append(ch);
        }
        if (name.isEmpty())
        {
            in.skipRawData(20);
            break;
        }
        // https://community.bistudio.com/wiki/PBO_File_Format#PBO_Header_Entry
        PboHeader header;
        header.fileName = name;
        // Skip packingMethod (4 bytes)
        // Skip originalSize (4 bytes)
        // Skip offset (4 bytes)
        // Skip timeStamp (4 bytes)
        in.skipRawData(16);
        in >> header.dataSize;
        pbo.headers.append(header);
    }
    qint64 dataBlockStart = file.pos();
    for (auto& header : pbo.headers)
    {
        header.dataStart = dataBlockStart;
        dataBlockStart += header.dataSize;
    }
    const auto remainingBytes = file.size() - dataBlockStart;
    return remainingBytes == 21 ? QString{}
        : QString{"Incorrect amount of bytes after data block: " + QString::number(remainingBytes)};
}

QByteArray padHash(const QByteArray& hash, quint32 size)
{
    return QByteArray::fromRawData("\x01", 1)
           + QByteArray{size - 38, '\xFF'}
           + QByteArray::fromRawData("\x00\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14", 16)
           + hash;
}

QByteArray generateHash2(const Pbo& pbo, quint32 version, quint32 length)
{
    Q_ASSERT(!pbo.checksum.isEmpty());
    Q_ASSERT(!pbo.nameHash.isEmpty());

    QCryptographicHash hasher2{QCryptographicHash::Sha1};
    hasher2.addData(pbo.checksum);
    hasher2.addData(pbo.nameHash);
    if (pbo.headerExtensions.contains("prefix"))
    {
        auto prefix = pbo.headerExtensions["prefix"];
        hasher2.addData(prefix);
        if (!prefix.endsWith('\\'))
        {
            hasher2.addData("\\"_ba);
        }
    }
    auto hash2 = hasher2.result();
    return padHash(hash2, length);
}

QByteArray generateHash3(const Pbo& pbo, quint32 version, quint32 length)
{
    Q_ASSERT(!pbo.nameHash.isEmpty());
    Q_ASSERT(!pbo.fileHash.isEmpty());

    QCryptographicHash hasher3{QCryptographicHash::Sha1};
    hasher3.addData(pbo.fileHash);
    hasher3.addData(pbo.nameHash);
    const auto prefix = pbo.headerExtensions.find("prefix");
    if (prefix != pbo.headerExtensions.end())
    {
        hasher3.addData(*prefix);
        if (!prefix->endsWith('\\'))
        {
            hasher3.addData("\\"_ba);
        }
    }
    return padHash(hasher3.result(), length);
}

bool checkSign1(BN_CTX* ctx, const BiKey& publicKey, const QByteArray& sig1Bytes, const QByteArray& hash1)
{
    Q_ASSERT(!publicKey.m.isEmpty());
    Q_ASSERT(!sig1Bytes.isEmpty());
    Q_ASSERT(ctx);
    Q_ASSERT(publicKey.exponent > 0);
    Q_ASSERT(publicKey.length > 0);

    auto realHash1Bytes = padHash(hash1, publicKey.byteLength);
    auto realHash1 = toBIGNUM(realHash1Bytes);
    auto sig1 = toBIGNUM(sig1Bytes);
    auto exponent = toBIGNUM(publicKey.exponent);
    auto m = toBIGNUM(publicKey.m);
    auto signedHash1 = unique_ptr<BIGNUM, decltype(&BN_free)>{BN_new(), &BN_free};
    BN_mod_exp(signedHash1.get(), sig1.get(), exponent.get(), m.get(), ctx);
    bool match = BN_cmp(realHash1.get(), signedHash1.get()) == 0;
    return match;
}

bool checkSig2(BN_CTX* ctx, const BiKey& publicKey, const BiSign& bisign, const Pbo& pbo)
{
    Q_ASSERT(!publicKey.m.isEmpty());
    Q_ASSERT(ctx);
    Q_ASSERT(publicKey.exponent > 0);
    Q_ASSERT(publicKey.length > 0);

    auto realHash2Bytes = generateHash2(pbo, bisign.version, publicKey.byteLength);
    auto realHash2 = toBIGNUM(realHash2Bytes);
    auto sig2 = toBIGNUM(bisign.sig2);
    auto exponent = toBIGNUM(publicKey.exponent);
    auto m = toBIGNUM(publicKey.m);
    auto signedHash2 = unique_ptr<BIGNUM, decltype(&BN_free)>{BN_new(), &BN_free};
    BN_mod_exp(signedHash2.get(), sig2.get(), exponent.get(), m.get(), ctx);
    bool match = BN_cmp(realHash2.get(), signedHash2.get()) == 0;
    return match;
}

bool checkSig3(BN_CTX* ctx, const BiKey& publicKey, const BiSign& bisign, const Pbo& pbo)
{
    Q_ASSERT(!publicKey.m.isEmpty());
    Q_ASSERT(ctx);
    Q_ASSERT(publicKey.exponent > 0);
    Q_ASSERT(publicKey.length > 0);
    Q_ASSERT(!pbo.nameHash.isEmpty());
    Q_ASSERT(!pbo.fileHash.isEmpty());

    auto realHash3Bytes = generateHash3(pbo, bisign.version, publicKey.byteLength);
    auto realHash3 = toBIGNUM(realHash3Bytes);
    auto sig3 = toBIGNUM(bisign.sig3);
    auto exponent = toBIGNUM(publicKey.exponent);
    auto m = toBIGNUM(publicKey.m);
    auto signedHash3 = unique_ptr<BIGNUM, decltype(&BN_free)>{BN_new(), &BN_free};
    BN_mod_exp(signedHash3.get(), sig3.get(), exponent.get(), m.get(), ctx);
    bool match = BN_cmp(realHash3.get(), signedHash3.get()) == 0;
    return match;
}
} // namespace

PboChecker::PboChecker()
{
    thread_.setObjectName("PboChecker thread");
    worker_.moveToThread(&thread_);
}

PboChecker::~PboChecker()
{
    thread_.requestInterruption();
    thread_.quit();
    thread_.wait();
}

tuple<bool, QString> PboChecker::checkPbo(const QString& pboPath,
                                               QString bikeyPath,
                                               QString bisignPath) const
{
    tuple<bool, QString> retVal;
    if (bikeyPath.isEmpty())
    {
        bikeyPath = findBikeyPathWithPbo(pboPath);
        if (bikeyPath.isEmpty())
        {
            retVal = {false, u"bikey not found"_s};
        }
    }
    printer_->printHeader(QFileInfo{bikeyPath}.fileName());
    retVal = checkPboPriv(pboPath, bikeyPath, bisignPath);
    printer_->printResult(QFileInfo{pboPath}.fileName(), get<0>(retVal), get<1>(retVal));
    if (get<0>(retVal))
    {
        printer_->printSuccessMsg();
    }
    else
    {
        printer_->printFailureMsg();
    }
    return retVal;
}

tuple<bool, QString> PboChecker::checkPboPriv(const QString& pboPath, QString bikeyPath, QString bisignPath) const
{
    if (bikeyPath.isEmpty())
    {
        bikeyPath = findBikeyPathWithPbo(pboPath);
        if (bikeyPath.isEmpty())
        {
            return {false, u"bikey not found"_s};
        }
    }
    if (bisignPath.isEmpty()) {
        bisignPath = findBisignFilePath(pboPath);
        if (bisignPath.isEmpty())
        {
            return {false, u"bisign not found"_s};
        }
    }
    Pbo pbo;
    pbo.filePath = pboPath;
    return checkPbo(pbo, bikeyPath, bisignPath);
}

tuple<bool, QString> PboChecker::checkPbo(Pbo& pbo, const QString& bikeyPath, const QString& bisignPath) const
{
    optional<BiKey> publicKeyOpt = readBiPublicKey(bikeyPath);
    if (!publicKeyOpt.has_value())
    {
        return {false, "Failed to read public key: " + bikeyPath};
    }
    auto publicKey = publicKeyOpt.value();
    optional<BiSign> bisignOpt = readBisign(bisignPath);
    if (!bisignOpt.has_value())
    {
        return {false, "Failed to read bisign file: " + bisignPath};
    }
    const auto& bisign = bisignOpt.value();
    QFile pboFile{pbo.filePath};
    if (!pboFile.open(QIODevice::ReadOnly))
    {
        return {false, "Failed to open PBO file: " + pbo.filePath};
    }
    optional<QByteArray> checksumOpt = readPboChecksum(pboFile);
    if (!checksumOpt.has_value())
    {
        return {false, "Failed to read checksum from PBO file:" + pbo.filePath};
    }
    pbo.checksum = checksumOpt.value();
    if (!checkSign1(ctx_.get(), publicKey, bisign.sig1, pbo.checksum))
    {
        return {false, "Signature 1 mismatch"};
    }
    auto error = fillHeaders(pbo, pboFile);
    if (!error.isEmpty())
    {
        return {false, error};
    }
    pbo.nameHash = nameHash(pbo.headers);
    if (pbo.nameHash.isEmpty())
    {
        return {true, "Signature checks 2 and 3 disabled"};
    }
    if (!checkSig2(ctx_.get(), publicKey, bisign, pbo))
    {
        return {false, "Signature 2 mismatch"};
    }
    pbo.fileHash = fileHash(pbo.headers, bisign.version, pboFile);
    if (verbose_)
    {
        printer_->println("File Hash: " + pbo.fileHash.toHex().toUpper());
    }
    if (pbo.fileHash.isEmpty()) {
        return {true, "Signature 3 unsupported"};
    }
    const bool success = checkSig3(ctx_.get(), publicKey, bisign, pbo);
    if (success)
    {
        return {true, {}};
    }
    return {false, "Signature 3 mismatch"};
}

QString PboChecker::findBikeyPath(const QString& modPath) const
{
    const QString cPath = casedPath(modPath + "/keys");
    QDir modDir{cPath};
    modDir.setNameFilters({"*.bikey"});
    QStringList bikeyFiles = modDir.entryList(QDir::Files | QDir::NoSymLinks);
    return bikeyFiles.isEmpty() ? QString{} : modDir.absoluteFilePath(bikeyFiles.first());
}

QString PboChecker::findBikeyPathWithPbo(const QString& pboFilePath) const
{
    // Look for bikey files in ../keys directory relative to the PBO file
    QFileInfo pboInfo(pboFilePath);
    const auto cPath = casedPath(pboInfo.absoluteDir().absolutePath() + "/../keys");
    QDir keysDir{cPath};

    if (keysDir.exists())
    {
        keysDir.setNameFilters({"*.bikey"});
        QStringList bikeyFiles = keysDir.entryList(QDir::Files | QDir::NoSymLinks);
        if (!bikeyFiles.isEmpty())
        {
            return keysDir.absoluteFilePath(bikeyFiles.first());
        }
    }
    printer_->printWarn("No .bikey file found in " + keysDir.absolutePath());
    return {};
}

QString PboChecker::findBisignFilePath(const QString& pboFilePath) const
{
    const QFileInfo pboFileInfo(pboFilePath);
    QDir pboDir = pboFileInfo.absoluteDir();
    const QString pboBaseName = pboFileInfo.completeBaseName();
    QStringList filters;
    filters << pboBaseName + "*.bisign";
    pboDir.setNameFilters(filters);
    QStringList bisignFiles = pboDir.entryList();
    return bisignFiles.isEmpty() ? QString{} : pboDir.absoluteFilePath(bisignFiles.first());
}

bool PboChecker::checkModSet(const QString& path)
{
    QDir modSetDir{path};
    const auto modDirs = modSetDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    bool allValid = true;
    for (const auto& modDir : modDirs)
    {
        if (modDir.startsWith('@'))
        {
            const auto modPath = modSetDir.absoluteFilePath(modDir);
            // Success return value is false when bikey is not found
            // so check if corrupted pbo amount is 0
            allValid &= get<1>(checkMod(modPath)).isEmpty();
        }
    }
    printer_->println(u"\nOverall result: "_s + (allValid ? u"Success"_s : u"Failure"_s));
    return allValid;
}

tuple<bool, QStringList> PboChecker::checkMod(const QString& path)
{
    const auto modPath = path;
    const auto bikeyPath = findBikeyPath(modPath);
    if (bikeyPath.isEmpty())
    {
        printer_->printWarn("\nUnable to find bikey for " + QFileInfo{path}.fileName());
        return {false, {}};
    }
    const auto cPath = PboChecker::casedPath(modPath + "/addons");
    QDir modDir{cPath};
    modDir.setNameFilters({"*.pbo"});
    const QStringList pboFiles = modDir.entryList(QDir::Files | QDir::NoSymLinks);

    if (pboFiles.isEmpty())
    {
        printer_->println("No PBO files found in " + modPath + "/addons\n");
        return {false, {}};;
    }

    qint64 totalSize = 0;
    for (const auto& pboFile : pboFiles)
    {
        QFile pboF{cPath + '/' + pboFile};
        totalSize += pboF.size();
    }
    if (totalSize <= 0)
    {
        printer_->printWarn("PBO files are empty\n");
        return {false, {}};
    }

    printer_->printHeader(QFileInfo{bikeyPath}.fileName(), QFileInfo{path}.fileName());

    QStringList brokenPboPaths;
    bool allSuccess = true;
    for (const auto& pboFile : pboFiles)
    {
        const auto filePath = cPath + '/' + pboFile;
        const auto [success,  extra] = checkPboPriv(filePath, bikeyPath);
        if (!success)
        {
            allSuccess = false;
            brokenPboPaths.append(filePath);
        }
        const auto fileName = QFileInfo(filePath).fileName();
        printer_->printResult(fileName, success, extra);
    }
    if (allSuccess)
    {
        printer_->printSuccessMsg();
    }
    else
    {
        printer_->printFailureMsg();
    }
    return {brokenPboPaths.isEmpty(), brokenPboPaths};
}

void PboChecker::checkModAsync(const QString& path, function<void(bool, QStringList)> callback)
{
    const auto callerThread = QThread::currentThread();
    thread_.start(); // If the thread is already running, this function does nothing.
    QMetaObject::invokeMethod(&worker_, [=, this] () {
        Q_ASSERT(QThread::currentThread() == &thread_);
        if (thread_.isInterruptionRequested())
        {
            return;
        }
        auto result = checkMod(path);
        QMetaObject::invokeMethod(callerThread, [=] () {
            callback(get<0>(result), get<1>(result));
        }, Qt::QueuedConnection);
    }, Qt::QueuedConnection);
}

void PboChecker::setVerbose(bool verbose)
{
    verbose_ = verbose;
}

void PboChecker::setPrinter(unique_ptr<PboCheckerPrinter>&& printer)
{
    printer_ = std::move(printer);
}

QString PboChecker::casedPath(const QString& path)
{
    QString pathCi = QFileInfo(path).absoluteFilePath();
    //Construct case sentive path by comparing file or dir names to case sentive ones.
    QStringList ciNames = pathCi.split('/');
    QString casedPath = ciNames.first().endsWith(':') ? ciNames.first() + "/"
                                                                     : u"/"_s;
    ciNames.removeFirst(); //Drive letter on windows or "" on linux.

    for (const QString& ciName : ciNames)
    {
        QDirIterator it(casedPath);
        while (true)
        {
            if (!it.hasNext())
            {
                return {};
            }
            QFileInfo fi = it.nextFileInfo();
            if (fi.fileName().toUpper() == ciName.toUpper())
            {
                casedPath += (casedPath.endsWith('/') ? "" : "/") + fi.fileName();
                break;
            }
        }
    }
    return casedPath;
}

QByteArray PboChecker::nameHash(const QList<PboHeader>& headers) const
{
    Q_ASSERT(!headers.isEmpty());

    QList<pair<QByteArray, bool>> pairs;
    for (const auto& header: headers)
    {
        pairs.append({header.fileName.toLower(), header.dataSize == 0});
    }
    sort(pairs.begin(), pairs.end(), [] (const auto& a, const auto& b) {
        return a.first < b.first;
    });
    QCryptographicHash hasher{QCryptographicHash::Sha1};
    for (const auto& [name, isEmpty] : pairs)
    {
        // @faces_of_war fails signature check
        // so keep this check until the real cause is found
        if (name.trimmed().isEmpty())
        {
            if (verbose_)
            {
                printer_->printWarn("Odd file name detected: " + name  + " . Signature checks 2 and 3 disabled.");
            }
            return {};
        }

        if (!isEmpty)
        {
            hasher.addData(name);
            if (verbose_)
            {
                printer_->println("Hashing name: " + name);
            }
        }
        else if (verbose_)
        {
            printer_->println("Empty file: " + name);
        }
    }
    return hasher.result();
}

/**
 * @brief Computes the hash of the file contents based on the headers and version.
 *
 * This function reads data blocks from pboFile according to the
 * pboHeader's dataStart and dataSize member variables. The data is read in a lazy manner meaning that
 * hash is calculated while the data is being read.
 *
 * @param headers List of PboHeader objects containing file metadata.
 * @param version Version of the PBO format.
 * @param pboFile QFile object representing the PBO file.
 * @return QByteArray containing the computed hash.
 */
QByteArray PboChecker::fileHash(QList<PboHeader>& headers, quint32 version, QFile& pboFile) const
{
    Q_ASSERT(version == 2 || version == 3);
    Q_ASSERT(!headers.isEmpty());
    Q_ASSERT(pboFile.isOpen());

    QCryptographicHash hasher{QCryptographicHash::Sha1};
    // sort(headers.begin(), headers.end(), alphabeticallyBis);
    bool nothing = true;
    static const QSet<QByteArray> v2ExcludeExtensions{"paa", "jpg", "p3d", "tga", "rvmat", "lip", "ogg", "wss", "png", "rtm", "pac", "fxy", "wrp"};
    static const QSet<QByteArray> v3Extensions{"sqf", "sqfc", "inc", "bikb", "ext", "fsm", "sqm", "hpp", "cfg", "sqs", "h"};
    QSet<QByteArray> ignored;
    for (const auto& header : headers)
    {
        const auto splits = header.fileName.split('.');
        const auto ext = splits.last().toLower();
        if (header.dataSize > 0
            && ((version == 2 && !v2ExcludeExtensions.contains(ext))
                || (version == 3 && v3Extensions.contains(ext))))
        {
            nothing = false;
            Q_ASSERT(header.dataStart > 0);
            pboFile.seek(header.dataStart);
            QByteArray data = pboFile.read(header.dataSize);
            if (verbose_)
            {
                printer_->println("Hashing file: " + header.fileName);
            }
            hasher.addData(data);
        }
    }
    if (nothing)
    {
        hasher.addData(version == 2 ? "nothing"_ba : "gnihton"_ba);
    }
    return hasher.result();
}

/**
 * @brief readBisign
 * Reads signature file which consists of
 * - name                               (NULL terminated)
 * - padding                            (16 bytes)
 * - key_length                         (4 bytes)
 * - exponent                           (4 bytes)
 * - modulus                            (key_length / 8 bytes)
 * - padding                            (5 * (key_length/16) bytes)
 * - signature 1                        (key_length / 8 bytes)
 * - BiSign version                     (4 bytes)
 * - padding                            (4 bytes)
 * - signature 2                        (key length / 8 bytes)
 * - padding                            (4 bytes)
 * - signature 3                        (key_length / 8 bytes)
 * Example: (keylength = 1028)
 *   18 bytes: cba_3.17.1.240424 name
 *   16 bytes: padding
 *   4 bytes: key length (1028)
 *   4 bytes: exponent (65537)
 *   128 bytes: modulus
 *   4 bytes: padding
 *   128 bytes: signature 1
 *   8 bytes: padding
 *   128 bytes: signature 2
 *   4 bytes: padding
 *  128 bytes: signature 3
 * @param file
 * @return parsed BiSign object
 */
optional<BiSign> PboChecker::readBisign(const QString& filePath) const
{
    BiSign bisign;
    QFile file{filePath};
    if (!file.open(QIODevice::ReadOnly))
    {
        printer_->printWarn("Failed to open bisign file: " + file.fileName());
        return nullopt;
    }
    // Read name with null terminator
    QDataStream stream{&file};
    stream.setByteOrder(QDataStream::LittleEndian);
    // Read name as a NULL terminated string
    char ch;
    while (stream.readRawData(&ch, 1) > 0 && ch != '\0')
    {
    }
    // Skip key_length / 8 + 20 (4 bytes)
    // Skip 0602 0000 0024 0000  (8 bytes)
    // Skip RSA1 text (4 bytes)
    stream.skipRawData(16);

    // Read keylength and exponent
    quint32 keylength;
    stream >> keylength;
    stream.skipRawData(4);  // stream >> bisign.exponent;
    if (keylength > 5000)
    {
        printer_->printWarn("Key length is too large (" + QString::number(keylength) + "B). Probably corrupted signature file ... pos:" + QString::number(file.pos()));
        return nullopt;
    }

    // Skip modulus (usually 128 bytes)
    // Skip padding (4 bytes)
    const auto keyByteSize = keylength / 8;
    stream.skipRawData(keyByteSize + 4);

    // Read signature values
    bisign.sig1.resize(keyByteSize);
    if (file.bytesAvailable() >= keyByteSize && stream.readRawData(bisign.sig1.data(), keyByteSize) <= 0)
    {
        printer_->printWarn("Failed to read signature 1");
        return nullopt;
    }
    reverse(bisign.sig1.begin(), bisign.sig1.end());

    // Read signature version
    if (file.bytesAvailable() < 8)
    {
        printer_->printWarn("Failed to read signature version");
        return nullopt;
    }
    stream >> bisign.version;
    stream.skipRawData(4);

    bisign.sig2.resize(keyByteSize);
    if (file.bytesAvailable() >= keyByteSize && stream.readRawData(bisign.sig2.data(), keyByteSize) < keyByteSize)
    {
        printer_->printWarn("Failed to read signature 2");
        return nullopt;
    }
    reverse(bisign.sig2.begin(), bisign.sig2.end());

    stream.skipRawData(4);

    bisign.sig3.resize(keyByteSize);
    if (file.bytesAvailable() >= keyByteSize && stream.readRawData(bisign.sig3.data(), keyByteSize) < keyByteSize)
    {
        printer_->printWarn("Failed to read signature 3");
        return nullopt;
    }
    reverse(bisign.sig3.begin(), bisign.sig3.end());
    Q_ASSERT(stream.atEnd());
    return bisign;
}

optional<QByteArray> PboChecker::readPboChecksum(QFile& file) const
{
    if (file.bytesAvailable() < 20)
    {
        printer_->printWarn("File is too small to contain checksum");
        return nullopt;
    }
    file.seek(file.size() - 20);
    return file.read(20);
}

optional<BiKey> PboChecker::readBiPublicKey(const QString& filePath) const
{
    QFile file{filePath};
    BiKey publicKey;
    if (!file.open(QIODevice::ReadOnly))
    {
        printer_->printWarn("Failed to open public key file: " + file.fileName());
        return nullopt;
    }
    QDataStream in{&file};
    in.setByteOrder(QDataStream::LittleEndian);

    // Skip name as a null-terminated string
    char ch;
    while (in.readRawData(&ch, 1) == 1 && ch != '\0')
    {
    }
    if (file.bytesAvailable() < 24)
    {
        printer_->printWarn("File is too small to contain key length and exponent");
        return nullopt;
    }
    quint32 keyByteLengthFile;
    in >> keyByteLengthFile;
    in.skipRawData(12);
    in >> publicKey.length;
    publicKey.byteLength = publicKey.length / 8;
    if (keyByteLengthFile != publicKey.byteLength + 20)
    {
        printer_->printWarn("Invalid length of " + file.fileName());
        return nullopt;
    }
    in >> publicKey.exponent;

    publicKey.m.resize(publicKey.byteLength);
    if (in.readRawData(publicKey.m.data(), publicKey.m.size()) != publicKey.m.size())
    {
        printer_->printWarn("Failed to read m from " + file.fileName());
        return nullopt;
    }
    reverse(publicKey.m.begin(), publicKey.m.end());

    if (!file.atEnd())
    {
        printer_->printWarn("Unexpected data at the end of the file: " + file.fileName());
        return nullopt;
    }
    Q_ASSERT(file.atEnd());
    return publicKey;
}
