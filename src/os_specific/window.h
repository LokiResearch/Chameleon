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
#ifndef WINDOW_H
#define WINDOW_H

#include <vector>

typedef unsigned int windowId;
typedef unsigned long processId;

typedef struct _screenshot {
    unsigned char* pixels;
    void* _data;
    unsigned int width;
    unsigned int height;
    int bits_per_pixels;
} screenshot;

// Functions
void initialize();
bool installFileOpenHook();
bool requestScreenCapturePermission();
bool requestAccessibilityPermission();
void updateOpenedWindows();
screenshot captureScreenshot(windowId windowId);
void clearCapturedScreenshotMemory(screenshot scrnsht);
bool isWindowRectHidden(windowId wid,int x, int y, int width, int height);
std::vector<std::string> getActiveWindowFiles();
windowId getActiveWindow();
void lookForOpenedFiles();
void setActivationEnabled(bool activation);
bool registerScrollCallback(processId pid, windowId wid);
void freeRegisteredScrollCallbacks();
bool isWindowPartHidden(windowId wid, int x, int y, int width, int height);

// Callbacks
void onFileOpened(const char* filePath, processId id);
void onWindowUpdated(windowId wid, processId pid, int x, int y, int width, int height, bool isOnScreen, const char* title);
void onWindowScrolled(windowId wid, int x, int y, int width, int height, double horizontalPos, double verticalPos);
void onWindowDestroyed(windowId id);
void onMouseMoved(int x, int y);


#endif // WINDOW_H
