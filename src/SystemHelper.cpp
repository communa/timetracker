#include <QGuiApplication>
#include <QStandardPaths>
#include <QOperatingSystemVersion>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QClipboard>
#include <QKeySequence>
#include <QHostInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QMetaType>
#include <QSysInfo>
#include <QThread>
#include <QCursor>
#ifdef  Q_OS_ANDROID
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtAndroidExtras>
#else
#include <QJniEnvironment>
#include <QJniObject>
#endif
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <lmcons.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include "SystemHelper.h"

extern bool sshKeygenEd25519(const QString &filename, const QString &comm);

static const char *localFilePrefix = "file://";

SystemHelper::SystemHelper(QObject *parent)
    : QObject(parent)
{
}

SystemHelper::~SystemHelper()
{
}

static QDir standardDir(QStandardPaths::StandardLocation type)
{
    QDir dir(QStandardPaths::writableLocation(type));
    return (dir.exists() || dir.mkpath(".")) ? dir : QDir::current();
}

static QString fixFilePath(const QString &name)
{
    QFileInfo info(name);
    if (info.isAbsolute()) return name;

    QDir dir(SystemHelper::appDataDir() + '/' + info.path());
    return (dir.exists() || dir.mkpath(".")) ? dir.filePath(info.fileName())
                                             : SystemHelper::appDataPath(info.fileName());
}

static QString fixFileName(const QString &name, const QString &suffix)
{
    QString path = name.contains(QLatin1String(":/")) ? name : QDir::cleanPath(name);

    path = fixFilePath(path.isEmpty() ? QCoreApplication::applicationName() : path);

    QFileInfo info(path);
    if (info.suffix().isEmpty()) {
        path.append('.');
        path.append(!suffix.isEmpty() ? suffix : QStringLiteral("txt"));
    }
    return path;
}

static QString fileContainsLine(const QString &path, const QRegularExpression &re)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty()) return QString();

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');

    QFile file(fn);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        for (int i = 0; i < 100 && !file.atEnd(); i++) {
            QByteArray line = file.readLine(1024).trimmed();
            if (!line.isEmpty()) {
                QString text = QString::fromLatin1(line);
                if (text.contains(re)) return text;
            }
        }
    }
    return QString();
}

// static
QString SystemHelper::appCachePath(const QString &name)
{
    return name.isEmpty() ? standardDir(QStandardPaths::CacheLocation).absolutePath()
                          : standardDir(QStandardPaths::CacheLocation).absoluteFilePath(name);
}

// static
QString SystemHelper::appConfigPath(const QString &name)
{
    return name.isEmpty() ? standardDir(QStandardPaths::AppConfigLocation).absolutePath()
                          : standardDir(QStandardPaths::AppConfigLocation).absoluteFilePath(name);
}

// static
QString SystemHelper::appDataPath(const QString &name)
{
    return name.isEmpty() ? standardDir(QStandardPaths::AppDataLocation).absolutePath()
                          : standardDir(QStandardPaths::AppDataLocation).absoluteFilePath(name);
}

// static
QString SystemHelper::appLogPath(const QString &name)
{
    return name.isEmpty() ? standardDir(QStandardPaths::DocumentsLocation).absolutePath()
                          : standardDir(QStandardPaths::DocumentsLocation).absoluteFilePath(name);
}

// static
QString SystemHelper::appCacheDir()
{
    return standardDir(QStandardPaths::CacheLocation).absolutePath();
}

// static
QString SystemHelper::appConfigDir()
{
    return standardDir(QStandardPaths::AppConfigLocation).absolutePath();
}

// static
QString SystemHelper::appDataDir()
{
    return standardDir(QStandardPaths::AppDataLocation).absolutePath();
}

// static
QString SystemHelper::appLogDir()
{
    return standardDir(QStandardPaths::DocumentsLocation).absolutePath();
}

// static
QString SystemHelper::appSshKey()
{
    static QString ssh_key;
    if (ssh_key.isEmpty()) {
        QString path = SystemHelper::appConfigPath(defaultSshKeyName);
        if (SystemHelper::isSshKeyPair(path) ||
            sshKeygenEd25519(path, QCoreApplication::applicationName() + '@' + SystemHelper::domainName())) {
            ssh_key = path;
        }
    }
    return ssh_key;
}

// static
QString SystemHelper::envVariable(const QString &name)
{
    if (!name.isEmpty()) {
        char var_name[100];
        return qEnvironmentVariable(qstrncpy(var_name, qPrintable(name), sizeof(var_name)),
                                    QString()).trimmed();
    }
    return QString();
}

// static
QString SystemHelper::platformOS()
{
    static QString os_name;
    if (os_name.isEmpty()) {
        QOperatingSystemVersion osv = QOperatingSystemVersion::current();
        os_name = osv.name();
        if (os_name.isEmpty()) {
#if defined(Q_OS_ANDROID)
            os_name = "Android";
#elif defined(Q_OS_DARWIN)
            os_name = "Darwin";
#elif defined(Q_OS_WIN)
            os_name = "Windows";
#elif defined(Q_OS_LINUX)
            os_name = "Linux";
#elif defined(Q_OS_FREEBSD)
            os_name = "FreeBSD";
#elif defined(Q_OS_OPENBSD)
            os_name = "OpenBSD";
#elif defined(Q_OS_UNIX)
            os_name = "Unix";
#else
            os_name = "Unknown";
#endif
        }
        if (osv.segmentCount()) {
            os_name += ' ';
            for (int i = 0; i < osv.segmentCount() && i < 3; i++) {
                if (i) os_name += '.';
                switch (i) {
                case 0: os_name += QString::number(osv.majorVersion()); break;
                case 1: os_name += QString::number(osv.minorVersion()); break;
                case 2: os_name += QString::number(osv.microVersion()); break;
                }
            }
        }
    }
    return os_name;
}

// static
QString SystemHelper::buildAbi()
{
    return QSysInfo::buildAbi();
}

// static
QString SystemHelper::buildCpuArch()
{
    return QSysInfo::buildCpuArchitecture();
}

// static
QString SystemHelper::cpuArch()
{
    return QSysInfo::currentCpuArchitecture();
}

// static
int SystemHelper::cpuCores()
{
    return QThread::idealThreadCount();
}

// static
QString SystemHelper::kernelType()
{
    return QSysInfo::kernelType();
}

// static
QString SystemHelper::kernelVersion()
{
    return QSysInfo::kernelVersion();
}

// static
QString SystemHelper::userName()
{
    static QString user_name;
    if (user_name.isEmpty()) {
#ifdef  Q_OS_UNIX
#if defined(Q_OS_ANDROID) && __ANDROID_API__ < 28
        char *cp;
#endif
        char buf[4096];
        struct passwd pw, *pwp = nullptr;
        if (::getpwuid_r(::getuid(), &pw, buf, sizeof(buf), &pwp) == 0 && pwp) user_name = pwp->pw_name;
#if defined(Q_OS_ANDROID) && __ANDROID_API__ < 28
        else if ((cp = ::getlogin()) != nullptr) user_name = cp;
#else
        else if (::getlogin_r(buf, sizeof(buf))) user_name = buf;
#endif
        if (user_name.isEmpty()) user_name = qEnvironmentVariable("USER");
#else // Windows?
        char buf[UNLEN + 1];
        DWORD len = UNLEN + 1;
        if (GetUserNameA(buf, &len)) user_name = buf;
        if (user_name.isEmpty()) user_name = qEnvironmentVariable("USERNAME");
#endif
        if (user_name.isEmpty()) user_name = QCoreApplication::applicationName();
    }
    return user_name;
}

#ifdef Q_OS_ANDROID

static QString getBuildProperty(const char *name)
{
    if (name && *name && ::geteuid()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (QAndroidJniEnvironment::javaVM()) {
            auto obj = QAndroidJniObject::getStaticObjectField<jstring>("android/os/Build", name);
            if (obj.isValid()) return obj.toString();
            QAndroidJniEnvironment env;
            if (env && env->ExceptionCheck()) env->ExceptionClear();
        }
#else
        if (QJniEnvironment::javaVM()) {
            auto obj = QJniObject::getStaticObjectField<jstring>("android/os/Build", name);
            if (obj.isValid()) return obj.toString();
            QJniEnvironment env;
            if (env.isValid()) env.checkAndClearExceptions(QJniEnvironment::OutputMode::Silent);
        }
#endif
    }
    return QString();
}

// static
QString SystemHelper::hostName()
{
    static QString host_name;
    if (host_name.isEmpty()) {
        QString manuf = getBuildProperty("MANUFACTURER").toLower();
        if (!manuf.isEmpty()) {
            QString model = getBuildProperty("MODEL").toLower();
            host_name = model.startsWith(manuf) ? camelCase(model) : camelCase(manuf + ' ' + model);
        }
        if (host_name.isEmpty()) {
            host_name = camelCase(getBuildProperty("PRODUCT"));
            if (host_name.isEmpty()) host_name = platformOS();
        }
        host_name.replace(QLatin1Char(' '), QLatin1Char('-'));
    }
    return host_name;
}

#else

// static
QString SystemHelper::hostName()
{
    static QString host_name;
    if (host_name.isEmpty()) {
        host_name = QSysInfo::machineHostName();
        if (host_name.isEmpty())
            host_name = platformOS();
        host_name.replace(QLatin1Char(' '), QLatin1Char('-'));
    }
    return host_name;
}

#endif // !Q_OS_ANDROID

// static
QString SystemHelper::domainName()
{
    static QString domain_name;
    if (domain_name.isEmpty()) {
        domain_name = QHostInfo::localDomainName();
        if (domain_name.isEmpty())
            domain_name = SystemHelper::hostName();
    }
    return domain_name;
}

// static
QStringList SystemHelper::sshKeyPairs()
{
    static const QStringList name_filters = { "id_*" };

    QStringList key_files;
    QString env_var = SystemHelper::envVariable(QStringLiteral("CLOUD_KEY"));
    if (!env_var.isEmpty()) key_files.append(env_var);
#if defined(__mobile__)
    const auto search_paths = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
#else
    const auto search_paths = QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
#endif
    for (const auto &folder : search_paths) {
        for (uint i = 0; i < sizeof(sshKeySearchPaths) / sizeof(sshKeySearchPaths[0]); i++) {
            QDir dir(folder + '/' + sshKeySearchPaths[i]);
            if (!dir.exists() || !dir.isReadable()) continue;

            const auto names = dir.entryList(name_filters, QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
            for (const auto &name : names) {
                QString path = dir.filePath(name);
                if (SystemHelper::isSshKeyPair(path)) key_files.append(path);
            }
        }
    }
    return key_files;
}

// static
bool SystemHelper::isSshKeyPair(const QString &path)
{
    return (SystemHelper::isSshPrivateKey(path) && !SystemHelper::sshPublicKey(path + ".pub").isEmpty());
}

// static
bool SystemHelper::isSshPrivateKey(const QString &path)
{
    static const QRegularExpression re("^-----BEGIN \\w+ PRIVATE KEY-----$");
    return !fileContainsLine(path, re).isEmpty();
}

// static
QString SystemHelper::sshPublicKey(const QString &path)
{
    static const QRegularExpression re("^ssh-\\w+\\s\\w+");
    return fileContainsLine(path, re);
}

// static
QString SystemHelper::pathName(const QString &path)
{
    return path.isEmpty() ? QDir::currentPath() : QFileInfo(path).path();
}

// static
QString SystemHelper::fileName(const QString &path)
{
    return path.isEmpty() ? QTemporaryFile().fileTemplate() : QFileInfo(path).fileName();
}

// static
QString SystemHelper::baseName(const QString &path)
{
    return path.isEmpty() ? QString() : QFileInfo(path).baseName();
}

// static
bool SystemHelper::isDir(const QString &path, bool writable)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty() || fn.startsWith(QLatin1String(":/")) || fn.startsWith(QLatin1String("qrc:/")))
        return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    QFileInfo fi(fn);
    if (!fi.exists() || !fi.isDir() || !fi.isReadable()) return false;
    return (writable ? fi.isWritable() : true);
}

// static
bool SystemHelper::isFile(const QString &path, bool writable)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty()) return false;

    if (fn.startsWith(QLatin1String(":/")) || fn.startsWith(QLatin1String("qrc:/")))
        return writable ? false : QFile::exists(path);

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    QFileInfo fi(fn);
    if (!fi.exists() || !fi.isFile() || !fi.isReadable()) return false;
    return (writable ? fi.isWritable() : true);
}

// static
bool SystemHelper::isExecutable(const QString &path)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty()) return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    QFileInfo fi(fn);
    return (fi.exists() && fi.isExecutable());
}

// static
bool SystemHelper::makeDir(const QString &path)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty() || fn == QDir::currentPath()) return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    if (!QDir(fn).mkpath(".")) return false;
#ifdef Q_OS_ANDROID
    QFile::setPermissions(fn, QFile::Permission(0x700));
#else
    QFile::setPermissions(fn, QFile::Permission(0x755));
#endif
    return true;
}

// static
bool SystemHelper::removeDir(const QString &path, bool recursively)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty() || fn == QDir::currentPath()) return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    return recursively ? QDir(fn).removeRecursively() : QDir().rmdir(fn);
}

// static
bool SystemHelper::removeFile(const QString &name)
{
    if (name.startsWith(QLatin1String(":/")) || name.startsWith(QLatin1String("qrc:/")))
        return false;

    QString path = QDir::cleanPath(name);
    if (path.isEmpty()) return false;

    if (QFileInfo(path).isRelative()) path.prepend(SystemHelper::appDataDir() + '/');

    QFile file(path);
    return (file.exists() ? file.remove() : true);
}

// static
bool SystemHelper::renameFile(const QString &from, const QString &to)
{
    QString fn = from.startsWith(localFilePrefix) ? from.mid(qstrlen(localFilePrefix)) : from;
    if (fn.isEmpty() || fn.startsWith(QLatin1String(":/")) || fn.startsWith(QLatin1String("qrc:/")))
        return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    QFile src_file(fn);
    if (src_file.exists()) {
        fn = to.startsWith(localFilePrefix) ? to.mid(qstrlen(localFilePrefix)) : to;
        if (fn.isEmpty() || fn.startsWith(QLatin1String(":/")) || fn.startsWith(QLatin1String("qrc:/")))
            return false;

        if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
        QFile dst_file(fn);
        if (!dst_file.exists() || dst_file.remove())
            return src_file.rename(dst_file.fileName());
    }
    return false;
}

// static
bool SystemHelper::copyFile(const QString &from, const QString &to, bool replace)
{
    QString fn = from.startsWith(localFilePrefix) ? from.mid(qstrlen(localFilePrefix)) : from;
    if (fn.isEmpty()) return false;

    if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
    QFile src_file(fn);
    if (src_file.exists()) {
        fn = to.startsWith(localFilePrefix) ? to.mid(qstrlen(localFilePrefix)) : to;
        if (fn.isEmpty() || fn.startsWith(QLatin1String(":/")) || fn.startsWith(QLatin1String("qrc:/")))
            return false;

        if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
        QFile dst_file(fn);
        bool dst_exist = dst_file.exists();
        if (dst_exist && !replace) return true;
        if ((!dst_exist || dst_file.remove()) && src_file.copy(dst_file.fileName())) {
            QFile::setPermissions(dst_file.fileName(), QFile::Permission(0x644));
            return true;
        }
    }
    return false;
}

// static
QString SystemHelper::findFile(const QString &name, const QStringList &paths)
{
    if (!name.isEmpty()) {
        QStringList list = paths;
        bool writable = list.isEmpty();
        if (writable) {
            list += QDir::currentPath();
            list += QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation);
            list += QStandardPaths::standardLocations(QStandardPaths::HomeLocation);
            list += QStandardPaths::standardLocations(QStandardPaths::TempLocation);
        }
        list.removeDuplicates();
        for (const auto &path : list) {
            if (isDir(path, writable)) {
                QString fn = path;
                fn += '/';
                fn += name;
                if (isFile(fn, writable)) return fn;
            }
        }
    }
    return QString();
}

// static
QString SystemHelper::findExecutable(const QString &name, const QStringList &paths)
{
    if (name.isEmpty()) return QString();
    if (!paths.isEmpty()) {
        QString found = QStandardPaths::findExecutable(name, paths);
        if (!found.isEmpty()) return found;
    }
    return QStandardPaths::findExecutable(name);
}

// static
qreal SystemHelper::fileSize(const QString &path)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (!fn.isEmpty()) {
        if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
        QFileInfo fi(fn);
        if (fi.isFile() && fi.isReadable()) return fi.size();
    }
    return 0;
}

// static
QDateTime SystemHelper::fileTime(const QString &path)
{
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (!fn.isEmpty()) {
        if (QFileInfo(fn).isRelative()) fn.prepend(SystemHelper::appDataDir() + '/');
        QFileInfo fi(fn);
        if (fi.isFile() && fi.isReadable()) return fi.lastModified();
    }
    return QDateTime();
}

// static
QStringList SystemHelper::fileList(const QString &path, const QString &mask, bool dirs_only)
{
    bool abs_path = false;
    const QString data_dir = SystemHelper::appDataDir();
    QString fn = path.startsWith(localFilePrefix) ? path.mid(qstrlen(localFilePrefix)) : path;
    if (fn.isEmpty()) fn = data_dir;
    else if (QFileInfo(fn).isRelative()) fn.prepend(data_dir + '/');
    else abs_path = true;

    bool writable = false;
    QDir::Filters filter = (dirs_only ? QDir::Dirs : QDir::AllEntries) | QDir::Readable;
    if (fn.startsWith(data_dir)) {
        writable = true;
        filter |= (QDir::NoSymLinks | QDir::NoDotAndDotDot | QDir::Writable);
    } else filter |= (QDir::Hidden | QDir::System);

    QFileInfo info(fn);
    if (!info.isDir() || !info.isReadable() || (writable && !info.isWritable()))
        return QStringList();

    QDir::SortFlags sorts = (dirs_only ? QDir::Time : QDir::Name) | QDir::DirsFirst;
    QDir dir(fn);
    const auto list = mask.isEmpty() ? dir.entryInfoList(filter, sorts | QDir::IgnoreCase)
                                     : dir.entryInfoList({ mask }, filter, sorts);
    QStringList result;
    for (const auto &file : list) {
        if (file.isDir()) {
            QString name = abs_path ? file.filePath() : file.fileName();
            if (!dirs_only) name += '/';
            result.append(name);
        } else if (file.isFile()) {
            result.append(abs_path ? file.filePath() : file.fileName());
        }
    }
    return result;
}

// static
QVariant SystemHelper::loadSettings(const QString &name, const QVariant &defValue)
{
    if (name.isEmpty()) return QVariant();
    QSettings settings;
    if (!settings.contains(name)) return defValue;
    QVariant var = settings.value(name);
    if (!var.isValid() || var.isNull()) return defValue;
    if (!defValue.isValid()) return var;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto type = defValue.userType();
    if (var.userType() == type) return var;
#else
    auto type = defValue.metaType();
    if (var.metaType() == type) return var;
#endif
    return var.convert(type) ? var : defValue;
}

// static
void SystemHelper::saveSettings(const QString &name, const QVariant &value)
{
    if (name.isEmpty()) return;
    if (value.isValid()) {
        QSettings().setValue(name, value);
    } else {
        QSettings().remove(name);
    }
}

// static
QJsonArray SystemHelper::loadArray(const QString &name)
{
    if (!name.isEmpty()) {
        QFile file(fixFileName(name, QStringLiteral("json")));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument jdoc = QJsonDocument::fromJson(file.readAll());
            if (jdoc.isArray()) return jdoc.array();
        }
    }
    return QJsonArray();
}

// static
QString SystemHelper::saveArray(const QString &name, const QJsonArray &json)
{
    if (!name.isEmpty()) {
        QString fn = fixFileName(name, QStringLiteral("json"));
        if (!json.isEmpty()) {
            const auto data = QJsonDocument(json).toJson();
            if (data.size()) {
                QSaveFile save_file(fn);
                if (save_file.open(QIODevice::WriteOnly | QIODevice::Text))
                    save_file.write(data);

                if (save_file.isOpen() && save_file.commit()) {
                    QFile::setPermissions(save_file.fileName(), QFile::Permission(0x644));
                    return save_file.fileName();
                }
            }
        } else if (!QFile::exists(fn) || QFile::remove(fn)) return fn;
    }
    return QString();
}

// static
QJsonObject SystemHelper::loadObject(const QString &name)
{
    if (!name.isEmpty()) {
        QFile file(fixFileName(name, QStringLiteral("json")));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument jdoc = QJsonDocument::fromJson(file.readAll());
            if (jdoc.isObject()) return jdoc.object();
        }
    }
    return QJsonObject();
}

// static
QString SystemHelper::saveObject(const QString &name, const QJsonObject &json)
{
    if (!name.isEmpty()) {
        QString fn = fixFileName(name, QStringLiteral("json"));
        if (!json.isEmpty()) {
            const auto data = QJsonDocument(json).toJson();
            if (data.size()) {
                QSaveFile save_file(fn);
                if (save_file.open(QIODevice::WriteOnly | QIODevice::Text))
                    save_file.write(data);

                if (save_file.isOpen() && save_file.commit()) {
                    QFile::setPermissions(save_file.fileName(), QFile::Permission(0x644));
                    return save_file.fileName();
                }
            }
        } else if (!QFile::exists(fn) || QFile::remove(fn)) return fn;
    }
    return QString();
}

// static
QStringList SystemHelper::loadText(const QString &name)
{
    QStringList text;
    if (!name.isEmpty()) {
        QFile file(fixFileName(name, QStringLiteral("txt")));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (file.size() > 0) {
                QTextStream in(&file);
                while (!in.atEnd()) {
                    text += in.readLine(1024);
                }
            } else { // may be linux /proc filesystem
                auto list = file.readAll().split('\n');
                for (int i = 0; i < list.size(); i++) {
                    auto line = list.at(i).simplified();
                    if (!line.isEmpty()) text += line;
                }
            }
        }
    }
    return text;
}

// static
QString SystemHelper::saveText(const QString &name, const QStringList &text, bool append)
{
    QIODevice::OpenMode mode = (QIODevice::WriteOnly | QIODevice::Text);
    if (append) mode |= (QIODevice::Append | QIODevice::ExistingOnly);

    QFile file(fixFileName(name, QStringLiteral("txt")));
    if (!file.open(mode)) return QString();

    QTextStream out(&file);
    for (int i = 0; i < text.size(); i++) {
        if (i || !text.at(i).isEmpty()) out << text.at(i) << '\n';
    }
    file.close();

    QFile::setPermissions(file.fileName(), QFile::Permission(0x644));
    return file.fileName();
}

// static
void SystemHelper::setClipboard(const QString &text)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        if (text.isNull()) clipboard->clear();
        else clipboard->setText(text);
    }
}

// static
QString SystemHelper::clipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    return clipboard ? clipboard->text() : QString();

}

//static
QString SystemHelper::camelCase(const QString &str, QChar sep)
{
    QStringList parts = str.simplified().toLower().split(sep, Qt::SkipEmptyParts);
    for (int i = 0; i < parts.size(); i++) {
        parts[i].replace(0, 1, parts[i][0].toUpper());
    }
    return parts.join(sep);
}

//static
QString SystemHelper::shortcutText(const QVariant &key)
{
#if defined(__mobile__)
    Q_UNUSED(key);
#else
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto type = key.type();
#else
    auto type = key.typeId();
#endif
    switch (type) {
    case QMetaType::Int: {
        QKeySequence::StandardKey code = (QKeySequence::StandardKey)key.toInt();
        if (code) return QKeySequence(code).toString(QKeySequence::PortableText); // NativeText/PortableText?
    }   break;
    case QMetaType::QString:
        return key.toString();
    default:
        break;
    }
#endif
    return QString();
}

void SystemHelper::setCursorShape(int shape)
{
    if (shape < 0) QGuiApplication::restoreOverrideCursor();
    else QGuiApplication::setOverrideCursor(QCursor((Qt::CursorShape)shape));
}
