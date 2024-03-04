#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QFileInfo>
#include <QTimer>
#include <QtDebug>

#ifdef Q_OS_LINUX
#include <sys/prctl.h>
#endif

#include "SqliteConsumer.h"
#include "SqliteProducer.h"

//#define TRACE_SQLITECONSUMER
#ifdef TRACE_SQLITECONSUMER
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif


SqliteConsumer::SqliteConsumer(const QString &filepath, QObject *parent)
    : QObject(parent)
    , db_filepath(filepath)
    , open_timer(nullptr)
    , open_retry(0)
    , db_mtime(0)
{
    TRACE();
}

SqliteConsumer::~SqliteConsumer()
{
    TRACE();

    if (!conn_name.isEmpty() && QSqlDatabase::contains(conn_name))
        QSqlDatabase::removeDatabase(conn_name);
}

void SqliteConsumer::start()
{
#ifdef PR_SET_NAME
    ::prctl(PR_SET_NAME, SqliteProducer::dataBaseFile, 0, 0, 0);
#endif
    conn_name = metaObject()->className();
    conn_name += QString::number(reinterpret_cast<quint64>(QThread::currentThreadId()));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn_name);
    db.setDatabaseName(db_filepath);
    db.setConnectOptions("QSQLITE_OPEN_READONLY");

    TRACE_ARG(db.connectionName() << db.databaseName());

    reopen();
}

void SqliteConsumer::reopen()
{
    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    QFileInfo finfo(db_filepath);
    if (finfo.exists() && finfo.isFile() && finfo.isReadable()) {
        QSqlDatabase db = QSqlDatabase::database(conn_name);
        if (db.isOpen() || db.open()) {
            QStringList tables = db.tables();
            if (tables.contains(SqliteProducer::dataBaseConfig)) {
                if (open_timer) open_timer->stop();
                db_mtime = finfo.lastModified().toMSecsSinceEpoch();
                return;
            }
            db.close();
            TRACE_ARG(db.lastError().text());
        }
    }
    if (!open_timer) {
        open_timer = new QTimer(this);
        open_timer->setInterval(500);
        open_timer->setSingleShot(true);
        connect(open_timer, &QTimer::timeout, this, &SqliteConsumer::reopen);
    } else if (++open_retry < 5) {
        open_timer->setInterval(open_timer->interval() * 2);
    } else {
        qWarning() << Q_FUNC_INFO << db_filepath << open_retry << "Not available";
        return;
    }
    open_timer->start();
}

void SqliteConsumer::execQuery(const QString &request)
{
    TRACE_ARG(request);

    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    if (!db_mtime) {
        emit queryResult(CborMapArray());
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(conn_name);
    if (!db.isOpen() && !db.open()) {
        TRACE_ARG(db.lastError().text());
        emit queryError(db.lastError().text());
        return;
    }
    QString query = !request.isEmpty() ? request : SqliteProducer::sqlConfigQuery;
    bool cached = query_cache.contains(query);
    if (cached) {
        qint64 mtime = QFileInfo(db.databaseName()).lastModified().toMSecsSinceEpoch();
        if (mtime > db_mtime) {
            db_mtime = mtime;
            query_cache.clear();
            cached = false;
        }
    }
    if (!cached) {
        QSqlQuery sql(db);
        sql.setForwardOnly(true);
        if (!sql.exec(query)) {
            if (db.lastError().isValid()) {
                TRACE_ARG(db.lastError().text());
                emit queryError(db.lastError().text());
            } else {
                emit queryResult(CborMapArray());
            }
            return;
        }
        CborMapArray rows;
        for (int row = 0; row < maxRowsPerQuery && sql.next(); row++) {
            QCborMap map;
            for (int col = 0; col < sql.record().count(); col++) {
                map.insert(sql.record().fieldName(col), QCborValue::fromVariant(sql.value(col)));
            }
            rows.append(map);
        }
        query_cache.insert(query, rows);
        TRACE_ARG("result rows" << rows.size());
    }
    emit queryResult(query_cache.value(query));
}
