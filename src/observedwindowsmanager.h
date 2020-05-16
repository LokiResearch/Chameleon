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
#ifndef OBSERVEDWINDOWSMANAGER_H
#define OBSERVEDWINDOWSMANAGER_H

#include <QList>
#include <QMutex>
#include <QTimer>
#include <QHash>
#include "os_specific/window.h"

class Figure;
class ObservedWindow;

class ObservedWindowsManager : public QObject
{
    Q_OBJECT

public:
    ObservedWindowsManager();
    void onWindowOpened(windowId wid, processId pid);
    void onWindowUpdated(windowId wid, processId pid, int x, int y, int width, int height, bool isOnScreen, const char* title);
    void onWindowScrolled(windowId wid, int x, int y, int width, int height, double horizontalPos, double verticalPos);
    void onWindowDestroyed(windowId wid);
    void addFigure(processId pid, Figure* figure);
    inline QList<ObservedWindow*> getWindows() {return observedWindows;}
    void dispatchMouseMovedEvent(int x, int y);

signals:
    void newFiguresDetected(processId pid, QList<Figure*> figures);

private slots:
    void onRefreshTimer();
    void onNewFiguresDetected(processId pid, QList<Figure*> figures);
    void onFigureDeleted(int id);

private:
    void addFigureToWindow(ObservedWindow* wnd, Figure* figure);
    void onAccessibilityStateChanged(bool newState);

    QTimer refreshTimer;
    QList<ObservedWindow*> observedWindows;
    QList<ObservedWindow*> observedWindowsTrashCan;
    QMutex observedWindowsMutex;
    QMutex figuresByProcessMutex;
    QHash<processId, QList<Figure*>> figuresByProcess;
};

#endif // OBSERVEDWINDOWSMANAGER_H
