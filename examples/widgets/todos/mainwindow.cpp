#include <QtWidgets/qframe.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlistview.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qtoolbar.h>
#include <QtWidgets/qinputdialog.h>
#include <QtWidgets/qlayout.h>

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>

#include <Enginio/enginioclient.h>
#include <Enginio/enginioreply.h>

#include "applicationconfig.h"
#include "mainwindow.h"
#include "todosmodel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Enginio TODO example"));

    m_client = new EnginioClient(this);
    m_client->setBackendId(Enginio::BACKEND_ID);
    m_client->setBackendSecret(Enginio::BACKEND_SECRET);
    QObject::connect(m_client, &EnginioClient::error, this, &MainWindow::error);

    m_model = new TodosModel(this);
    m_model->setEnginio(m_client);

    QJsonObject query;
    query["objectType"] = QString::fromUtf8("objects.todos");
    m_model->setQuery(query);

    QToolBar *toolBar = new QToolBar(this);
    QPushButton *addNewButton = new QPushButton(toolBar);
    addNewButton->setText("Append");
    QObject::connect(addNewButton, &QPushButton::clicked, this, &MainWindow::appendItem);

    QPushButton *removeButton = new QPushButton(toolBar);
    removeButton->setText("Remove");
    QObject::connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeItem);

    QPushButton *toggleButton = new QPushButton(toolBar);
    toggleButton->setText("Toggle completed");
    QObject::connect(toggleButton, &QPushButton::clicked, this, &MainWindow::toggleCompleted);

    toolBar->addWidget(addNewButton);
    toolBar->addWidget(removeButton);
    toolBar->addWidget(toggleButton);

    m_view = new QListView(this);
    QFrame *frame = new QFrame(this);
    QVBoxLayout *windowLayout = new QVBoxLayout(frame);
    windowLayout->addWidget(m_view);
    windowLayout->addWidget(toolBar);
    setCentralWidget(frame);

    m_view->setModel(m_model);
}

QSize MainWindow::sizeHint() const
{
    return QSize(400, 600);
}

void MainWindow::error(EnginioReply *error)
{
    qWarning() << Q_FUNC_INFO << error;
}

void MainWindow::removeItem()
{
    QModelIndex index = m_view->currentIndex();
    EnginioReply *reply = m_model->remove(index.row());
    QObject::connect(reply, &EnginioReply::finished, reply, &EnginioReply::deleteLater);
}

void MainWindow::appendItem()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Create a new To Do")
                                         , tr("Title:"), QLineEdit::Normal
                                         , "", &ok);
    if (ok && !text.isEmpty()){
        QJsonObject object;
        object["title"] = text;
        object["completed"] = false;
        EnginioReply *reply = m_model->append(object);
        QObject::connect(reply, &EnginioReply::finished, reply, &EnginioReply::deleteLater);
    }
}

void MainWindow::toggleCompleted()
{
    QModelIndex index = m_view->currentIndex();
    bool completed = m_model->data(index, m_model->DoneRole).toBool();
    EnginioReply *reply = m_model->setProperty(index.row(), "completed", !completed);
    QObject::connect(reply, &EnginioReply::finished, reply, &EnginioReply::deleteLater);
}
