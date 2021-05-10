#ifndef DEMODIALOG_H
#define DEMODIALOG_H

#include <QDialog>

class Database;

namespace Ui {
class DemoDialog;
}

class DemoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DemoDialog(Database* db, QWidget *parent = nullptr);
    ~DemoDialog();


private slots:
    void on_openDocumentButton_clicked();

private:
    Ui::DemoDialog *ui;
    Database* db;
};

#endif // DEMODIALOG_H
