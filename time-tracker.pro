APP_NAME = time-tracker
APP_VERSION = $$cat(APP_VERSION)
APP_DISPLAY = "Communa TimeTracker"
APP_PACKAGE = "Communa Network"
APP_HOMEURL = "https://communa.network/"

APP_DISTRIB = "Communa_TimeTracker_$${APP_VERSION}"
macx {
    TARGET = TimeTracker
    APP_DISTRIB = "$${APP_DISTRIB}-universal"
} else {
    TARGET = $${APP_NAME}
    APP_DISTRIB = "$${APP_DISTRIB}-x86_64"
}

TEMPLATE = app
QT += qml quick quickcontrols2 network sql svg
greaterThan(QT_MAJOR_VERSION,5): QT += core5compat

CONFIG -= debug
CONFIG -= debug_and_release
CONFIG -= qtquickcompiler
CONFIG += release deploy
CONFIG += lrelease embed_translations
CONFIG += file_copies
#macx:CONFIG += app_bundle  # by default; to remove CONFIG -= app_bundle

DEFINES += APP_NAME='\\"$${APP_NAME}\\"'
DEFINES += APP_VERSION='\\"$${APP_VERSION}\\"'

linux {
    UIOHOOK_OS_DIR = uiohook_x11
    DEFINES += USE_XKB_COMMON USE_XKB_FILE USE_EVDEV
    LIBS += -lX11 -lXtst -lxkbcommon-x11 -lxkbcommon -lX11-xcb -lxcb -lxkbfile
} else : win32 {
    contains(QMAKE_TARGET.arch, x86_64) {
        UIOHOOK_OS_DIR = uiohook_windows
        # Run compilation using VS compiler using multiple threads
        QMAKE_CXXFLAGS += -MP
        QMAKE_CXXFLAGS_WARN_ON += /WX /W3
        RC_ICONS = "deploy/windows/logo.ico"
        msvc:LIBS += Advapi32.lib User32.lib
        gcc:LIBS += -lAdvapi32 -lUser32
    } else {
        error("Unsupported toolchain. Currently only Visual Studio 2019+ 64 bit are supported")
    }
} else : macx {
    UIOHOOK_OS_DIR = uiohook_darwin
    QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15    
    QMAKE_TARGET_BUNDLE_PREFIX += network.communa
    ICON = "deploy/macos/logo.icns"
    DEFINES += USE_APPKIT USE_APPLICATION_SERVICES USE_IOKIT USE_OBJC
    LIBS += -framework Carbon -framework ApplicationServices -framework IOKit -framework AppKit -lobjc
} else {
    error("Unsupported build platform. Currently only Linux, Windows and MacOS are supported")
}

SOURCES += \
    src/$${UIOHOOK_OS_DIR}/input_helper.c \
    src/$${UIOHOOK_OS_DIR}/input_hook.c \
    src/$${UIOHOOK_OS_DIR}/system_properties.c \
    src/uiohook_logger.c \
    src/main.cpp \
    src/ActivityCounter.cpp \
    src/ActivityRecord.cpp \
    src/ActivityTableModel.cpp \
    src/HttpRequest.cpp \
    src/HttpRequestArgs.cpp \
    src/SqliteConsumer.cpp \
    src/SqliteExecQuery.cpp \
    src/SqliteProducer.cpp \
    src/SshKeygenEd25519.cpp \
    src/SystemHelper.cpp \
    src/UrlModel.cpp

HEADERS += \
    src/$${UIOHOOK_OS_DIR}/input_helper.h \
    src/uiohook.h \
    src/uiohook_logger.h \
    src/ActivityCounter.h \
    src/ActivityRecord.h \
    src/ActivityTableModel.h \
    src/BaseThread.h \
    src/HttpRequest.h \
    src/HttpRequestArgs.h \
    src/PermanentCache.h \
    src/SqliteConsumer.h \
    src/SqliteExecQuery.h \
    src/SqliteProducer.h \
    src/SystemHelper.h \
    src/UrlModel.h

linux {
    SOURCES += src/SystemSignal.cpp
    HEADERS += src/SystemSignal.h
}

RESOURCES += \
    fonts.qrc \
    icons.qrc \
    images.qrc \
    qml.qrc

TRANSLATIONS += $$files(translations/$${APP_NAME}_*.ts)

CONFIG(deploy) {
    linux {
        STAGING_DIR = AppDir
        DESKTOP_FILE = $${STAGING_DIR}/usr/share/applications/$${TARGET}.desktop
        COPY_COMMAND = $${QMAKE_COPY} -npL
        QMAKE_POST_LINK = $${QMAKE_MKDIR} $${STAGING_DIR}/usr/bin $${STAGING_DIR}/usr/lib && \
            $${COPY_COMMAND} $${TARGET} $${STAGING_DIR}/usr/bin

        DESKTOP_ENTRY += "[Desktop Entry]"
        DESKTOP_ENTRY += "Name=$${APP_DISPLAY} $${APP_VERSION}"
        DESKTOP_ENTRY += "Comment=$${APP_PACKAGE} - $${APP_HOMEURL}"
        DESKTOP_ENTRY += "Categories=Development;Office;"
        DESKTOP_ENTRY += "Exec=$${TARGET}"
        DESKTOP_ENTRY += "Icon=$${TARGET}"
        DESKTOP_ENTRY += "Type=Application"
        DESKTOP_ENTRY += "Terminal=false"
        DESKTOP_ENTRY += "StartupNotify=true"
        !write_file($${OUT_PWD}/$${DESKTOP_FILE}, DESKTOP_ENTRY): error("Can't write file")

        ICONS_HICOLOR = $${STAGING_DIR}/usr/share/icons/hicolor
        ICONS_SOURCES = $$files(deploy/linux/icons/*)
        for(icon_file, ICONS_SOURCES) {
            icon_name = $$basename(icon_file)
            icon_parts = $$split(icon_name, .)
            icon_apps_dir = "$${ICONS_HICOLOR}/$$first(icon_parts)/apps"
            QMAKE_POST_LINK += && $${QMAKE_MKDIR} $${icon_apps_dir} && \
                $${COPY_COMMAND} \"$${PWD}/$${icon_file}\" $${icon_apps_dir}/$${TARGET}.$$last(icon_parts)
        }

        DEPLOYQT_TOOL = $$[QT_HOST_BINS]/linuxdeployqt
        !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(which linuxdeployqt)
        !isEmpty(DEPLOYQT_TOOL) {
            QMAKE_POST_LINK += && $${DEPLOYQT_TOOL} $${DESKTOP_FILE} -qmake=\"$${QMAKE_QMAKE}\" -qmldir=\"$${PWD}/qml\" \
                -appimage -always-overwrite -extra-plugins=platformthemes -no-copy-copyright-files -no-translations
            exists(deploy/linux/exclude.libs) {
                exclude_libs = $$cat(deploy/linux/exclude.libs)
                exclude_split = $$split(exclude_libs)
                QMAKE_POST_LINK += -exclude-libs=$$join(exclude_split, ",")
            }
        }
    } else : win32 {
        STAGING_DIR = AppDir
        COPIES += windowsDll
        windowsDll.files = $$files(deploy/windows/*.dll)
        windowsDll.path = "$${OUT_PWD}/$${STAGING_DIR}"

        INNO_SETUP_EXEC = "C:\\Program Files \(x86\)\\Inno Setup 6\\iscc.exe"
        INNO_SETUP_FILE = "inno_setup.iss"
        !exists("$${INNO_SETUP_EXEC}"): error("No executable \"$${INNO_SETUP_EXEC}\" found in the System")
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppName \"$${APP_DISPLAY}\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppVersion \"$${APP_VERSION}\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppPublisher \"$${APP_PACKAGE}\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppURL \"$${APP_HOMEURL}\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppExeName \"$${TARGET}.exe\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppIconFile \"$$shell_path($${PWD}/$${RC_ICONS})\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppDeployFiles \"$$shell_path($${STAGING_DIR}/*)\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyAppOutputName \"$${APP_DISTRIB}\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyWizardImageLarge \"$$shell_path($${PWD}/deploy/windows/inno_setup_large.bmp)\""
        INNO_SETUP_DATA += "$${LITERAL_HASH}define MyWizardImageSmall \"$$shell_path($${PWD}/deploy/windows/inno_setup_small.bmp)\""
        INNO_SETUP_DATA += $$cat($${PWD}/deploy/windows/$${INNO_SETUP_FILE}, blob)
        !write_file($${OUT_PWD}/$${INNO_SETUP_FILE}, INNO_SETUP_DATA): error("Can't write file")

        DEPLOYQT_TOOL = $$[QT_HOST_BINS]/windeployqt
        !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(where windeployqt)
        !isEmpty(DEPLOYQT_TOOL) {
            QMAKE_POST_LINK = $${DEPLOYQT_TOOL} --release --force --no-compiler-runtime --no-system-d3d-compiler --no-translations \
                --qtpaths \"$$[QT_HOST_BINS]/qtpaths.exe\" --qmldir \"$${PWD}/qml\" --dir $${STAGING_DIR} $${TARGET}.exe
            QMAKE_POST_LINK += $$escape_expand(\\n) $$quote("\"$${INNO_SETUP_EXEC}\"" $${INNO_SETUP_FILE})
        }
    } else : macx {
        CODESIGN_CERT = "Developer ID Application: Ivan Proskuryakov (Q2Y2F39KYS)"

        DEPLOYQT_TOOL = $$[QT_HOST_BINS]/macdeployqt
        !exists("$${DEPLOYQT_TOOL}"): DEPLOYQT_TOOL = $$system(which macdeployqt)
        !isEmpty(DEPLOYQT_TOOL) {
            QMAKE_POST_LINK = $${DEPLOYQT_TOOL} $${TARGET}.app -qmldir=\"$${PWD}/qml\" -always-overwrite
            !isEmpty(CODESIGN_CERT): QMAKE_POST_LINK += -sign-for-notarization=\"$${CODESIGN_CERT}\"
            exists(deploy/macos/exclude.libs) {
                exclude_libs = $$cat(deploy/macos/exclude.libs)
                for(exclude_lib, exclude_libs) {
                    REMOVE_LIBS += $${TARGET}.app/$${exclude_lib}
                }
                !isEmpty(REMOVE_LIBS): QMAKE_POST_LINK += && rm -rf $${REMOVE_LIBS}
            }
            QMAKE_POST_LINK += && hdiutil create -volname \"$${APP_DISPLAY} $${APP_VERSION}\" -srcfolder $${TARGET}.app -ov -format ULMO $${APP_DISTRIB}.dmg
            !isEmpty(CODESIGN_CERT): QMAKE_POST_LINK += && codesign --options runtime --timestamp -s \"$${CODESIGN_CERT}\" $${APP_DISTRIB}.dmg
        }
    }
    isEmpty(DEPLOYQT_TOOL) {
        warning("The [OS]deployqt not found, so the deployment package will not be generated")
    }
}
