#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QTimer>
#include <QUuid>
#include <QtDebug>

#ifdef Q_OS_LINUX
#include <sys/prctl.h>
#endif

#include "SqliteProducer.h"
#include "ActivityRecord.h"

//#define TRACE_SQLITEPRODUCER
#ifdef TRACE_SQLITEPRODUCER
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

static const char *sqlConfigCreate =
    "CREATE TABLE '%1' (Created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    " HistoryOn TINYINT, TimeStep TINYINT, KeepDays SMALLINT, ReduceAt TEXT, PRIMARY KEY (Created))";
static const char *sqlConfigViewCreate =
    "CREATE VIEW Config AS SELECT tab.Created, tab.HistoryOn, tab.TimeStep, tab.KeepDays, tab.ReduceAt"
    " FROM '%1' AS tab WHERE tab.Created =(SELECT MAX(Created) FROM '%1')";
static const char *sqlConfigInsert =
    "INSERT INTO '%1' (HistoryOn, TimeStep, KeepDays, ReduceAt) VALUES (%2, %3, %4, '%5')";

static const char *sqlTableCreate =
    "CREATE TABLE '%1' (LocalTime INTEGER PRIMARY KEY NOT NULL,"
    " ProjectId TEXT NOT NULL, TextNote TEXT, KeyPresses INTEGER, MouseClicks INTEGER, MouseDistance INTEGER, ServerStatus TEXT"
    ") WITHOUT ROWID";
static const char *sqlTableInsert =
    "INSERT INTO '%1' (LocalTime, ProjectId, TextNote, KeyPresses, MouseClicks, MouseDistance)"
    " VALUES (:LocalTime, :ProjectId, :TextNote, :KeyPresses, :MouseClicks, :MouseDistance)";
static const char *sqlTableStatusUpdate =
        "UPDATE '%1' SET ServerStatus='%2' WHERE LocalTime=%3";
static const char *sqlTableStatusBatch =
        "UPDATE '%1' SET ServerStatus=(CASE %2 END) WHERE LocalTime IN (%3)";
static const char *sqlTableStatusWhen =
        " WHEN LocalTime=%1 THEN '%2'";
static const char *sqlTableReduce =
    "DELETE FROM '%1' WHERE LocalTime < %2";


SqliteProducer::SqliteProducer(const QString &filepath, QObject *parent)
    : QObject(parent)
    , db_filepath(filepath)
    , keep_days(KeepDays)
    , reduce_timer(nullptr)
{
    TRACE();
}

SqliteProducer::~SqliteProducer()
{
    TRACE();
}

void SqliteProducer::start()
{
#ifdef PR_SET_NAME
    ::prctl(PR_SET_NAME, dataBaseFile, 0, 0, 0);
#endif
    QDate mdate = QFileInfo(db_filepath).lastModified().date();

    conn_name = metaObject()->className();
    conn_name += QString::number(reinterpret_cast<quint64>(QThread::currentThreadId()));

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn_name);
    db.setDatabaseName(db_filepath);
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=2000");

    TRACE_ARG(db.connectionName() << db.databaseName());

    if (!db.open()) {
        TRACE_ARG(QString("Can't open '%1'").arg(db_filepath));
        emit errorOccurred(QString("Can't open '%1'").arg(db_filepath));
        return;
    }
    db_config.clear();

    QSqlQuery sql(db);
    QStringList tables = db.tables();
    if (tables.isEmpty()) sql.exec("PRAGMA encoding = 'UTF-8'");
    if (!tables.contains(dataBaseConfig)) {
        sql.exec(QString(sqlConfigCreate).arg(dataBaseConfig));
        sql.exec(QString(sqlConfigViewCreate).arg(dataBaseConfig));
        sql.exec(QString(sqlConfigInsert).arg(dataBaseConfig)
                     .arg(HistoryOn).arg(TimeStep).arg(KeepDays).arg(dataBaseReduce));
        db_config.insert(QStringLiteral("HistoryOn"), HistoryOn);
        db_config.insert(QStringLiteral("TimeStep"), TimeStep);
        db_config.insert(QStringLiteral("KeepDays"), KeepDays);
        db_config.insert(QStringLiteral("ReduceAt"), dataBaseReduce);
    }
    if (db_config.isEmpty()) {
        sql.exec(sqlConfigQuery);
        if (sql.last()) {
            QSqlRecord record = sql.record();
            for (int i = 0; i < record.count(); i++) {
                db_config.insert(record.fieldName(i), record.value(i));
            }
        }
    }
    db.close();

    reconfig();

    if (mdate != QDate::currentDate())
        reduceTimer()->start(1500);

    emit configChanged(db_config);
}

void SqliteProducer::configure(const QVariantMap &map)
{
    TRACE_ARG(map);

    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(conn_name);
    if (!db.isValid() || db.lastError().isValid()) {
        TRACE_ARG("Not ready");
        return;
    }
    QVariantMap next = map;
    if (next.isEmpty()) {
        next.insert(QStringLiteral("HistoryOn"), HistoryOn);
        next.insert(QStringLiteral("TimeStep"), TimeStep);
        next.insert(QStringLiteral("KeepDays"), KeepDays);
        next.insert(QStringLiteral("ReduceAt"), dataBaseReduce);
    }
    for (auto it = db_config.constBegin(); it != db_config.constEnd(); ++it) {
        if (!next.contains(it.key())) next.insert(it.key(), it.value());
    }
    if (next == db_config) return;

    bool ok = false;
    if (db.isOpen() || db.open()) {
        QString insert = QString("INSERT INTO '%1'(").arg(dataBaseConfig);
        QString values = ") VALUES (";
        int i = 0;
        for (auto it = next.constBegin(); it != next.constEnd(); ++it) {
            if (it.key().toLower() == "created") continue;
            if (i++) {
                insert += ", ";
                values += ", ";
            }
            insert += it.key();
            bool is_str = (it.value().userType() == qMetaTypeId<QString>());
            if (is_str) values += '\'';
            values += it.value().toString();
            if (is_str) values += '\'';
        }
        values += ')';
        insert += values;

        QSqlQuery sql(db);
        ok = sql.exec(insert);
    }
    if (!ok || next.isEmpty()) {
        if (db.lastError().isValid()) {
            TRACE_ARG(db.lastError().text());
            emit errorOccurred(db.lastError().text());
        }
        return;
    }

    db_config = next;
    reconfig();
    emit configChanged(db_config);
}

void SqliteProducer::insertRow(const ActivityRecord &record)
{
    TRACE_ARG(record);

    if (!record.isValid()) {
        TRACE_ARG("ActivityRecord is invalid");
        return;
    }
    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(conn_name);
    if (!db.isValid() || db.lastError().isValid()) {
        TRACE_ARG("Not ready");
        return;
    }
    if (!db.isOpen() && !db.open()) {
        TRACE_ARG(db.lastError().text());
        emit errorOccurred(db.lastError().text());
        return;
    }
    QString table = record.tableName();
    if (!db.tables().contains(table)) {
        QSqlQuery sql(db);
        if (!sql.exec(QString(sqlTableCreate).arg(table))) {
            TRACE_ARG(db.lastError().text());
            emit errorOccurred(db.lastError().text());
            return;
        }
    }
    bool ta = db.transaction();
    QSqlQuery sql(db);
    sql.prepare(QString(sqlTableInsert).arg(table));
    sql.bindValue(":LocalTime",     record.localTime().toSecsSinceEpoch());
    sql.bindValue(":ProjectId",     record.projectId());
    sql.bindValue(":TextNote",      record.textNote());
    sql.bindValue(":KeyPresses",    record.keyPresses());
    sql.bindValue(":MouseClicks",   record.mouseClicks());
    sql.bindValue(":MouseDistance", record.mouseDistance());
    bool ok = sql.exec();
    if (ok) sql.finish();
    if (ta) {
        if (ok) db.commit();
        else db.rollback();
    }
    if (!ok) {
        TRACE_ARG(db.lastError().text());
        emit errorOccurred(db.lastError().text());
        return;
    }
    emit dataChanged();
}

void SqliteProducer::setServerStatus(const QString &table, const ServerStatusMap &status)
{
    TRACE_ARG(status);

    if (table.isEmpty() || status.isEmpty()) return; // just for sanity

    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(conn_name);
    if (!db.isValid() || db.lastError().isValid()) {
        TRACE_ARG("Not ready");
        return;
    }
    if (!db.isOpen() && !db.open()) {
        TRACE_ARG(db.lastError().text());
        emit errorOccurred(db.lastError().text());
        return;
    }
    if (!db.tables().contains(table)) return;

    bool ok = true;
    bool ta = db.transaction();
    if (status.size() == 1) { //"UPDATE '%1' SET ServerStatus='%2' WHERE LocalTime=%3"
            QSqlQuery sql(db);
            ok = sql.exec(QString(sqlTableStatusUpdate).arg(table, status.first()).arg(status.firstKey()));
    } else {
        int batch = 0;
        QString when, where;
        for (auto it = status.constBegin(); it != status.constEnd() && ok; ++it) {
            //" WHEN LocalTime=%1 THEN '%2'"
            if (!when.isEmpty()) when += '\n';
            when += QString(sqlTableStatusWhen).arg(it.key()).arg(it.value());
            if (!where.isEmpty()) where += ", ";
            where += QString::number(it.key());

            if (++batch > 10) { //"UPDATE '%1' SET ServerStatus=(CASE %2 END) WHERE LocalTime IN (%3)"
                if (when.isEmpty() || where.isEmpty()) break; // just for sanity
                QSqlQuery sql(db);
                ok = sql.exec(QString(sqlTableStatusBatch).arg(table, when, where));
                batch = 0;
                when.clear();
                where.clear();
            }
        }
        if (!when.isEmpty() && !where.isEmpty()) {
            QSqlQuery sql(db);
            ok = sql.exec(QString(sqlTableStatusBatch).arg(table, when, where));
        }
    }
    if (ta) {
        if (ok) db.commit();
        else db.rollback();
    }
    if (!ok) {
        TRACE_ARG(db.lastError().text());
        emit errorOccurred(db.lastError().text());
        return;
    }
    emit dataChanged();
}

QTimer *SqliteProducer::reduceTimer()
{
    if (!reduce_timer) {
        reduce_timer = new QTimer(this);
        reduce_timer->setSingleShot(true);
        reduce_timer->setTimerType(Qt::VeryCoarseTimer);
        connect(reduce_timer, &QTimer::timeout, this, &SqliteProducer::reduce);
    }
    return reduce_timer;
}

static int millisecondsTo(const QString &at)
{
    QTime tm = QTime::fromString(at, QStringLiteral("hh:mm:ss"));
    int ms = QTime::currentTime().msecsTo(tm.isValid() ? tm : QTime(23,59,55));
    if (ms < 0) ms += 24 * 60 * 60 * 1000;
    return ms;
}

void SqliteProducer::reconfig()
{
    QString at = db_config.value(QStringLiteral("ReduceAt")).toString();
    if (!at.isEmpty() && at != reduce_at) {
        reduceTimer()->start(millisecondsTo(at));
        reduce_at = at;
    }
    int kd = db_config.value(QStringLiteral("KeepDays")).toUInt();
    if (kd && kd != keep_days) {
        if (kd < keep_days) reduceTimer()->start(0);
        keep_days = kd;
    }
    TRACE_ARG(keep_days << reduce_at);
}

void SqliteProducer::reduce()
{
    TRACE();

    if (keep_days < 1) return;

    if (conn_name.isEmpty()) {
        TRACE_ARG("Not started");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database(conn_name);
    if (!db.isValid() || db.lastError().isValid()) {
        TRACE_ARG("Not ready");
        return;
    }
    if (!db.isOpen() && !db.open()) {
        TRACE_ARG(db.lastError().text());
        emit errorOccurred(db.lastError().text());
        return;
    }
    int count = 0;
    QStringList db_tables = db.tables();
    for (const auto &table : db_tables) {
        if (table.isEmpty()) continue;
        bool ta = db.transaction();
        QSqlQuery sql(db);
        QDateTime now = QDateTime::currentDateTime();
        auto sec = now.addSecs(-keep_days * 86400).secsTo(now); // to consider DST
        bool ok = sql.exec(QString(sqlTableReduce).arg(table).arg(sec));
        if (ok) sql.finish();
        if (ta) {
            if (ok) ok = db.commit();
            else db.rollback();
        }
        if (!ok) {
            TRACE_ARG(db.lastError().text());
            emit errorOccurred(db.lastError().text());
            return;
        }
        count++;
    }
    if (count) {
        QSqlQuery sql(db);
        sql.exec("VACUUM");
    }
    if (!reduce_at.isEmpty()) {
        int msec = millisecondsTo(reduce_at);
        TRACE_ARG("Next time" << QTime(0,0).addMSecs(msec).toString());
        reduceTimer()->start(msec);
    }
    if (count) emit dataChanged();
}

// static
QString SqliteProducer::columnName(int column)
{
    return (column >= 0 && column < TotalColumns) ? dataBaseColumn[column] : QString();
}

// static
int SqliteProducer::columnIndex(const QString &name)
{
    if (!name.isEmpty()) {
        for (int i = 0; i < TotalColumns; i++) {
            if (name == dataBaseColumn[i]) return i;
        }
    }
    return -1;
}

// static
QMap<QString,int> SqliteProducer::columnMap()
{
    QMap<QString,int> map;
    for (int i = 0; i < TotalColumns; i++) {
        map.insert(QLatin1String(dataBaseColumn[i]), i);
    }
    return map;
}
