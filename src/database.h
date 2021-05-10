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
#ifndef DATABASE_H
#define DATABASE_H

#include <QList>
#include <QSqlDatabase>
#include <QUrl>
#include <opencv2/opencv.hpp>
#include "observedfile.h"

class Figure;

class Database
{
public:
    Database();
    void load();
    void saveFigureInDb(cv::Mat image, ObservedFile& file, QUrl url);
    void deleteFigure(int id);

    QList<Figure*> getFiguresOfFile(const char* filePath);
    QList<QPair<int, QString>> getFigureList();
    bool getFigureDetails(int figureId, int* width, int* height, int* fileSize, QString* md5);
    void updateMD5(QString oldMD5, QString newMD5, qint64 newSize);
    void updateFigureUrl(QString oldUrl, QString newUrl);
    inline QUrl getUrl() {return url;}

private:
    QMutex databaseAccess;
    QSqlDatabase db;
    QList<Figure*> figures;
    QUrl url;
};

#endif // DATABASE_H
