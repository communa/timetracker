#ifndef ACTIVITYTABLEMODEL_H
#define ACTIVITYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QCborValue>

#include "SqliteConsumer.h"

class ActivityTableModel : public QAbstractTableModel // !QSqlQueryModel
{
    Q_OBJECT
    Q_DISABLE_COPY(ActivityTableModel)

    Q_PROPERTY(QString       view READ view      WRITE setView NOTIFY viewChanged FINAL)
    Q_PROPERTY(bool          busy READ busy      NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString  lastError READ lastError NOTIFY lastErrorChanged FINAL)

public:
    explicit ActivityTableModel(QObject *parent = nullptr);
    ~ActivityTableModel();

    QString view() const;
    void setView(const QString &query);

    bool busy() const;
    QString lastError() const;

    Q_INVOKABLE QVariant rawData(int row, int column) const;

    Q_INVOKABLE static QString columnIdName(int column);
    Q_INVOKABLE static int columnIdIndex(const QString &name);

    // reimplemented from QAbstractItemModel
    //QHash<int, QByteArray> roleNames() const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

signals:
    void viewChanged();
    void busyChanged();
    void lastErrorChanged(const QString &text);

private:
    void setLastError(const QString &text);
    void execLastQuery();
    void onQueryResult(const CborMapArray &rows);

    SqliteConsumer *sql_db;
    CborValueArray header_keys;
    CborMapArray sql_data;
    QString last_query;
    QString last_error;
    bool sql_busy;
};

#endif // ACTIVITYTABLEMODEL_H
