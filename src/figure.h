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
#ifndef FIGURE_H
#define FIGURE_H

#include <vector>
#include <QUrl>
#include <QObject>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/features2d.hpp>

class Figure : public QObject
{
    Q_OBJECT
public:
    Figure(int id, int width, int height, std::vector<cv::KeyPoint> keypoints, cv::Mat descriptors, QUrl url);

    inline int getId() {return id;}
    inline int getWidth() {return width;}
    inline int getHeight() {return height;}
    inline std::vector<cv::KeyPoint>& getKeypoints() {return keypoints;}
    inline cv::Mat& getDescriptors() {return descriptors;}
    inline QUrl getUrl() {return url;}

    bool operator==(const Figure& other) const {return other.id == this->id;}

signals:
    void deleted(int id);

private:
    int id;
    int width;
    int height;
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    QUrl url;
};

#endif // FIGURE_H
