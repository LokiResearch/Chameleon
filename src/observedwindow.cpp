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
#include "observedwindow.h"
#include "figure.h"
#include "os_specific/window.h"
#include <QThread>
#include <QDebug>
#include <QDateTime>

ObservedWindow::ObservedWindow(processId pid, windowId wid) :
    pid(pid), wid(wid) {
    hasScreenshot = false;
    hasMoved = false;
    title[0] = 0;
    lastVisible = false;
    visible = false;
    hasScrollPos = false;
    lastScrollTime = 0;
}

// Add a new figure to look for in the window
void ObservedWindow::addFigure(Figure* figure) {
    augmentedViewsMutex.lock();
    for (auto augmentedView : augmentedViews) {
        if (augmentedView->getReferenceFigure()->getId() == figure->getId()) {
            // This figure was already added to this window
            augmentedViewsMutex.unlock();
            return;
        }
    }
    augmentedViews.append(new AugmentedView(figure, this));

    augmentedViewsMutex.unlock();
}

void ObservedWindow::onWindowScrolled(QRect scrollRect, double horizontalPos, double verticalPos) {
    // TODO : Handle multiple scrollArea in the same window
    augmentedViewsMutex.lock();
    if (hasScrollPos) {
        double scrollDeltaX = horizontalPos - lastHorizontalScrollPos;
        double scrollDeltaY = verticalPos - lastVerticalScrollPos;
        double canvasDeltaX = scrollRect.x() - lastCanvasX;
        double canvasDeltaY = scrollRect.y() - lastCanvasY;
        double deltaX = scrollDeltaX - canvasDeltaX;
        double deltaY = scrollDeltaY - canvasDeltaY;

        if (deltaX != 0 || deltaY != 0) {
            for (auto augmentedView : augmentedViews) {
                augmentedView->moveInsideRect(scrollRect, augmentedView->getX() - deltaX, augmentedView->getY() - deltaY);
            }
        }
    }

    lastHorizontalScrollPos = horizontalPos;
    lastVerticalScrollPos = verticalPos;
    lastCanvasX = scrollRect.x();
    lastCanvasY = scrollRect.y();
    hasScrollPos = true;
    lastScrollTime = QDateTime::currentMSecsSinceEpoch();
    augmentedViewsMutex.unlock();
}

// Capture a screenshot of the window and then return it
// *hasChanged* will be set to true if the screenshot is different from the previous call to this method
cv::Mat ObservedWindow::getScreenshot(bool* hasChanged) {
    screenshot capture = captureScreenshot(wid);

    if (capture.width && capture.height) {
        cv::Mat newScreen = cv::Mat(capture.height, capture.width, capture.bits_per_pixels > 24 ? CV_8UC4 : CV_8UC3, capture.pixels);

        if (hasChanged != NULL) {
            *hasChanged = true;
        }

        if (hasScreenshot) {
            if (hasChanged != NULL && currentScreenshot.height == capture.height && currentScreenshot.width == capture.width) {
                // Compare the previous screenshot with the new one in order to determine if there was a change
                cv::Mat lastScreen = cv::Mat(currentScreenshot.height, currentScreenshot.width, currentScreenshot.bits_per_pixels > 24 ? CV_8UC4 : CV_8UC3, currentScreenshot.pixels);
                double errorL2 = norm(lastScreen, newScreen, cv::NORM_L2);
                double similarity = errorL2 / (double) (lastScreen.rows * lastScreen.cols);
                *hasChanged = similarity > 0.001;
                if (hasMoved) {
                    hasMoved = false;
                    *hasChanged = true;
                }
            }

            this->clearScreenshotMemory();
        }

        currentScreenshot = capture;
        hasScreenshot = true;
        return newScreen;
    }

    return cv::Mat();
}

// Clear the screenshot memory used by calling getScreenshot
void ObservedWindow::clearScreenshotMemory() {
    if (hasScreenshot) {
        clearCapturedScreenshotMemory(currentScreenshot);
        hasScreenshot = false;
    }
}

// Test if the window is visible (on screen and not hidden by other windows)
bool ObservedWindow::isVisible() {
    lastVisible = visible;
    visible = isOnScreen() && !isWindowRectHidden(this->getWid(), this->x, this->y, this->width, this->height);
    return visible;
}

void ObservedWindow::hideAugmentedViews() {
    for (auto augmentedView : augmentedViews) {
        augmentedView->hide();
    }
}

// Test if the window was visible (before the last call to "isVisible")
bool ObservedWindow::wasVisible() {
    return lastVisible;
}

ObservedWindow::~ObservedWindow() {
    augmentedViewsMutex.lock();
    for (auto augmentedView : augmentedViews) {
        delete augmentedView;
    }
    augmentedViewsMutex.unlock();
}
