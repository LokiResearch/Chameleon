#-------------------------------------------------
#
# Project created by QtCreator 2018-07-18T10:28:16
#
#-------------------------------------------------

QT       += core gui webenginewidgets sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Chameleon
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        src/main.cpp \
        src/mainwindow.cpp \
    src/observedfile.cpp \
    src/observedfilesmanager.cpp \
    src/observedwindow.cpp \
    src/observedwindowsmanager.cpp \
    src/figure.cpp \
    src/augmentedview.cpp \
    src/figurefindertask.cpp \
    src/database.cpp \
    src/algorithms/featurematchingalgorithm.cpp \
    src/algorithms/surfalgorithm.cpp \
    src/registrationtooldialog.cpp

INCLUDEPATH += src/

HEADERS += \
        src/mainwindow.h \
    src/observedfile.h \
    src/observedfilesmanager.h \
    src/observedwindow.h \
    src/observedwindowsmanager.h \
    src/figure.h \
    src/augmentedview.h \
    src/figurefindertask.h \
    src/database.h \
    src/os_specific/window.h \
    src/algorithms/featurematchingalgorithm.h \
    src/algorithms/surfalgorithm.h \
    src/registrationtooldialog.h \
    src/model/observable.h \
    src/model/model.h


FORMS += \
        forms/mainwindow.ui \
    forms/registrationtooldialog.ui

# OpenCV
LIBS += -lopencv_core \
        -lopencv_flann \
        -lopencv_highgui \
        -lopencv_imgproc \
        -lopencv_calib3d \
        -lopencv_features2d \
        -lopencv_ml \
        -lopencv_xfeatures2d \
        -lopencv_tracking \
        -lopencv_imgcodecs


windows {
    message("Needs to be configured")
} else:mac {
    # = Code specific to macOs
    OBJECTIVE_SOURCES += src/os_specific/macos/window.mm \
                         src/os_specific/macos/accessibility.mm

    HEADERS += src/os_specific/macos/accessibility.h

    LIBS += -F/System/Library/PrivateFrameworks \
        -framework MultitouchSupport -v \
        -framework AppKit \
        -framework QuickLook \
        -framework Security

    # = OpenCV4 using pkg-config
    # Look for pkg-config in Fink, Macports and Homebrew (the last one we find wins)
    exists(/sw/bin/pkg-config) {
        QMAKE_PKG_CONFIG = /sw/bin/pkg-config
        INCLUDEPATH += /sw/include
    }
    exists(/opt/local/bin/pkg-config) {
            QMAKE_PKG_CONFIG = /opt/local/bin/pkg-CONFIG
        INCLUDEPATH += /opt/local/include
    }
    exists(/usr/local/bin/pkg-config) {
        QMAKE_PKG_CONFIG = /usr/local/bin/pkg-config
        INCLUDEPATH += /usr/local/include
    }

    message("Looking for dependencies using $$QMAKE_PKG_CONFIG")

    system($$QMAKE_PKG_CONFIG --exists opencv4) {
      QMAKE_CXXFLAGS += $$system("$$QMAKE_PKG_CONFIG --cflags opencv4")
      LIBS += $$system("$$QMAKE_PKG_CONFIG --libs opencv4")
      message("OpenCV4 found")
    }

} else:linux-*  {
}

RESOURCES += \
    resources/resources.qrc
