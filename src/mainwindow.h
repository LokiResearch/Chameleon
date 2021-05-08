/* Copyright 2020 Damien Masson, Sylvain Malacria, Edward Lank, Géry Casiez
               (University of Waterloo, Université de Lille, Inria, France)

This file is part of Chameleon.

Chameleon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Chameleon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Chameleon.  If not, see <https://www.gnu.org/licenses/>. */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Database;
class ObservedWindowsManager;
class ObservedFilesManager;
class QListWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Database* dataBase, ObservedWindowsManager* windowsManager, ObservedFilesManager* filesManager, QWidget *parent = 0);
    void closeEvent(QCloseEvent *event);
    ~MainWindow();

public slots:
    void onRegisterToolCalled();

private slots:
    void refreshDatabaseListView();

    void on_databaseDeletePushButton_clicked();

    void on_refreshTimeSpinBox_valueChanged(int arg1);

    void on_distanceThresholdSpinBox_valueChanged(double arg1);

    void on_nbAssociationsSpinBox_valueChanged(int arg1);

    void on_infoButtonCheckBox_stateChanged(int arg1);

    void on_redirectCheckBox_stateChanged(int arg1);

    void on_databaseListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_accessibilityCheckbox_stateChanged(int arg1);

    void on_onlyActiveWndCheckbox_stateChanged(int arg1);

private:
    bool event(QEvent *event);

    Ui::MainWindow *ui;
    ObservedWindowsManager* windowsManager;
    ObservedFilesManager* filesManager;
    QList<QPair<int, QString>> displayedFigureList;
    Database* dataBase;
};

#endif // MAINWINDOW_H
