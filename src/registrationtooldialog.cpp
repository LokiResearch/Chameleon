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
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include "registrationtooldialog.h"
#include "ui_registrationtooldialog.h"
#include "opencv2/opencv.hpp"
#include "observedwindow.h"
#include "observedfilesmanager.h"
#include "database.h"

RegistrationToolDialog::RegistrationToolDialog(Database* dataBase, ObservedFilesManager* filesManager, QList<ObservedWindow*> windows, QWidget*) :
    QDialog(NULL),
    ui(new Ui::RegistrationToolDialog),
    windows(windows),
    dataBase(dataBase),
    filesManager(filesManager) {
    ui->setupUi(this);
    dragging = false;

    setWindowFlags(Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowStaysOnTopHint);
    this->setFocusPolicy(Qt::StrongFocus);
    for (auto window : windows) {
        ui->windowComboBox->addItem(QString(window->getTitle()));
    }

    std::vector<std::string> files = getActiveWindowFiles();
    for (auto file : files) {
        ui->documentComboBox->addItem(QString::fromStdString(file));
    }

    this->connect(ui->xSpinBox, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxModified()));
    this->connect(ui->ySpinBox, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxModified()));
    this->connect(ui->widthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxModified()));
    this->connect(ui->heightSpinBox, SIGNAL(valueChanged(int)), this, SLOT(on_spinBoxModified()));
}

void RegistrationToolDialog::updateScreenshotLabel() {
    ObservedWindow* window = windows.at(ui->windowComboBox->currentIndex());

    cv::Mat scene = window->getScreenshot();
    QImage image = QImage((uchar*) scene.data, scene.cols, scene.rows, scene.step, scene.depth() < 24 ? QImage::Format_RGBA8888 : QImage::Format_RGB888);
    screenshot = QPixmap::fromImage(image);

    int width = ui->screenshotLabel->width();
    int height = ui->screenshotLabel->height();
    ui->screenshotLabel->setAlignment(Qt::AlignCenter);
    ui->screenshotLabel->setPixmap(screenshot.scaled(width, height, Qt::KeepAspectRatio));
}

void RegistrationToolDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    updateScreenshotLabel();
}

void RegistrationToolDialog::on_spinBoxModified() {
    initPos.setX(ui->xSpinBox->value());
    initPos.setY(ui->ySpinBox->value());
    lastPos.setX(ui->xSpinBox->value() + ui->widthSpinBox->value());
    lastPos.setY(ui->ySpinBox->value() + ui->heightSpinBox->value());

    redrawFigureSelection();
}

void RegistrationToolDialog::accept() {
    // Check that user inputs are correct before accepting

    QRect figureRect = getFigureSelectionRect();
    if (figureRect.width() == 0 || figureRect.height() == 0) {
        QMessageBox::critical(this, tr("Error"), tr("No figure selected."));
        return;
    }

    QString documentFile = ui->documentComboBox->currentText();
    if (!QFile(documentFile).exists()) {
        QMessageBox::critical(this, tr("Error"), tr("The document file specified (") + documentFile + tr(") does not exist"));
        return;
    }

    QDialog::accept();
}

void RegistrationToolDialog::on_RegistrationToolDialog_accepted() {
    QImage imgSelection = screenshot.copy(getFigureSelectionRect()).toImage().convertToFormat(QImage::Format_RGBA8888);
    cv::Mat cvSelection(imgSelection.height(), imgSelection.width(), CV_8UC4, (void *)imgSelection.constBits(), imgSelection.bytesPerLine());

    QString filePath = ui->documentComboBox->currentText();
    ObservedFile file(filePath);

    // Clean the input by removing empty spaces in the beginning/end
    QString input = ui->augmentedFigureLineEdit->text();
    input = input.trimmed();

    dataBase->saveFigureInDb(cvSelection, file, QUrl::fromUserInput(input));
    filesManager->addObservedFile(file);

    lookForOpenedFiles();
}

void RegistrationToolDialog::on_windowComboBox_currentIndexChanged(int) {
    updateScreenshotLabel();
}

void RegistrationToolDialog::resizeEvent(QResizeEvent*) {
    updateScreenshotLabel();
}

QRect RegistrationToolDialog::getFigureSelectionRect() {
    int px = qMin(initPos.x(), lastPos.x());
    int py = qMin(initPos.y(), lastPos.y());
    int lx = qMax(initPos.x(), lastPos.x());
    int ly = qMax(initPos.y(), lastPos.y());

    return QRect(px, py, lx - px, ly - py);
}

void RegistrationToolDialog::redrawFigureSelection() {
    QImage tmp = screenshot.toImage();
    QPainter painter(&tmp);

    painter.setPen(QColor(0, 255, 0, 100));
    painter.setBrush(QColor(0, 255, 0, 100));

    painter.drawRect(initPos.x(), initPos.y(), lastPos.x() - initPos.x(), lastPos.y() - initPos.y());

    painter.end();
    int width = ui->screenshotLabel->width();
    int height = ui->screenshotLabel->height();
    ui->screenshotLabel->setPixmap(QPixmap::fromImage(tmp).scaled(width, height, Qt::KeepAspectRatio));
}

void RegistrationToolDialog::blockSpinBoxSignals(bool block) {
    ui->xSpinBox->blockSignals(block);
    ui->ySpinBox->blockSignals(block);
    ui->widthSpinBox->blockSignals(block);
    ui->heightSpinBox->blockSignals(block);
}

void RegistrationToolDialog::mouseMoveEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) {
        QPoint pos = ui->screenshotLabel->mapFrom(this, event->pos());

        int cx = (ui->screenshotLabel->width() - ui->screenshotLabel->pixmap()->width()) / 2;
        int cy = (ui->screenshotLabel->height() - ui->screenshotLabel->pixmap()->height()) / 2;

        pos = QPoint(pos.x() - cx, pos.y() - cy);

        QTransform transformation;
        QTransform::quadToQuad(QPolygon(ui->screenshotLabel->pixmap()->rect()), QPolygon(screenshot.rect()), transformation);
        pos = transformation.map(pos);

        lastPos = pos;

        if (dragging) {
            redrawFigureSelection();
            blockSpinBoxSignals(true);
            QRect figureRect = getFigureSelectionRect();
            ui->xSpinBox->setValue(figureRect.x());
            ui->ySpinBox->setValue(figureRect.y());
            ui->widthSpinBox->setValue(figureRect.width());
            ui->heightSpinBox->setValue(figureRect.height());
            blockSpinBoxSignals(false);
        } else {
            dragging = true;
            initPos = pos;
        }
    }
}

void RegistrationToolDialog::mouseReleaseEvent(QMouseEvent*) {
    dragging = false;
}

RegistrationToolDialog::~RegistrationToolDialog() {
    delete ui;
}

void RegistrationToolDialog::on_documentPushButton_clicked() {
    QUrl fileUrl = QUrl::fromLocalFile(QFileDialog::getOpenFileName(this, tr("Open Document")));
    ui->documentComboBox->insertItem(0, fileUrl.toString());
    ui->documentComboBox->setCurrentIndex(0);
}

void RegistrationToolDialog::on_augmentedFigurePushButton_clicked() {
    QUrl fileUrl = QUrl::fromLocalFile(QFileDialog::getOpenFileName(this, tr("Open Augmented Figure")));
    ui->augmentedFigureLineEdit->setText(fileUrl.toString());
}
