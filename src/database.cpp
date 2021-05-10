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
#include <QSqlQuery>
#include <QSqlError>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/features2d.hpp>
#include <QFile>
#include <QStandardPaths>
#include "database.h"
#include "figure.h"
#include <QVariant>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QDir>
#include <QDebug>
#include <QFile>
#include "figurefindertask.h"
#include "algorithms/featurematchingalgorithm.h"

using namespace cv;

Database::Database() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    url = QUrl(dbLocation + "/figures.db");
}

void Database::load() {
    db.setDatabaseName(url.toString());
    if (!db.open()) {
        qDebug() << "Couldn't open database" << db.lastError();
    }

    // Required only once, but will fail silently if already done
    QSqlQuery query;
    query.exec("create table figures (id integer primary key, filesize integer, md5 string, width integer, height integer, keypoints string, descriptors string, url string)");
}


// Load the augmented figures stored in the database for the specified file
QList<Figure*> Database::getFiguresOfFile(const char* filePath) {
    databaseAccess.lock();
    QList<Figure*> result;
    QFile file(filePath);


    QSqlQuery query(db);
    query.prepare("SELECT width, height, keypoints, descriptors, url, id, md5 FROM figures WHERE filesize = (:filesize)");
    query.bindValue(":filesize", file.size());

    if (query.exec()) {
        QString fileMD5;

        while (query.next()) {
            if (fileMD5.isEmpty()) {
                ObservedFile file(filePath);
                fileMD5 = file.getMD5();
            }

            QString md5 = query.value("md5").toString();

            if (fileMD5 == md5) {
                int width = query.value(0).toInt();
                int height = query.value(1).toInt();
                QString keypoints = query.value(2).toString();
                QString descriptors = query.value(3).toString();
                QString url = query.value(4).toString();
                int id = query.value(5).toInt();

                std::vector<KeyPoint> keypointsVect;
                Mat descriptorsMat;


                Figure* figure = NULL;

                // Look in already loaded figures
                for (auto f : figures) {
                    if (f->getId() == id) {
                        figure = f;
                        break;
                    }
                }

                if (figure == NULL) {
                    figure = new Figure(id, width, height, keypointsVect, descriptorsMat, QUrl(url));

                    cv::FileStorage keypointsFile(keypoints.toStdString(), cv::FileStorage::READ + FileStorage::MEMORY);
                    keypointsFile["keypoints"] >> figure->getKeypoints();

                    cv::FileStorage descriptorsFile(descriptors.toStdString(), cv::FileStorage::READ + FileStorage::MEMORY);
                    descriptorsFile["descriptors"] >> figure->getDescriptors();

                    figures.append(figure);
                }

                result.append(figure);
            }
        }
    }
    databaseAccess.unlock();

    return result;
}

void Database::saveFigureInDb(Mat image, ObservedFile& file, QUrl url) {
    databaseAccess.lock();
    std::vector<KeyPoint> vectKeypoints = FigureFinderTask::featureMatchingAlgorithm->detect(image);

    cv::FileStorage keypoints(".bin", FileStorage::WRITE + FileStorage::MEMORY);
    keypoints << "keypoints" << vectKeypoints;

    cv::FileStorage descriptors(".bin", FileStorage::WRITE + FileStorage::MEMORY);
    Mat descriptorsMat = FigureFinderTask::featureMatchingAlgorithm->compute(image, vectKeypoints);
    descriptors << "descriptors" << descriptorsMat;

    qint64 size = file.getSize();
    QString md5 = file.getMD5();

    QSqlQuery query(db);
    query.prepare("INSERT INTO figures (filesize, md5, width, height, keypoints, descriptors, url) VALUES (:filesize, :md5, :width, :height, :keypoints, :descriptors, :url)");
    query.bindValue(":filesize", size);
    query.bindValue(":md5", md5);
    query.bindValue(":width", image.cols);
    query.bindValue(":height", image.rows);
    query.bindValue(":keypoints", keypoints.releaseAndGetString().c_str());
    query.bindValue(":descriptors", descriptors.releaseAndGetString().c_str());
    query.bindValue(":url", url.toString());
    query.exec();

    image.release();
    databaseAccess.unlock();
}

// Load all the figures stored in the database
QList<QPair<int, QString>> Database::getFigureList() {
    databaseAccess.lock();
    QList<QPair<int, QString>> result;

    QSqlQuery query(db);
    query.prepare("SELECT url, id FROM figures");

    if (query.exec()) {

        while (query.next()) {
            QString url = query.value(0).toString();
            int id = query.value(1).toInt();
            result.append(QPair<int, QString>(id, url));
        }
    }
    databaseAccess.unlock();
    return result;
}

bool Database::getFigureDetails(int figureId, int* width, int* height, int* fileSize, QString* md5) {
    databaseAccess.lock();
    QSqlQuery query(db);
    query.prepare("SELECT width, height, filesize, md5 FROM figures WHERE id = :figureId");
    query.bindValue(":figureId", figureId);
    bool res = false;

    if (query.exec() && query.next()) {
        *width = query.value(0).toInt();
        *height = query.value(1).toInt();
        *fileSize = query.value(2).toInt();
        *md5 = query.value(3).toString();
        res = true;
    }

    databaseAccess.unlock();
    return res;
}

void Database::updateMD5(QString oldMD5, QString newMD5, qint64 newSize) {
    databaseAccess.lock();
    db.exec("update figures set md5 = '"+ newMD5 + "', filesize = "+ QString::number(newSize) + " where md5 = '" + oldMD5 + "'");
    databaseAccess.unlock();
}

void Database::updateFigureUrl(QString oldUrl, QString newUrl) {
    databaseAccess.lock();
    db.exec("update figures set url = '"+ newUrl + "' where url = '" + oldUrl + "'");
    databaseAccess.unlock();
}

void Database::deleteFigure(int id) {
    databaseAccess.lock();
    db.exec("delete from figures where id = " + QString::number(id));

    QMutableListIterator<Figure*> i(figures);
    while (i.hasNext()) {
        Figure* figure = i.next();
        if (figure->getId() == id) {
            i.remove();
            emit figure->deleted(id);
            delete figure;
        }
    }
    databaseAccess.unlock();
}
