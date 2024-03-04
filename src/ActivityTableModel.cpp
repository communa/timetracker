#include "ActivityTableModel.h"
#include "ActivityCounter.h"
#include "SqliteProducer.h"
#include "SystemHelper.h"

//#define TRACE_ACTIVITYTABLEMODEL
#ifdef TRACE_ACTIVITYTABLEMODEL
#include <QTime>
#define TRACE()      qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO;
#define TRACE_ARG(x) qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << QThread::currentThreadId() << Q_FUNC_INFO << x;
#else
#define TRACE()
#define TRACE_ARG(x)
#endif

ActivityTableModel::ActivityTableModel(QObject *parent)
    : QAbstractTableModel(parent)
    , sql_busy(false)
{
    TRACE();

    auto thread = new SqliteConsumerThread(SystemHelper::appDataPath(SqliteProducer::dataBaseFile), this);
    sql_db = thread->worker();
    Q_ASSERT(sql_db);
    connect(sql_db, &SqliteConsumer::queryError, this, &ActivityTableModel::setLastError, Qt::QueuedConnection);
    connect(sql_db, &SqliteConsumer::queryResult, this, &ActivityTableModel::onQueryResult, Qt::QueuedConnection);
    connect(ActivityCounter::instance(), &ActivityCounter::sqlDbChanged, this, &ActivityTableModel::execLastQuery);

    thread->start();
}

ActivityTableModel::~ActivityTableModel()
{
    TRACE();
}

QString ActivityTableModel::view() const
{
    return last_query;
}

void ActivityTableModel::setView(const QString &query)
{
    TRACE_ARG(query);

    QString q = query.simplified();
    if (q == last_query) return;

    bool busy = !q.isEmpty();
    if (busy) {
        if (sql_busy) {
            setLastError(tr("SQL database is busy"));
            return;
        }
        last_query = q;
        execLastQuery();
    } else {
        last_query.clear();
    }
    if (busy != sql_busy) {
        sql_busy = busy;
        emit busyChanged();
    }
}

void ActivityTableModel::execLastQuery()
{
    TRACE();

    if (sql_busy || last_query.isEmpty()) return;

    Q_ASSERT(sql_db);
    QMetaObject::invokeMethod(sql_db, "execQuery", Qt::QueuedConnection, Q_ARG(QString, last_query));

    sql_busy = true;
    emit busyChanged();
}

bool ActivityTableModel::busy() const
{
    return sql_busy;
}

QString ActivityTableModel::lastError() const
{
    return last_error;
}

void ActivityTableModel::setLastError(const QString &text)
{
    TRACE_ARG(text);

    if (sql_busy) {
        sql_busy = false;
        emit busyChanged();
    }
    if (text != last_error) {
        last_error = text;
        emit lastErrorChanged(text);
    }
}

QVariant ActivityTableModel::rawData(int row, int column) const
{
    //TRACE_ARG(row << column);

    QVariant var;
    if (row >= 0 && row < sql_data.size()) {
        const auto &map = sql_data.at(row);
        if (map.size() == header_keys.size() && column >= 0 && column < header_keys.size()) {
            var = map.value(header_keys.at(column)).toVariant();
            if (var.isNull() && var.canConvert<QString>())
                return "NULL";
        }
    }
    return var;
}

// static
QString ActivityTableModel::columnIdName(int column)
{
    return SqliteProducer::columnName(column);
}

// static
int ActivityTableModel::columnIdIndex(const QString &name)
{
    return SqliteProducer::columnIndex(name);
}

void ActivityTableModel::onQueryResult(const CborMapArray &rows)
{
    CborValueArray keys;
    for (const auto &row : rows) {
        if (keys.isEmpty()) {
            keys = row.keys();
        } else if (row.keys() != keys) {
            qWarning() << Q_FUNC_INFO << "CBOR columns mismatched";
            break;
        }
    }
    TRACE_ARG(keys << "rows" << rows.size());

    bool header_changed = (keys != header_keys);

    beginResetModel();
    if (header_changed) header_keys = keys;
    sql_data = rows;
    endResetModel();

    if (sql_busy) {
        sql_busy = false;
        emit busyChanged();
    }
    //if (header_changed) emit headerDataChanged(Qt::Horizontal, 0, header_keys.size() - 1);
    emit viewChanged();
}

// reimplemented from QAbstractTableModel
/*QHash<int, QByteArray> ActivityTableModel::roleNames() const
{
    static QHash<int, QByteArray> role_names;
    if (role_names.isEmpty()) {
        // 0="display", 1="decoration", 2="edit", 3="toolTip", 4="statusTip", 5="whatsThis", 256=UserRole
        role_names = QAbstractTableModel::roleNames();
        for (int i = 0; i < SqliteProducer::TotalColumns; i++) {
            role_names[Qt::UserRole + i] = SqliteProducer::dataBaseColumn[i];
        }
    }
    return role_names;
}*/

// reimplemented from QAbstractTableModel
int ActivityTableModel::columnCount(const QModelIndex&) const
{
    return header_keys.size();
}

// reimplemented from QAbstractTableModel
int ActivityTableModel::rowCount(const QModelIndex&) const
{
    return sql_data.size();
}

// reimplemented from QAbstractTableModel
QVariant ActivityTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    TRACE_ARG(section << orientation << role);

    if (role == Qt::DisplayRole) {
        switch (orientation) {
        case Qt::Horizontal:
            if (section >= 0 && section < header_keys.size())
                return header_keys.at(section).toVariant();
            break;
        case Qt::Vertical:
            if (section >= 0 && section < sql_data.size())
                return data(index(section, 0), role);
            break;
        }
    }
    return QVariant();
}

// reimplemented from QAbstractTableModel
QVariant ActivityTableModel::data(const QModelIndex &item, int role) const
{
    //TRACE_ARG(item.row() << item.column() << role);

    QVariant var;
    if (item.isValid() && role == Qt::DisplayRole) {
        var = rawData(item.row(), item.column());
        if (!item.column()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            auto type = var.type();
#else
            auto type = var.typeId();
#endif
            switch (type) {
            case QMetaType::QString:
                if (var.toString().count('-') != 4) return var;
                break;
            case QMetaType::QByteArray:
                if (var.toByteArray().count('-') != 4) return var;
                break;
            case QMetaType::QUuid:
                break;
            default:
                return var;
            }
            QString title = ActivityCounter::instance()->uuidCache(var.toUuid());
            if (!title.isEmpty()) return title;
        }
    }
    return var;
}
