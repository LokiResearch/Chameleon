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
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

Database* database;
ObservedWindowsManager* windowManager;
ObservedFilesManager* filesManager;

void onFileOpened(const char* filePath, processId id) {
    QList<Figure*> figures = database->getFiguresOfFile(filePath);

    if (!figures.isEmpty()) {
        qDebug() << "New file found";
        filesManager->addObservedFile(filePath);
        emit windowManager->newFiguresDetected(id, figures);
    }
}

void onWindowDestroyed(windowId id) {
    windowManager->onWindowDestroyed(id);
}

void onWindowUpdated(windowId wid, processId pid, int x, int y, int width, int height, bool isOnScreen, const char* title, bool isFrontMost) {
    windowManager->onWindowUpdated(wid, pid, x, y, width, height, isOnScreen, title, isFrontMost);
}

void onWindowScrolled(windowId wid, int x, int y, int width, int height, double horizontalPos, double verticalPos) {
    windowManager->onWindowScrolled(wid, x, y, width, height, horizontalPos, verticalPos);
}

void onMouseMoved(int x, int y) {
    windowManager->dispatchMouseMovedEvent(x, y);
}

std::chrono::steady_clock::time_point startTime;

void customMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString logFileLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logFileLocation);

    FILE* output = fopen((logFileLocation+"/log.txt").toStdString().c_str(), "a+");
    int timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
    if (output != NULL) {
        //output = stderr;
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
        case QtDebugMsg:
            fprintf(output, "[%d] %s [%s:%u]\n", timestamp, localMsg.constData(), context.file, context.line);
            break;
        case QtInfoMsg:
            fprintf(output, "[%d] Info: %s (%s:%u, %s)\n", timestamp, localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            fprintf(output, "[%d] /!\\: %s\n", timestamp, localMsg.constData());
            break;
        case QtCriticalMsg:
            fprintf(output, "[%d] Critical: %s (%s:%u, %s)\n", timestamp, localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            fprintf(output, "[%d] Fatal: %s (%s:%u, %s)\n", timestamp, localMsg.constData(), context.file, context.line, context.function);
            abort();
        }
        fclose(output);
    }
}
int main(int argc, char *argv[])
{
    startTime = std::chrono::steady_clock::now();
    qInstallMessageHandler(customMessageOutput);
    QApplication a(argc, argv);
    a.setOrganizationDomain("Loki");
    a.setApplicationName("Chameleon");

    a.setQuitOnLastWindowClosed(false);

    bool screenCapture = requestScreenCapturePermission();
    bool accessibility = requestAccessibilityPermission();

    if (!screenCapture || !accessibility) {
        QMessageBox::warning(NULL, "Chameleon needs permissions", "Chameleon requires access to Accessibility and Screen Capture.\nPlease make sure Chameleon has access to both in System Preferences > Security & Privacy > Privacy.\n\nPressing 'OK' will exit the application.");
        return 1;
    }

    if (!installFileOpenHook()) {
        return 1;
    }

    initialize();
    qDebug() << "Load database";
    database = new Database();
    windowManager = new ObservedWindowsManager;
    filesManager = new ObservedFilesManager(database);
    MainWindow w(database, windowManager, filesManager);
    qDebug() << "Look for opened files";
    lookForOpenedFiles();
    setActivationEnabled(false);
    qDebug() << "Run the app";
    return a.exec();
}
