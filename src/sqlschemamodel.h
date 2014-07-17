#ifndef SQLSCHEMAMODEL_H
#define SQLSCHEMAMODEL_H

#include <QAbstractTableModel>

class QSqlQuery;
class QSqlDatabase;

struct SqlColumn {
    QString name;
    QString type;
    QString length;
    bool is_unsigned;
    bool allow_null;
    QString key;
    QString default_value;
    QString extra;
    QString collation;
    QString comment;
};

class SqlSchemaModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit SqlSchemaModel(QSqlDatabase* db, QString tableName, QObject *parent = 0);
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &item, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
signals:

public slots:
private:
    QSqlQuery* query_;
    QVector<SqlColumn> columns_;
    QString tableName_;
    QSqlDatabase& db_;
};

#endif // SQLSCHEMAMODEL_H