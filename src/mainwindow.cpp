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
#include "mainwindow.h"
#include <QListWidgetItem>
#include "ui_mainwindow.h"
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include "registrationtooldialog.h"
#include "observedwindow.h"
#include "observedwindowsmanager.h"
#include "observedfilesmanager.h"
#include "database.h"
#include "model/model.h"

Model* Model::instance = 0;

MainWindow::MainWindow(Database* dataBase, ObservedWindowsManager* windowsManager, ObservedFilesManager* filesManager, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    windowsManager(windowsManager),
    filesManager(filesManager),
    dataBase(dataBase)
{
    ui->setupUi(this);

    QMenu* trayIconMenu = new QMenu(this);
    QAction* configureAction = new QAction("Configure");
    connect(configureAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    trayIconMenu->addAction(configureAction);

    QAction* registerAction = new QAction("Registration tool\tCTRL+F1");
    connect(registerAction, SIGNAL(triggered()), this, SLOT(onRegisterToolCalled()));
    trayIconMenu->addAction(registerAction);
    trayIconMenu->addSeparator();

    QAction* quitAction = new QAction("Quit");
    connect(quitAction, &QAction::triggered, QCoreApplication::quit);
    trayIconMenu->addAction(quitAction);

    QSystemTrayIcon* trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    QIcon icon(":/images/icon.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);

    trayIcon->show();

    this->connect(ui->databaseRefreshPushButton, SIGNAL(clicked(bool)), this, SLOT(refreshDatabaseListView()));
    refreshDatabaseListView();

    ui->refreshTimeSpinBox->setValue(Model::getInstance()->timeBetweenUpdates.getValue());
    ui->distanceThresholdSpinBox->setValue(Model::getInstance()->distanceThreshold.getValue());
    ui->nbAssociationsSpinBox->setValue(Model::getInstance()->nbAssociationsMax.getValue());
    ui->infoButtonCheckBox->setChecked(Model::getInstance()->showInfoButton.getValue());
    ui->redirectCheckBox->setChecked(Model::getInstance()->redirectAugmentedView.getValue());
    ui->accessibilityCheckbox->setChecked(Model::getInstance()->useAccessibility.getValue());
    ui->databasePathLineEdit->setText(dataBase->getUrl().toString());

    setWindowFlag(Qt::WindowStaysOnTopHint);
}


bool MainWindow::event(QEvent *event) {
    if (event->type() == QEvent::WindowActivate) {
        setActivationEnabled(true);
    } else if (event->type() == QEvent::WindowDeactivate) {
        setActivationEnabled(false);
    }
    return QMainWindow::event(event);
}

void MainWindow::onRegisterToolCalled() {
    QList<ObservedWindow*> onScreenWindows;

    windowId activeWindowId = getActiveWindow();
    ObservedWindow* activeWindow = NULL;

    for (auto window : windowsManager->getWindows()) {
        // Filter only onScreen window
        if (activeWindowId != 0 && window->getWid() == activeWindowId) {
            activeWindow = window;
        } else if (window->isOnScreen() && window->getTitle()[0] != 0) {
            onScreenWindows.append(window);
        }
    }

    if (activeWindow != NULL) {
        onScreenWindows.insert(0, activeWindow);
    }
    setActivationEnabled(true);
    RegistrationToolDialog dialog(dataBase, filesManager, onScreenWindows, this);
    dialog.exec();
    setActivationEnabled(false);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    this->hide();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::refreshDatabaseListView() {
    ui->databaseListWidget->clear();

    displayedFigureList = dataBase->getFigureList();

    for (auto figure : displayedFigureList) {
        ui->databaseListWidget->addItem(QString::number(figure.first) + ") " + figure.second);
    }
}

void MainWindow::on_databaseDeletePushButton_clicked() {
    int selectedIndex = ui->databaseListWidget->currentRow();

    if (selectedIndex >= 0 && selectedIndex < displayedFigureList.size()) {
        QPair<int, QString> selectedFigure = displayedFigureList.at(selectedIndex);
        dataBase->deleteFigure(selectedFigure.first);
        refreshDatabaseListView();
    }
}

void MainWindow::on_refreshTimeSpinBox_valueChanged(int val) {
    Model::getInstance()->timeBetweenUpdates.setValue(val);
}

void MainWindow::on_distanceThresholdSpinBox_valueChanged(double val) {
    Model::getInstance()->distanceThreshold.setValue(val);
}

void MainWindow::on_nbAssociationsSpinBox_valueChanged(int val) {
    Model::getInstance()->nbAssociationsMax.setValue(val);
}

void MainWindow::on_infoButtonCheckBox_stateChanged(int val) {
    Model::getInstance()->showInfoButton.setValue(val);
}

void MainWindow::on_redirectCheckBox_stateChanged(int val) {
    Model::getInstance()->redirectAugmentedView.setValue(val);
}

void MainWindow::on_databaseListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
    int width = 0, height = 0, fileSize = 0;
    QString md5;
    int selectedIndex = ui->databaseListWidget->currentRow();

    if (selectedIndex >= 0) {
        QPair<int, QString> selectedFigure = displayedFigureList.at(selectedIndex);
        dataBase->getFigureDetails(selectedFigure.first, &width, &height, &fileSize, &md5);
    }


    ui->md5Label->setText(md5);
    ui->fileSizeLabel->setText(QString::number(fileSize) + " octets");
    ui->imageSizeLabel->setText(QString::number(width) + "x" + QString::number(height));
}

void MainWindow::on_accessibilityCheckbox_stateChanged(int val){
    Model::getInstance()->useAccessibility.setValue(val);
}
