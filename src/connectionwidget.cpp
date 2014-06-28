/*
 * Copyright 2014 Oliver Giles
 * 
 * This file is part of SequelJoe. SequelJoe is licensed under the 
 * GNU GPL version 3. See LICENSE or <http://www.gnu.org/licenses/>
 * for more information
 */
#include "connectionwidget.h"

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QPushButton>
#include <QSettings>
#include <QListWidget>
#include <QListWidgetItem>
#include "dbconnection.h"
#include "sshdbconnection.h"
#include "favourites.h"
#include "passkeywidget.h"
#include <QSqlDatabase>
#include <QDebug>
ConnectionWidget::ConnectionWidget(QWidget *parent) :
    QWidget(parent)
{
    QBoxLayout* panes = new QHBoxLayout(this);
    panes->setContentsMargins(0,0,0,0);
    listWidget_ = new Favourites(this);


//    QSettings s;
//    foreach(QString c, s.childGroups()) {
//        s.beginGroup(c);
//        QListWidgetItem* item = new QListWidgetItem(s.value("Name", "Unnamed").toString());
//        item->setData(Qt::UserRole, c);
//        listWidget_->addItem(item);
//    }
    connect(listWidget_, SIGNAL(favouriteSelected(QString)), this, SLOT(loadSettings(QString)));
connect(this, SIGNAL(nameChanged(QString)), listWidget_, SLOT(updateName(QString)));

    panes->addWidget(listWidget_,1);
    {
        QWidget* panel = new QWidget(this);
    QBoxLayout* layout = new QVBoxLayout(panel);
    QGroupBox* boxSetup = new QGroupBox("Connection Setup", panel);
    QFormLayout* form = new QFormLayout(boxSetup);

    name_ = new QLineEdit(boxSetup);
    form->addRow("Name", name_);

    host_ = new QLineEdit(boxSetup);
    form->addRow("Host", host_);

    port_ = new QLineEdit(boxSetup);
    port_->setPlaceholderText(QString::number(DbConnection::DEFAULT_SQL_PORT));
    port_->setValidator(new QIntValidator);
    form->addRow("Port", port_);

    sqlType_ = new QComboBox(boxSetup);
    QSqlDatabase db;
    sqlType_->addItems(db.drivers());
    form->addRow("Connection Type", sqlType_);

    username_ = new QLineEdit(boxSetup);
    form->addRow("Username", username_);

    password_ = new QLineEdit(boxSetup);
    form->addRow("Password", password_);

    chkUseSsh_ = new QCheckBox("SSH Tunnel", boxSetup);
    form->addWidget(chkUseSsh_);
    {
        QGroupBox* boxSsh = new QGroupBox("SSH", boxSetup);
        boxSsh->setEnabled(false);
        connect(chkUseSsh_, SIGNAL(toggled(bool)), boxSsh, SLOT(setEnabled(bool)));

        QFormLayout* sshForm = new QFormLayout(boxSsh);

        sshHost_ = new QLineEdit(boxSsh);
        sshForm->addRow("Host", sshHost_);

        sshPort_ = new QLineEdit(boxSsh);
        sshPort_->setPlaceholderText(QString::number(SshDbConnection::DEFAULT_SSH_PORT));
        sshPort_->setValidator(new QIntValidator);
        sshForm->addRow("Port", sshPort_);

        sshUsername_ = new QLineEdit(boxSsh);
        sshForm->addRow("Username", sshUsername_);

        sshPassKey_ = new PassKeyWidget(boxSsh);
        sshForm->addRow("Password", sshPassKey_);

        form->setWidget(7, QFormLayout::SpanningRole, boxSsh);
    }
    QPushButton* pushButton = new QPushButton("Connect", boxSetup);
    form->addWidget(pushButton);
    connect(pushButton, &QPushButton::clicked, this, &ConnectionWidget::connectButtonClicked);

    layout->addWidget(boxSetup);
    layout->setAlignment(boxSetup, Qt::AlignCenter);

panel->setLayout(layout);
    panes->addWidget(panel,4);
}
    //this->setLayout(panes);

    connect(name_, SIGNAL(textEdited(QString)), this, SLOT(setupNameChanged(QString)));
    connect(host_, SIGNAL(textEdited(QString)), this, SLOT(setupHostChanged(QString)));
    connect(port_, SIGNAL(textEdited(QString)), this, SLOT(setupPortChanged(QString)));
    connect(sqlType_, SIGNAL(currentTextChanged(QString)), this, SLOT(setupSqlTypeChanged(QString)));
    connect(username_, SIGNAL(textEdited(QString)), this, SLOT(setupUserChanged(QString)));
    connect(password_, SIGNAL(textEdited(QString)), this, SLOT(setupPassChanged(QString)));
    connect(chkUseSsh_, SIGNAL(toggled(bool)), this, SLOT(setupUseSshChanged(bool)));
    connect(sshHost_, SIGNAL(textEdited(QString)), this, SLOT(setupSshHostChanged(QString)));
    connect(sshPort_, SIGNAL(textEdited(QString)), this, SLOT(setupSshPortChanged(QString)));
    connect(sshUsername_, SIGNAL(textEdited(QString)), this, SLOT(setupSshUserChanged(QString)));
    connect(sshPassKey_, SIGNAL(changed(bool,QString)), this, SLOT(setupSshPassKeyChanged(bool,QString)));

}
#include <QDebug>
void ConnectionWidget::connectButtonClicked() {
    emit doConnect(group_);
}
void ConnectionWidget::setupNameChanged(QString name) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue("Name", name);
    s.endGroup();
   // listItem_->setText(name);
    emit nameChanged(name);
}
void ConnectionWidget::setupHostChanged(QString host) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(DbConnection::KEY_HOST, host);
    s.endGroup();
}

void ConnectionWidget::setupPortChanged(QString port) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(DbConnection::KEY_PORT, port);
    s.endGroup();
}

void ConnectionWidget::setupSqlTypeChanged(QString type) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(DbConnection::KEY_TYPE, type);
    s.endGroup();
}

void ConnectionWidget::setupUserChanged(QString user) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(DbConnection::KEY_USER, user);
    s.endGroup();
}

void ConnectionWidget::setupPassChanged(QString pass) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(DbConnection::KEY_PASS, pass);
    s.endGroup();
}

void ConnectionWidget::setupUseSshChanged(bool ssh) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(SshDbConnection::KEY_USE_SSH, ssh);
    s.endGroup();
}
void ConnectionWidget::setupSshHostChanged(QString host) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(SshDbConnection::KEY_SSH_HOST, host);
    s.endGroup();
}
void ConnectionWidget::setupSshPortChanged(QString port) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(SshDbConnection::KEY_SSH_PORT, port);
    s.endGroup();
}
void ConnectionWidget::setupSshUserChanged(QString user) {
    QSettings s;
    s.beginGroup(group_);
    s.setValue(SshDbConnection::KEY_SSH_USER, user);
    s.endGroup();
}
void ConnectionWidget::setupSshPassKeyChanged(bool key, QString value) {
    QSettings s;
    s.beginGroup(group_);
    s.remove(key ? SshDbConnection::KEY_SSH_PASS : SshDbConnection::KEY_SSH_KEY);
    qDebug() << "removing " << (key ? SshDbConnection::KEY_SSH_PASS : SshDbConnection::KEY_SSH_KEY);
    qDebug() << "value: " << value;
    s.setValue(key ? SshDbConnection::KEY_SSH_KEY : SshDbConnection::KEY_SSH_PASS, value);
    s.endGroup();
}

#include <qdebug.h>
void ConnectionWidget::loadSettings(QString name) {
    group_ = name;
    QSettings s;
    s.beginGroup(group_);
    name_->setText(s.value("Name").toString());
    host_->setText(s.value(DbConnection::KEY_HOST).toString());
    sqlType_->setCurrentText(s.value(DbConnection::KEY_TYPE).toString());
    port_->setText(s.value(DbConnection::KEY_PORT).toString());
    username_->setText(s.value(DbConnection::KEY_USER).toString());
    password_->setText(s.value(DbConnection::KEY_PASS).toString());
    chkUseSsh_->setChecked(s.value(SshDbConnection::KEY_USE_SSH).toBool());
    sshHost_->setText(s.value(SshDbConnection::KEY_SSH_HOST).toString());
    sshPort_->setText(s.value(SshDbConnection::KEY_SSH_PORT).toString());
    sshUsername_->setText(s.value(SshDbConnection::KEY_SSH_USER).toString());
    //sshPassword_->setText(s.value(SshDbConnection::KEY_SSH_PASS).toString());
    if(s.contains(SshDbConnection::KEY_SSH_KEY)) sshPassKey_->setValue(true, s.value(SshDbConnection::KEY_SSH_KEY).toString());
    else sshPassKey_->setValue(false, s.value(SshDbConnection::KEY_SSH_PASS).toString());

    s.endGroup();
    //qDebug() << "loadsettings: "  << s;
}
/*
void SettingsWidget::setTableNames(QStringList tables) {
    listWidget_->addItems(tables);
}*/