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
#ifndef AUGMENTEDVIEW_H
#define AUGMENTEDVIEW_H

#include <QMainWindow>
#include <QWidget>
#include <QWebEngineView>
#include <QPushButton>

class Figure;
class ObservedWindow;

class AugmentedView : public QMainWindow
{
    Q_OBJECT
public:
    AugmentedView(Figure* referenceFigure, ObservedWindow* window);
    inline Figure* getReferenceFigure() {return referenceFigure;}
    inline bool isFound() {return found;}
    void injectEvent(QEvent* evt) ;
    void moveInsideRect(QRect rect, double x, double y);
    void moveInsideWindow(double x, double y);

    inline double getX() {return virtualX;}
    inline double getY() {return virtualY;}

signals:
    void figureFound(QRect rect);
    void figureNotFound();

private slots:
    void onFigureFound(QRect rect);
    void onFigureNotFound();
    void onHideButtonPressed();
    void onFloatingWindowPressed();
    void onReferenceFigureDeleted(int id);

private:
    Figure* referenceFigure;
    ObservedWindow* window;
    QWidget* webEngineView;
    QWidget* webViewContainer;
    QAction* floatingWindowAction;
    QPushButton* btn;
    bool isHidden;
    bool found;
    bool isFloating;

    double virtualX;
    double virtualY;
    QRect virtualRect;
    int virtualWidth;
    int virtualHeight;
};

#endif // AUGMENTEDVIEW_H
