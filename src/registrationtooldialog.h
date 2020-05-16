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
#ifndef REGISTRATIONTOOLDIALOG_H
#define REGISTRATIONTOOLDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui {
class RegistrationToolDialog;
}

class Database;
class ObservedWindow;
class ObservedFilesManager;

class RegistrationToolDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegistrationToolDialog(Database* dataBase, ObservedFilesManager* filesManager, QList<ObservedWindow*> windows, QWidget *parent = 0);
    void showEvent(QShowEvent* event);
    ~RegistrationToolDialog();

private slots:
    void on_RegistrationToolDialog_accepted();
    void on_windowComboBox_currentIndexChanged(int index);
    void on_spinBoxModified();

    void on_documentPushButton_clicked();

    void on_augmentedFigurePushButton_clicked();

private:
    void redrawFigureSelection();
    void updateScreenshotLabel();
    void resizeEvent(QResizeEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent*);
    void blockSpinBoxSignals(bool block);
    QRect getFigureSelectionRect();
    void accept();

    Ui::RegistrationToolDialog *ui;
    QList<ObservedWindow*> windows; // Not a good idea -> could keep a ref to a deleted windows. TODO: Fix
    bool dragging;
    QPixmap screenshot;
    QPoint lastPos, initPos;
    Database* dataBase;
    ObservedFilesManager* filesManager;
};

#endif // REGISTRATIONTOOLDIALOG_H
