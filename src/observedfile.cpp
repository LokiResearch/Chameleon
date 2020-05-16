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
#include "observedfile.h"

#include <QFile>
#include <QCryptographicHash>

ObservedFile::ObservedFile(QString filePath) :
    rememberedDecision(-1),
    path(filePath)
{
    QFile file(filePath);
    if (file.open(QFile::ReadOnly)) {
        cachedSize = file.size();
        cachedMD5 = getMD5(file);
        file.close();
    }
}

ObservedFile::ObservedFile(ObservedFile& file):
    rememberedDecision(-1),
    path(file.path),
    cachedMD5(file.cachedMD5),
    cachedSize(file.cachedSize)
     {
}

// Test if the file was modified
bool ObservedFile::isDifferent() {
    QFile file(path);
    if (file.open(QFile::ReadOnly)) {
        qint64 newSize = file.size();
        QString newMD5 = getMD5(file);

        if (cachedMD5 != newMD5) {
            this->cachedSize = newSize;
            this->cachedMD5 = newMD5;
            return true;
        }
    }

    return false;
}

QString ObservedFile::getMD5(QFile& file) {
    QCryptographicHash hash(QCryptographicHash::Algorithm::Md5);
    if (hash.addData(&file)) {
        return QString::fromStdString(hash.result().toHex().toStdString());
    }

    return QString::fromStdString(QCryptographicHash::hash((file.fileName().toUtf8()), QCryptographicHash::Md5).toHex().toStdString());
}


bool ObservedFile::operator==(const ObservedFile &other) const {
    return other.path == path;
}

QString ObservedFile::getPath() const {
    return path;
}

QString ObservedFile::getMD5() {
    return cachedMD5;
}

qint64 ObservedFile::getSize() {
    return cachedSize;
}
