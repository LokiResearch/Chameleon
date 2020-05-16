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
#include "observedwindowsmanager.h"
#include "observedwindow.h"
#include "os_specific/window.h"
#include "figure.h"
#include "figurefindertask.h"
#include <QThreadPool>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

#include "model/model.h"

ObservedWindowsManager::ObservedWindowsManager() {
    this->connect(&refreshTimer, &QTimer::timeout, this, &ObservedWindowsManager::onRefreshTimer);

    Model::getInstance()->timeBetweenUpdates.addCallbackOnChange([=](int& val) {
        refreshTimer.start(val);
    });

    Model::getInstance()->useAccessibility.addCallbackOnChange([=](bool& val) {
        onAccessibilityStateChanged(val);
    });

    qRegisterMetaType<processId>("processId");
    qRegisterMetaType<QList<Figure*>>("QList<Figure*>");
    connect(this, SIGNAL(newFiguresDetected(processId,QList<Figure*>)), this, SLOT(onNewFiguresDetected(processId,QList<Figure*>)), Qt::QueuedConnection);
}

void ObservedWindowsManager::onRefreshTimer() {
    updateOpenedWindows();

    observedWindowsMutex.lock();
    QThreadPool* threadPool = QThreadPool::globalInstance();
    bool observedWindowVisible = false;
    for (auto observedWindow : observedWindows) {
        if (observedWindow->getAugmentedViews().size() > 0) {
            if (observedWindow->isVisible() || strlen(observedWindow->getTitle()) > 0) {
                observedWindowVisible = true;
                FigureFinderTask* finderTask = new FigureFinderTask(observedWindow);
                threadPool->start(finderTask);
            } else {
                for (auto views : observedWindow->getAugmentedViews()) {
                    emit views->figureNotFound();
                }
            }
        }
    }
    observedWindowsMutex.unlock();
}

void ObservedWindowsManager::onAccessibilityStateChanged(bool newState) {
    if (newState) {
        observedWindowsMutex.lock();
        for (auto observedWindow : observedWindows) {
            if (observedWindow->getAugmentedViews().size() > 0) {
                registerScrollCallback(observedWindow->getPid(), observedWindow->getWid());
            }
        }
        observedWindowsMutex.unlock();
    } else {
        freeRegisteredScrollCallbacks();
    }
}

void ObservedWindowsManager::addFigureToWindow(ObservedWindow* wnd, Figure* figure) {
    wnd->addFigure(figure);

    if (Model::getInstance()->useAccessibility.getValue()) {
        registerScrollCallback(wnd->getPid(), wnd->getWid());
    }
}

void ObservedWindowsManager::onWindowUpdated(windowId wid, processId pid, int x, int y, int width, int height, bool isOnScreen, const char* title) {
    observedWindowsMutex.lock();

    QMutableListIterator<ObservedWindow*> i(observedWindowsTrashCan);
    while (i.hasNext()) {
        ObservedWindow* wnd = i.next();
        if (wnd->getAnalysisMutex().tryLock()) {
            i.remove();
            delete wnd;
        }
    }

    ObservedWindow* wnd = NULL;
    for (auto observedWindow : observedWindows) {
        if (observedWindow->getWid() == wid) {
            // The window was already in the list, so we just update its parameters
            wnd = observedWindow;
            break;
        }
     }

    if (wnd == NULL) {
        // New window
        wnd = new ObservedWindow(pid, wid);
        figuresByProcessMutex.lock();
        for (auto figure : figuresByProcess[pid]) {
            addFigureToWindow(wnd, figure);
        }
        figuresByProcessMutex.unlock();
        observedWindows.append(wnd);
    }

    wnd->setX(x);
    wnd->setY(y);
    wnd->setWidth(width);
    wnd->setHeight(height);
    wnd->setOnScreen(isOnScreen);
    wnd->setTitle(title);

    observedWindowsMutex.unlock();
}

void ObservedWindowsManager::onWindowScrolled(windowId wid, int x, int y, int width, int height, double horizontalPos, double verticalPos) {
    observedWindowsMutex.lock();

    for (auto observedWindow : observedWindows) {
        if (observedWindow->getWid() == wid) {
            observedWindow->onWindowScrolled(QRect(x, y, width, height), horizontalPos, verticalPos);
        }
     }

    observedWindowsMutex.unlock();
}

void ObservedWindowsManager::onWindowDestroyed(windowId wid) {
    observedWindowsMutex.lock();
    QMutableListIterator<ObservedWindow*> i(observedWindows);
    while (i.hasNext()) {
        ObservedWindow* wnd = i.next();
        if (wnd->getWid() == wid) {
            i.remove();
            wnd->hideAugmentedViews();
            observedWindowsTrashCan.append(wnd);
        }
    }
    observedWindowsMutex.unlock();
}

// Add a new figure to look for
// We should only look for the figure in the windows of the specified process id
void ObservedWindowsManager::addFigure(processId pid, Figure* figure) {
    observedWindowsMutex.lock();
    for (auto window : observedWindows) {
        if (window->getPid() == pid) {
            addFigureToWindow(window, figure);
        }
    }
    observedWindowsMutex.unlock();
}

void ObservedWindowsManager::dispatchMouseMovedEvent(int x, int y) {
    observedWindowsMutex.lock();
    for (auto window : observedWindows) {
        for (auto view : window->getAugmentedViews()) {
            if (view->isVisible()) {
                QPoint globalPos = QPoint(x, y);
                QPoint localPos = view->mapFromGlobal(globalPos);
                //if (view->rect().contains(localPos)) {

                    QMouseEvent* evt = new QMouseEvent(QEvent::MouseMove, localPos, globalPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                    view->injectEvent(evt);
                //}
            }
        }
    }
    observedWindowsMutex.unlock();
}

void ObservedWindowsManager::onFigureDeleted(int figureId) {
    figuresByProcessMutex.lock();
    QHash<processId, QList<Figure*>>::iterator i;
    for (i = figuresByProcess.begin(); i != figuresByProcess.end(); ++i) {

        QMutableListIterator<Figure*> figureIt(i.value());
        while (figureIt.hasNext()) {
            if (figureIt.next()->getId() == figureId) {
                figureIt.remove();
            }
        }
    }
    figuresByProcessMutex.unlock();
}

void ObservedWindowsManager::onNewFiguresDetected(processId pid, QList<Figure*> figures) {
    if (figures.isEmpty()) {
        return;
    }

    figuresByProcessMutex.lock();
    QList<Figure*>* processFigures = &figuresByProcess[pid];

    for (auto figure : figures) {
        if (!processFigures->contains(figure)) {
            processFigures->append(figure);
            connect(figure, SIGNAL(deleted(int)), this, SLOT(onFigureDeleted(int)), static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::DirectConnection));
        }
        addFigure(pid, figure);
    }
    figuresByProcessMutex.unlock();
}
