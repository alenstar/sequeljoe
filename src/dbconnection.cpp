/*
 * Copyright 2014 Oliver Giles
 * 
 * This file is part of SequelJoe. SequelJoe is licensed under the 
 * GNU GPL version 3. See LICENSE or <http://www.gnu.org/licenses/>
 * for more information
 */
#include "dbconnection.h"

#include "schemamodel.h"
#include "sqlmodel.h"
#include "notify.h"
#include "savedconfig.h"
#include "sshthread.h"
#include "driver.h"
#include "tabledata.h"

#include <QSqlResult>
#include <QSettings>
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QSqlQuery>
#include <QStringList>
#include <QThread>
#include <QSqlError>
#include <QApplication>
#include <QSqlRecord>

int DbConnection::nConnections = 0;

DbConnection::DbConnection(const QSettings &settings) {
    tunnel = {0,0};

    sqlParams.host = settings.value(SavedConfig::KEY_HOST).toByteArray();
    sqlParams.port = settings.value(SavedConfig::KEY_PORT).toInt();
    if(sqlParams.port == 0)
        sqlParams.port = SavedConfig::DEFAULT_SQL_PORT;
    sqlParams.user = settings.value(SavedConfig::KEY_USER).toByteArray();
    sqlParams.pass = settings.value(SavedConfig::KEY_PASS).toByteArray();
    sqlParams.driverName = settings.value(SavedConfig::KEY_TYPE).toString();
    sqlParams.dbName = settings.value(SavedConfig::KEY_DBNM).toByteArray();


    useSshTunnel = settings.value(SavedConfig::KEY_USE_SSH).toBool();
    if(useSshTunnel) {
        sshParams.sshHost = settings.value(SavedConfig::KEY_SSH_HOST).toByteArray();
        sshParams.sshPort = settings.value(SavedConfig::KEY_SSH_PORT).toInt() == 0 ?
                    QString::number(SavedConfig::DEFAULT_SSH_PORT).toLocal8Bit() :
                    settings.value(SavedConfig::KEY_SSH_PORT).toByteArray();
        sshParams.remoteHost = sqlParams.host;
        sshParams.remotePort = sqlParams.port;
        sshParams.sshUser = settings.value(SavedConfig::KEY_SSH_USER).toByteArray();
        sshParams.useSshKey = settings.contains(SavedConfig::KEY_SSH_KEY);
        sshParams.sshKeyPath = settings.value(SavedConfig::KEY_SSH_KEY).toByteArray();
        sshParams.sshPass = settings.value(SavedConfig::KEY_SSH_PASS).toByteArray();
    }

    driver = Driver::createDriver(sqlParams.driverName);
}

DbConnection::~DbConnection() {
    delete driver;
    delete tunnel.ssh;
    if(tunnel.thread) {
        tunnel.thread->exit();
        tunnel.thread->wait();
        delete tunnel.thread;
    }
}

QStringList DbConnection::tables() const {
    return tableNames;
}

void DbConnection::cleanup() {
    QString name = driver->connectionName();
    driver->close();
    *((QSqlDatabase*) driver) = QSqlDatabase{};
    QSqlDatabase::removeDatabase(name);
}

QSqlQueryModel* DbConnection::query(QString q, QSqlQueryModel* update) {
    if(!update)
        update = new QSqlQueryModel;
    update->setQuery(q, *driver);
    return update;
}

int DbConnection::execQuery(QSqlQuery& q) const {
    q.exec();
    QString msg;
    int nRows = 0;
    if(q.lastError().isValid()) {
        msg = "Error: " + q.lastError().text();
    } else {
        if(q.isSelect()) {
            nRows = driver->countRows(q);
            msg = QString::number(nRows) + " rows retrieved";
        } else {
            nRows = q.numRowsAffected();
            msg = QString::number(nRows) + " rows affected";
        }
    }
    if(qApp->focusWindow() == 0) {
        Notifier::instance()->send("Query complete", msg.toLocal8Bit().constData());
    }
    emit queryExecuted(q.lastQuery(), msg);
    return nRows;
}

void DbConnection::queryTableMetadata(QString tableName, QObject* callbackOwner, const char* callbackName) {
    QMetaObject::invokeMethod(callbackOwner, callbackName, Qt::QueuedConnection, Q_ARG(TableMetadata, driver->metadata(tableName)));
}

void DbConnection::queryTableContent(QSqlQuery* query, QObject* callbackOwner, const char* callbackName) {
    int nRows = execQuery(*query);
    QMetaObject::invokeMethod(callbackOwner, callbackName, Qt::QueuedConnection, Q_ARG(int, nRows));
}

QStringList DbConnection::columnNames(QString table) const {
    QStringList names;
    QSqlRecord record = driver->record(table);
    for(int i = 0; i < record.count(); ++i)
        names << record.fieldName(i);
    return names;
}
void DbConnection::queryTableColumns(Schema* res, QString tableName, QObject* callbackOwner, const char* callbackName) {
    driver->columns(*res, tableName);
    QMetaObject::invokeMethod(callbackOwner, callbackName, Qt::QueuedConnection, Q_ARG(int, /*unused:*/0));
}

void DbConnection::queryTableUpdate(QString query, QObject *callbackOwner, const char *callbackName) {
    QSqlQuery q(*driver);
    q.prepare(query);
    int rowsAffected = execQuery(q);
    QMetaObject::invokeMethod(callbackOwner, callbackName, Qt::QueuedConnection, Q_ARG(int, rowsAffected), Q_ARG(int, q.lastInsertId().toInt()));
}

void DbConnection::populateDatabases() {
    dbNames = driver->databases();
}

QString DbConnection::queryCreateTable(QString tableName) {
    QSqlQuery query(*driver);
    query.exec("SHOW CREATE TABLE \"" + tableName + "\"");
    query.next();
    return query.value(1).toString();
}

void DbConnection::createTable(QString tableName) {
    QSqlQuery query(*driver);
    query.prepare(driver->createTableQuery(tableName));
    execQuery(query);
}

void DbConnection::deleteTable(QString tableName) {
    QSqlQuery query(*driver);
    query.prepare("DROP TABLE \"" + tableName + "\"");
    execQuery(query);
}

void DbConnection::start() {
    if(useSshTunnel) {
        tunnel.thread = new QThread;
        tunnel.ssh = new SshThread(sshParams);
        tunnel.ssh->moveToThread(tunnel.thread);
        connect(tunnel.thread, SIGNAL(started()), tunnel.ssh, SLOT(connectToServer()));
        connect(tunnel.ssh, SIGNAL(sshTunnelOpened(QString,int)), this, SLOT(openDatabase(QString,int)));
        connect(tunnel.ssh, SIGNAL(tunnelFailed(QString)), this, SIGNAL(connectionFailed(QString)));
        connect(tunnel.ssh, SIGNAL(confirmUnknownHost(QString,bool*)), this, SIGNAL(confirmUnknownHost(QString,bool*)), Qt::BlockingQueuedConnection);
        tunnel.thread->start();
    } else {
        openDatabase(sqlParams.host, sqlParams.port);
    }
}

QString DbConnection::databaseName() const {
    return driver->databaseName();
}

void DbConnection::openDatabase(QString host, int port) {
    QString name = "connection_" + QString::number(nConnections++);
    *((QSqlDatabase*) driver) = QSqlDatabase::addDatabase(sqlParams.driverName, name);
    driver->setHostName(host);
    driver->setPort(port);
    driver->setDatabaseName(sqlParams.dbName);
    driver->setUserName(sqlParams.user);
    driver->setPassword(sqlParams.pass);
    if(driver->open()) {
        populateDatabases();
        if(!sqlParams.dbName.isEmpty())
            populateTables();
        emit connectionSuccess();
    } else {
        emit connectionFailed(driver->lastError().text());
    }
}

void DbConnection::useDatabase(QString dbName) {
    QSqlQuery query(*driver);
    query.prepare("USE \"" + dbName + "\"");
    execQuery(query);
    driver->setDatabaseName(dbName);
    populateTables();
    emit databaseChanged(dbName);
}

void DbConnection::populateTables() {
    tableNames = driver->tableNames();
}
