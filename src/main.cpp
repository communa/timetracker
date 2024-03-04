#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlFileSelector>
#include <QQmlContext>
#include <QIcon>
#include <QQuickStyle>
#include <QSettings>
#include <QVersionNumber>
#include <QLibraryInfo>
#include <QLocale>
#include <QLockFile>
#include <QDirIterator>
#include <QFontDatabase>
#include <QTranslator>

#include "ActivityCounter.h"
#include "ActivityTableModel.h"
#include "HttpRequest.h"
#include "SystemHelper.h"
#include "UrlModel.h"
#include "SqliteProducer.h"
#include "SqliteExecQuery.h"

#ifndef QML_CUSTOM_MODULES
#define QML_CUSTOM_MODULES "QmlCustomModules"
#endif
#ifndef CPP_CUSTOM_MODULES
#define CPP_CUSTOM_MODULES "CppCustomModules"
#endif

#if defined(__mobile__)
const bool isMobile = true;
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Normal";
#else
const bool isMobile = false;
static const char *quickControlStyle = "Material";
static const char *materialStyleMode = "Dense";
#endif
static const char *embeddedFontFamily = "Roboto";

int main(int argc, char *argv[])
{
    QLockFile lock_file(QDir::tempPath() + '/' + APP_NAME ".lock");
    if (!lock_file.tryLock(100)) {
        qWarning() << APP_NAME << "-- Already running!";
        return -1;
    }
    //qputenv("QT_QUICK_CONTROLS_1_STYLE", "Plasma"); // just for FileDialog Controls-1

    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE"))
        qputenv("QT_QUICK_CONTROLS_STYLE", quickControlStyle);

    if (!qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_MATERIAL_VARIANT"))
        qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", materialStyleMode);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if defined(__mobile__)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#else
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
#endif
    QGuiApplication app(argc, argv);

    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setApplicationDisplayName(QObject::tr("Communa v%1").arg(APP_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/image-logo64")));
    QQuickStyle::setStyle(quickControlStyle);

#ifdef Q_OS_ANDROID
    //setAndroidPermission();

    app.setOrganizationDomain(QStringLiteral("settings")); // just a fake for QSettings
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope,
                       QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
#else
    app.setOrganizationDomain(app.applicationName());
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif

    bool set_font = false;
    QDirIterator font_it(QStringLiteral(":/"), QStringList() << "font-*");
    while (font_it.hasNext()) {
        QString path = font_it.next();
        int font_id = QFontDatabase::addApplicationFont(path);
        if (!set_font) {
            auto font_families = QFontDatabase::applicationFontFamilies(font_id);
            set_font = font_families.contains(QLatin1String(embeddedFontFamily));
        }
    }
    if (set_font) {
        QFont sys_font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
        QFont app_font(QLatin1String(embeddedFontFamily), sys_font.pointSize(), sys_font.weight());
        app_font.setStyleStrategy(QFont::PreferAntialias);
        app.setFont(app_font);
    }

    QTranslator translator;
    const QStringList languages = QLocale::system().uiLanguages();
    for (const QString &lang : languages) {
        if (translator.load(QString(":/i18n/%1-%2").arg(APP_NAME, QLocale(lang).name()))) {
            app.installTranslator(&translator);
            break;
        }
    }

    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/MaterialSet.qml")), QML_CUSTOM_MODULES, 1, 0, "MaterialSet");
    qmlRegisterSingletonType(QUrl(QStringLiteral("qrc:/RestApiSet.qml")), QML_CUSTOM_MODULES, 1, 0, "RestApiSet");

    qmlRegisterSingletonType<ActivityCounter>(CPP_CUSTOM_MODULES, 1, 0, "ActivityCounter",
                                                [](QQmlEngine*, QJSEngine*)->QObject* {
        auto singleton = ActivityCounter::instance();
        QQmlEngine::setObjectOwnership(singleton, QQmlEngine::CppOwnership);
        return singleton;
    });
    qmlRegisterSingletonType<SystemHelper>(CPP_CUSTOM_MODULES, 1, 0, "SystemHelper",
                                           [](QQmlEngine*, QJSEngine*)->QObject* {
        return new SystemHelper();
    });
    qmlRegisterSingletonType<HttpRequest>(CPP_CUSTOM_MODULES, 1, 0, "HttpRequest",
                                          [](QQmlEngine*, QJSEngine*)->QObject* {
        return new HttpRequest();
    });
    qmlRegisterSingletonType<UrlModel>(CPP_CUSTOM_MODULES, 1, 0, "Url",
                                       [](QQmlEngine*, QJSEngine*)->QObject* {
        return new UrlModel();
    });
    qmlRegisterType<UrlModel>(CPP_CUSTOM_MODULES, 1, 0, "UrlModel");
    qmlRegisterType<ActivityTableModel>(CPP_CUSTOM_MODULES, 1, 0, "ActivityTableModel");
    qmlRegisterType<SqliteExecQuery>(CPP_CUSTOM_MODULES, 1, 0, "SqliteExecQuery");
    qmlRegisterUncreatableType<SqliteProducer>(CPP_CUSTOM_MODULES, 1, 0, "SqliteProducer", "Reference only");

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();

    const QString appSettingsPath(QSettings().fileName());
    context->setContextProperty(QStringLiteral("appSettingsPath"), appSettingsPath);
    context->setContextProperty(QStringLiteral("isMobile"), isMobile);

    const QVersionNumber qt_ver = QLibraryInfo::version(); // Qt run-time version
    if (qt_ver.majorVersion() > 5) {
        QStringList efs;
        efs.append(QStringLiteral("qt") + QString::number(qt_ver.majorVersion()));
        efs.append(QStringLiteral("mobile"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQmlFileSelector::get(&engine)->setExtraSelectors(efs);
#else
        engine.setExtraFileSelectors(efs);
#endif
    }
    QString qtRunningVersion = qVersion();
    if (qtRunningVersion != QT_VERSION_STR) qtRunningVersion += QString(" (build on %1)").arg(QT_VERSION_STR);
    context->setContextProperty(QStringLiteral("qtVersion"), qtRunningVersion);

    const QUrl qml(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app, [qml](QObject *obj, const QUrl &url) {
            if (!obj && qml == url) QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);
    engine.load(qml);

    return app.exec();
}
