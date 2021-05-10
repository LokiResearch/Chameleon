#include "demodialog.h"
#include "ui_demodialog.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QFile>
#include <QDesktopServices>
#include <QTemporaryDir>
#include <QDebug>
#include <QDir>
#include "database.h"

DemoDialog::DemoDialog(Database* db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DemoDialog),
    db(db) {

    ui->setupUi(this);
}

DemoDialog::~DemoDialog() {
    delete ui;
}

void DemoDialog::on_openDocumentButton_clicked() {
    // Copy all necessary files from resources to a temporary folder
    QTemporaryDir dir;
    dir.setAutoRemove(false);

    QUrl paperUrl = QUrl(dir.filePath("paper.pdf"));
    QUrl bubbleUrl = QUrl(dir.filePath("bubble.html"));
    QUrl dbUrl = db->getUrl();

    Q_ASSERT_X(QFile::copy(":/demo/paper.pdf", paperUrl.toString()), "File copy", "Could not copy paper.pdf");
    Q_ASSERT_X(QFile::copy(":/demo/bubble.html", bubbleUrl.toString()), "File copy", "Could not copy bubble.html");
    Q_ASSERT_X(QFile::copy(":/demo/figures.db", dbUrl.toString()), "File copy", "Could not copy figures.db");
    QFile::setPermissions(dbUrl.toString(), QFileDevice::ReadOwner|QFileDevice::WriteOwner);

    db->load();

    // The ressource for the bubble cursor is located in a temporary folder, we need to update the database accordingly
    db->updateFigureUrl("bubble.html", QUrl::fromLocalFile(bubbleUrl.toString()).toString());

    // Open the document using the default PDF viewer
    QDesktopServices::openUrl(QUrl::fromLocalFile(paperUrl.toString()).toString());

    this->accept();
}
