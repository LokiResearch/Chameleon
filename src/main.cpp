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
#include "mainwindow.h"
#include "database.h"
#include "observedwindowsmanager.h"
#include "observedfilesmanager.h"
#include <QApplication>
#include <QtGlobal>
#include "os_specific/window.h"
#include <QDebug>

Database* database;
ObservedWindowsManager* windowManager;
ObservedFilesManager* filesManager;

void onFileOpened(const char* filePath, processId id) {
    QList<Figure*> figures = database->getFiguresOfFile(filePath);

    if (!figures.isEmpty()) {
        filesManager->addObservedFile(filePath);
        emit windowManager->newFiguresDetected(id, figures);
    }
}

void onWindowDestroyed(windowId id) {
    windowManager->onWindowDestroyed(id);
}

void onWindowUpdated(windowId wid, processId pid, int x, int y, int width, int height, bool isOnScreen, const char* title) {
    windowManager->onWindowUpdated(wid, pid, x, y, width, height, isOnScreen, title);
}

void onWindowScrolled(windowId wid, int x, int y, int width, int height, double horizontalPos, double verticalPos) {
    windowManager->onWindowScrolled(wid, x, y, width, height, horizontalPos, verticalPos);
}

void onMouseMoved(int x, int y) {
    windowManager->dispatchMouseMovedEvent(x, y);
}

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "%s [%s:%u]\n", localMsg.constData(), context.file, context.line);
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "/!\\: %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageOutput);
    QApplication a(argc, argv);
    a.setOrganizationDomain("Loki");
    a.setApplicationName("Chameleon");

    a.setQuitOnLastWindowClosed(false);

    if (!installFileOpenHook()) {
        return 1;
    }

    initialize();

    database = new Database();
    windowManager = new ObservedWindowsManager;
    filesManager = new ObservedFilesManager(database);
    MainWindow w(database, windowManager, filesManager);

    lookForOpenedFiles();

    setActivationEnabled(false);

    return a.exec();
}
