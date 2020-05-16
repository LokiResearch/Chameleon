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
/*
 * Observes files with augmented figures
 * If these files are modified, we want to prompt users to ask if the new version of the file should also be registered
*/
#include "observedfilesmanager.h"
#include "observedfile.h"
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QCheckBox>
#include "database.h"

ObservedFilesManager::ObservedFilesManager(Database* db):
    database(db) {
    checkTimer.setInterval(5000);
    this->connect(&checkTimer, &QTimer::timeout, this, &ObservedFilesManager::lookForModifiedFiles);

    checkTimer.start();
}


ObservedFile* ObservedFilesManager::addObservedFile(QString filePath) {
    ObservedFile* observedFile = new ObservedFile(filePath);
    files.insert(observedFile);

    return observedFile;
}

ObservedFile* ObservedFilesManager::addObservedFile(ObservedFile &file) {
    ObservedFile* observedFile = new ObservedFile(file);
    files.insert(observedFile);

    return observedFile;
}


void ObservedFilesManager::lookForModifiedFiles() {
    for (auto file : files) {
        QString previousMD5 = file->getMD5();
        if (file->isDifferent()) {
            int ret = file->rememberedDecision;

            if (ret == -1) {
                QMessageBox msgBox;
                QCheckBox* checkBox = new QCheckBox("Remember my decision for this document");
                msgBox.setText("The document" + file->getPath() + " was modified but contains augmented figures.");
                msgBox.setInformativeText("Do you want to register the new version to keep the augmentations?");
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                msgBox.setDefaultButton(QMessageBox::Save);
                msgBox.setCheckBox(checkBox);
                msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint);
                ret = msgBox.exec();
                if (checkBox->isChecked()) {
                    file->rememberedDecision = ret;
                }
            }

            if (ret == QMessageBox::Save) {
                database->updateMD5(previousMD5, file->getMD5(), file->getSize());
            }

        }
    }
}
