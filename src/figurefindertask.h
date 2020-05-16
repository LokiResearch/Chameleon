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
#ifndef FIGUREFINDERTASK_H
#define FIGUREFINDERTASK_H

#include <QRunnable>
#include <vector>
#include <opencv2/opencv.hpp>

class Figure;
class ObservedWindow;
class FeatureMatchingAlgorithm;

class FigureFinderTask : public QRunnable
{
public:
    FigureFinderTask(ObservedWindow* observedWindow);
    void run();
    bool getFigureRect(Figure* figure, std::vector<cv::KeyPoint>& sceneKeypoints, cv::Mat& sceneDescriptors, cv::Rect* figureRect, int* reason);
    ~FigureFinderTask();

    static FeatureMatchingAlgorithm* featureMatchingAlgorithm;


private:
   ObservedWindow* observedWindow;
};

#endif // FIGUREFINDERTASK_H
