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
#ifndef FEATUREMATCHINGALGORITHM_H
#define FEATUREMATCHINGALGORITHM_H

#include <Qt>
#include <QString>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/objdetect/objdetect.hpp>

using namespace cv;

class FeatureMatchingAlgorithm
{
public:
    FeatureMatchingAlgorithm();
    
    std::vector<KeyPoint> detect(Mat image);
    Mat compute(Mat image, std::vector<KeyPoint> keypoints);
    std::vector<DMatch> match(Mat objectDescriptors, Mat sceneDescriptors);
    Rect computeObjectRect(int imgWidth, int imgHeight, std::vector<DMatch> matches, std::vector<KeyPoint> objectKeypoints, std::vector<KeyPoint> sceneKeypoints);
    QString getDescription();

    void setNbAssociationMax(int nbAssociationMax) {this->nbAssociationMax = nbAssociationMax;}
    void setDistanceThreshold(double distanceThreshold) {this->distanceThreshold = distanceThreshold;}
    
protected:
    Ptr<FeatureDetector> detector;
    Ptr<DescriptorExtractor> descriptor;
    qint64 objectDetectTime;
    qint64 sceneDetectTime;
    qint64 objectComputeTime;
    qint64 sceneComputeTime;
    qint64 matchTime;
    qint64 computeRectTime;
    QString name;

private:
    int nbAssociationMax;
    double distanceThreshold;
    
};

#endif // FEATUREMATCHINGALGORITHM_H
