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
#include "figurefindertask.h"
#include "observedwindow.h"
#include "algorithms/featurematchingalgorithm.h"
#include "algorithms/surfalgorithm.h"
#include "figure.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <model/model.h>

FeatureMatchingAlgorithm* FigureFinderTask::featureMatchingAlgorithm = new SURFAlgorithm(300, 2, 3);

FigureFinderTask::FigureFinderTask(ObservedWindow* observedWindow) :
    observedWindow(observedWindow)  {
    this->setAutoDelete(true);
}

bool FigureFinderTask::getFigureRect(Figure* figure, std::vector<KeyPoint>& sceneKeypoints, Mat& sceneDescriptors, cv::Rect* figureRect, int* reason) {
    *reason = 1;

    if (sceneKeypoints.size() < 2)  {
        *reason = 2;
        return false;
    }

    featureMatchingAlgorithm->setDistanceThreshold(Model::getInstance()->distanceThreshold.getValue());
    featureMatchingAlgorithm->setNbAssociationMax(Model::getInstance()->nbAssociationsMax.getValue());

    std::vector<DMatch> matches = featureMatchingAlgorithm->match(figure->getDescriptors(), sceneDescriptors);
    if (matches.size() >= 3) { // Need at least 3 matches to compute the figure's rectangle.
        Rect rect = featureMatchingAlgorithm->computeObjectRect(figure->getWidth(), figure->getHeight(), matches, figure->getKeypoints(), sceneKeypoints);

        double aspectRatioA = ((double) figure->getWidth()) / figure->getHeight();
        double aspectRatioB = ((double) rect.width) / rect.height;
        bool aspectRatioCorrect = qAbs((1 - (aspectRatioA / aspectRatioB))) <= 0.1;

        if (rect.width > 10 && rect.height > 10 && aspectRatioCorrect) {
            figureRect->x = observedWindow->getX() + rect.x;
            figureRect->y = observedWindow->getY() + rect.y;
            figureRect->width = rect.width;
            figureRect->height = rect.height;
            *reason = 0;
            return true;
        }

        *reason = 3;
    }
    *reason = 4;

    return false;
}

void FigureFinderTask::run() {
    if (!observedWindow->getAnalysisMutex().tryLock(100)) {
        return;
    }

    bool hasChanged = false;
    cv::Mat scene = observedWindow->getScreenshot(&hasChanged);
    double hScrollPos = observedWindow->getHScrollPos();
    double vScrollPos = observedWindow->getVScrollPos();
    QRect initialRect = observedWindow->getScrollRect();

    if (!hasChanged && !observedWindow->wasVisible()) {
        observedWindow->getAugmentedViewsMutex().lock();
        for (auto augmentedView : observedWindow->getAugmentedViews()) {
            if (augmentedView->isFound() && !augmentedView->isVisible()) {
                emit augmentedView->figureFound(augmentedView->geometry());
            }
        }
        observedWindow->getAugmentedViewsMutex().unlock();
    } else if (!scene.empty()) {
        std::vector<KeyPoint> sceneKeypoints = featureMatchingAlgorithm->detect(scene);
        Mat sceneDescriptors = featureMatchingAlgorithm->compute(scene, sceneKeypoints);
        // By the time we reach this line (e.g. after the analysis)  the document might have been modified (e.g. resized, moved, or scrolled) making the results outdated
        // We detect this by comparing the current scroll position/geometry to the scroll position/geometry when we took the screenshot
        // If these are different, we just discard the results of the pixel analysis
        if (qAbs(hScrollPos - observedWindow->getHScrollPos()) < 0.1 && qAbs(vScrollPos - observedWindow->getVScrollPos()) < 0.1 && observedWindow->getScrollRect() == initialRect) {
            observedWindow->getAugmentedViewsMutex().lock();
            for (auto augmentedView : observedWindow->getAugmentedViews()) {
                cv::Rect figureRect;
                int reason = 0;
                if (getFigureRect(augmentedView->getReferenceFigure(), sceneKeypoints, sceneDescriptors, &figureRect, &reason)) {
                    emit augmentedView->figureFound(QRect(figureRect.x, figureRect.y, figureRect.width, figureRect.height));
                } else {
                    emit augmentedView->figureNotFound();
                }
            }
            observedWindow->getAugmentedViewsMutex().unlock();
        }
    }

    observedWindow->getAnalysisMutex().unlock();
}

FigureFinderTask::~FigureFinderTask() {
}
