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
#ifndef OBSERVEDWINDOW_H
#define OBSERVEDWINDOW_H

#include "os_specific/window.h"
#include <opencv2/opencv.hpp>
#include <QList>
#include <QMutex>
#include "augmentedview.h"

class ObservedWindow : public QObject
{
    Q_OBJECT
public:
    ObservedWindow(processId pid, windowId wid);
    ~ObservedWindow();

    void addFigure(Figure* figure);
    bool isVisible();
    bool wasVisible();
    cv::Mat getScreenshot(bool* hasChanged = NULL);
    void clearScreenshotMemory();
    void hideAugmentedViews();
    void onWindowScrolled(QRect scrollRect, double horizontalPos, double verticalPos);


    inline processId getPid() {return pid;}
    inline windowId getWid() {return wid;}
    inline int getX() {return x;}
    inline int getY() {return y;}
    inline int getWidth() {return width;}
    inline int getHeight() {return height;}
    inline QList<AugmentedView*>& getAugmentedViews() {return augmentedViews;}
    inline QMutex& getAnalysisMutex() {return analysis;}
    inline QMutex& getAugmentedViewsMutex() {return augmentedViewsMutex;}
    inline bool isOnScreen() {return onScreen;}
    inline bool isFrontMost() {return frontMost;}
    inline const char* getTitle() {return title;}
    inline qint64 getMSecsSinceScroll() {return QDateTime::currentMSecsSinceEpoch() - lastScrollTime;}
    inline double getHScrollPos() {return lastHorizontalScrollPos;}
    inline double getVScrollPos() {return lastVerticalScrollPos;}
    inline QRect getScrollRect() {return lastScrollRect;}

    inline void setX(int newX) {if (x != newX) hasMoved = true; x = newX;}
    inline void setY(int newY) {if (y != newY) hasMoved = true; y = newY;}
    inline void setWidth(int newWidth) {width = newWidth;}
    inline void setHeight(int newHeight) {height = newHeight;}
    inline void setOnScreen(bool onScreen) {this->onScreen = onScreen;}
    inline void setFrontMost(bool frontMost) {this->frontMost = frontMost;}
    inline void setTitle(const char* title) {if (title != NULL) strncpy(this->title, title, sizeof(this->title) - 1);}

private:
    screenshot currentScreenshot;
    bool hasScreenshot;
    bool lastVisible;
    bool visible;
    bool frontMost;
    double hasScrollPos;
    double lastVerticalScrollPos;
    double lastHorizontalScrollPos;
    QRect lastScrollRect;
    qint64 lastScrollTime;

    QList<AugmentedView*> augmentedViews;

    processId pid;
    windowId wid;
    int x;
    int y;
    int width;
    int height;
    bool onScreen;
    bool hasMoved;
    char title[256];

    QMutex augmentedViewsMutex;
    QMutex analysis;
};

#endif // OBSERVEDWINDOW_H
