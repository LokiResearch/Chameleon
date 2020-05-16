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
#include "featurematchingalgorithm.h"
#include <QDebug>
#include <QDateTime>

using namespace cv;

FeatureMatchingAlgorithm::FeatureMatchingAlgorithm() {
    this->name = "FeatureMatchingAlgorithm";
    nbAssociationMax = -1;
    distanceThreshold = -1;
}

std::vector<KeyPoint> FeatureMatchingAlgorithm::detect(Mat image) {
    std::vector<KeyPoint> keypoints;
    
    detector->detect(image, keypoints);
    
    return keypoints;
}

Mat FeatureMatchingAlgorithm::compute(Mat image, std::vector<KeyPoint> keypoints) {
    Mat descriptors;
    descriptor->compute(image, keypoints, descriptors);
    return descriptors;
}

bool matchComparison(DMatch a, DMatch b) {
    return a.distance < b.distance;
}

std::vector<DMatch> FeatureMatchingAlgorithm::match(Mat objectDescriptors, Mat sceneDescriptors) {
    int norm = NORM_L2;

    if (objectDescriptors.type() != CV_32F) {
        norm = NORM_HAMMING;
    }

    BFMatcher matcher(norm);
    std::vector<DMatch> matches;
    matcher.match(objectDescriptors, sceneDescriptors, matches);
    
    std::vector< DMatch > goodMatches;
    std::sort(matches.begin(), matches.end(), matchComparison);
    
    int maxAssociations = nbAssociationMax > 0 ? qMin((int) matches.size(), nbAssociationMax) : (int) matches.size();

    for (int i = 0; i < maxAssociations; ++i) {
        if (distanceThreshold > 0 && matches.at(i).distance >= distanceThreshold) {
            break;
        }
        goodMatches.push_back(matches.at(i));
    }
    
    return goodMatches;
}

Rect FeatureMatchingAlgorithm::computeObjectRect(int imgWidth, int imgHeight, std::vector<DMatch> matches, std::vector<KeyPoint> objectKeypoints, std::vector<KeyPoint> sceneKeypoints) {
    std::vector<Point2f> obj;
    std::vector<Point2f> scen;

    if (matches.size() <= 4) {
        return Rect(-1, -1, -1, -1);
    }
    
    for (int i = 0; i < (int) matches.size(); i++) {
        obj.push_back(objectKeypoints[matches[i].queryIdx].pt);
        scen.push_back(sceneKeypoints[matches[i].trainIdx].pt);
    }
    
    Mat H = findHomography(obj, scen, RANSAC);
    
    if (H.empty()) {
        return Rect(-1, -1, -1, -1);
    }

    std::vector<Point2f> objCorners(4);
    objCorners[0] = Point2f(0,0); objCorners[1] = Point2f(imgWidth, 0);
    objCorners[2] = Point2f(imgWidth, imgHeight); objCorners[3] = Point2f(0, imgHeight);
    std::vector<Point2f> sceneCorners(4);
    
    perspectiveTransform(objCorners, sceneCorners, H);
    
    int x = (sceneCorners[0]).x;
    int y = (sceneCorners[0]).y;
    int width = sceneCorners[2].x - x;
    int height = sceneCorners[2].y - y;
    
    return Rect(x, y, width, height);
}

QString FeatureMatchingAlgorithm::getDescription() {
    qint64 detectTime = objectDetectTime + sceneDetectTime;
    qint64 computeTime = objectComputeTime + sceneComputeTime;
    qint64 totalTime = detectTime + computeTime + matchTime + computeRectTime;
    
    return name + " in " + QString::number(totalTime) + "msecs (" + QString::number(detectTime) +
            "/" + QString::number(computeTime) + "/" + QString::number(matchTime) + "/" + QString::number(computeRectTime) + ")";
}
