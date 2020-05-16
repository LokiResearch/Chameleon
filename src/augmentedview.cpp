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
#include "augmentedview.h"
#include <QMainWindow>
#include <QWebEngineView>
#include <QPushButton>
#include <QMenu>
#include <QThread>
#include "observedwindow.h"
#include "figure.h"
#include "model/model.h"
#include <QApplication>


AugmentedView::AugmentedView(Figure* referenceFigure, ObservedWindow* window) :
    QMainWindow(NULL), referenceFigure(referenceFigure), window(window) {

    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    this->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);

    QWebEngineView* webView = new QWebEngineView();

    webView->setUrl(referenceFigure->getUrl());
    webEngineView = webView;

    webViewContainer = new QWidget(this);
    webEngineView->setParent(webViewContainer);

    btn = new QPushButton("i");
    btn->setFixedSize(25, 25);
    btn->setStyleSheet("QPushButton{background-color: rgba(0, 0, 255, 50%);}");
    btn->setParent(this);

    QMenu *menu = new QMenu(this);
    floatingWindowAction = menu->addAction("Floating window", this, SLOT(onFloatingWindowPressed()));
    menu->addSeparator();
    menu->addAction("Hide", this, SLOT(onHideButtonPressed()));
    btn->setMenu(menu);
    isHidden = false;
    isFloating = false;

    connect(this, SIGNAL(figureFound(QRect)), this, SLOT(onFigureFound(QRect)), Qt::QueuedConnection);
    connect(this, SIGNAL(figureNotFound()), this, SLOT(onFigureNotFound()), Qt::QueuedConnection);
    connect(referenceFigure, SIGNAL(deleted(int)), this, SLOT(onReferenceFigureDeleted(int)), Qt::DirectConnection);

    Model::getInstance()->showInfoButton.addCallbackOnChange([&](bool& visible) {
        btn->setVisible(visible);
    });
}

void AugmentedView::moveInsideRect(QRect rect, double x, double y) {
    this->virtualX = x;
    this->virtualY = y;
    this->virtualRect = rect;

    if (isFloating) {
        return;
    }

    int roundedX = (int) qRound(x);
    int roundedY = (int) qRound(y);

    int maxX = rect.x() + rect.width() - virtualWidth;
    int maxY = rect.y() + rect.height() - virtualHeight;

    int newX = qMin(qMax(roundedX, rect.x()), maxX);
    int newY = qMin(qMax(roundedY, rect.y()), maxY);

    int viewx = roundedX - newX;
    int viewy = roundedY - newY;
    webEngineView->move(viewx, viewy);
    btn->move(viewx, viewy);

    // Augmented figure is "disabled" when not fully displayed. Prevent scroll event from being captured by empty areas of the figure
    setWindowFlag(Qt::WindowTransparentForInput, (viewx != 0 || viewy != 0));

    bool isVisible = !isWindowPartHidden(window->getWid(), newX, newY, this->width(), this->height()); // Hide if windows overlap the figure
    isVisible &= !(qAbs(roundedX - newX) >= webEngineView->width() || qAbs(roundedY - newY) >= webEngineView->height()); // Hide if outside of the window bounds
    this->setVisible(isVisible && found);

    QMainWindow::move(newX, newY);

    repaint();
}

void AugmentedView::moveInsideWindow(double x, double y) {
    moveInsideRect(QRect(window->getX(), window->getY(), window->getWidth(), window->getHeight()), x, y);
}

void AugmentedView::onHideButtonPressed() {
    isHidden = !isHidden;
    webEngineView->setVisible(!isHidden);
    btn->menu()->actions().last()->setText(isHidden ? "Show" : "Hide");
}

void AugmentedView::onFloatingWindowPressed() {

    setAttribute(Qt::WA_NoSystemBackground, isFloating);
    setAttribute(Qt::WA_TranslucentBackground, isFloating);
    setAttribute(Qt::WA_ShowWithoutActivating, isFloating);
    setAttribute(Qt::WA_AlwaysStackOnTop, true);
    setAttribute(Qt::WA_MacShowFocusRect, !isFloating);

    isFloating = !isFloating;
    if (!isFloating) {
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowDoesNotAcceptFocus | Qt::NoDropShadowWindowHint);
        moveInsideRect(this->virtualRect, this->virtualX, this->virtualY);
    } else {
        this->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    }

    floatingWindowAction->setText(isFloating ? "Single window" : "Floating window");
    this->setVisible(true);
    repaint();
}

void AugmentedView::onFigureFound(QRect rect) {
    moveInsideWindow(rect.x(), rect.y());

    webViewContainer->setFixedSize(rect.width(), rect.height());
    webEngineView->setFixedSize(rect.width(), rect.height());
    this->setFixedSize(rect.width(), rect.height());
    virtualWidth = rect.width();
    virtualHeight = rect.height();

    if (!this->found) {
        showNormal();
        this->found = true;
    }
}

void AugmentedView::onReferenceFigureDeleted(int id) {
    window->getAnalysisMutex().lock(); // Make sure that it is not deleted during an analysis of the window
    window->getAugmentedViewsMutex().lock();

    QMutableListIterator<AugmentedView*> i(window->getAugmentedViews());
    while (i.hasNext()) {
        AugmentedView* augmentedView = i.next();

        if (augmentedView->window == window && augmentedView->referenceFigure == referenceFigure) {
            i.remove();
        }
    }
    delete this;

    window->getAugmentedViewsMutex().unlock();
    window->getAnalysisMutex().unlock();
}

void AugmentedView::injectEvent(QEvent* evt) {
    if (webEngineView->focusProxy() != NULL) {
        QApplication::postEvent(webEngineView->focusProxy(), evt);
    } else {
        QApplication::postEvent(webEngineView, evt);
    }
}

void AugmentedView::onFigureNotFound() {
    if (this->found) {
        if (!isFloating) {
            hide();
        }
        this->found = false;
    }
}
