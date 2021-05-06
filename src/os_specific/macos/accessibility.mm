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
#include "../window.h"
#include <QDebug>
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include <iostream>
#include <set>
#include "accessibility.h"

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, CGWindowID* out);

typedef struct _scrollArea {
    AXUIElementRef element;
    double horizontalPos;
    double verticalPos;
    int x;
    int y;
} scrollArea;

QHash<windowId, QList<scrollArea*>> windowScrollAreas;
QMutex windowScrollAreasMutex;

// Return true if the role name of *element* is *role*
bool isElementKindOf(AXUIElementRef element, CFStringRef role) {
    CFStringRef elementRole;
    AXError err = AXUIElementCopyAttributeValue(element, kAXRoleAttribute, (CFTypeRef*) &elementRole);

    if (err) {
        return false;
    }

    return (elementRole != NULL && CFStringCompare(elementRole, role, 0) == 0);
}

CFTypeRef getAttributeValue(AXUIElementRef element, CFStringRef attribute) {
    CFTypeRef value = NULL;
    AXUIElementCopyAttributeValue(element, attribute, &value);

    return value;
}

void getElementBounds(AXUIElementRef element, CGSize* size, CGPoint* position) {
    // AXSize AXPosition

    AXValueRef _size =  (AXValueRef) getAttributeValue(element, kAXSizeAttribute);
    AXValueGetValue((AXValueRef) _size, kAXValueTypeCGSize, (void*) size);

    AXValueRef _position =  (AXValueRef) getAttributeValue(element, kAXPositionAttribute);
    AXValueGetValue((AXValueRef) _position, kAXValueTypeCGPoint, (void*) position);
}

double getScrollValue(AXUIElementRef scrollBar) {
    if (scrollBar != NULL) {
        CFNumberRef number = (CFNumberRef) getAttributeValue(scrollBar, kAXValueAttribute);
        NSNumber* nsNumber = (NSNumber*) number;

        return  (double) [nsNumber doubleValue];
    }
    return 0;
}

AXUIElementRef getAXUIElementFromWindow(processId pid, windowId wid) {
    AXUIElementRef appRef = AXUIElementCreateApplication(pid);
    AXUIElementRef window = NULL;

    CFArrayRef childrens;
    AXUIElementCopyAttributeValue(appRef, kAXChildrenAttribute, (CFTypeRef*) &childrens);
    if (childrens == NULL) return NULL;

    int nbChildrens = CFArrayGetCount(childrens);

    for (int i = 0; i < nbChildrens; ++i) {
        AXUIElementRef children = (AXUIElementRef) CFArrayGetValueAtIndex(childrens, i);
        if (isElementKindOf(children, kAXWindowRole)) {
            CGWindowID wndId;
            _AXUIElementGetWindow(children, &wndId);
            if (wid != wndId) continue;
            window = children;
            break;
        }
    }

    CFRelease(appRef);

    return window;
}

AXUIElementRef getFocusedWindowElement() {
    AXUIElementRef app = AXUIElementCreateSystemWide();

    AXUIElementRef frontApp = nil;
    AXUIElementCopyAttributeValue(app, kAXFocusedApplicationAttribute, (CFTypeRef *) &frontApp);

    AXUIElementRef frontWindow = nil;
    AXUIElementCopyAttributeValue(frontApp, kAXFocusedWindowAttribute, (CFTypeRef *) &frontWindow);

    CFRelease(app);
    if (frontApp != NULL) {
        CFRelease(frontApp);
    }

    return frontWindow;
}

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, CGWindowID* out);

windowId getActiveWindow() {
    // TODO : Do not use accessibility and private functions
    windowId id;
    AXUIElementRef frontWindow = getFocusedWindowElement();
    _AXUIElementGetWindow(frontWindow, &id);

    if (frontWindow == NULL) {
        return 0;
    }

    CFRelease(frontWindow);

    return id;
}

std::string getDocumentFromWindow(AXUIElementRef window) {
    NSString* document = nil;
    AXUIElementCopyAttributeValue(window, kAXDocumentAttribute, (CFTypeRef *) &document);

    if (document != nil) {
        NSURL *url = [[NSURL URLWithString:document] standardizedURL];
        const char* path = [[url path] UTF8String];
        return std::string(path);
    }

    return std::string();
}

std::vector<std::string> getActiveWindowFiles() {
    std::vector<std::string> windowFiles;

    std::string doc = getDocumentFromWindow(getFocusedWindowElement());

    if (!doc.empty()) {
        windowFiles.push_back(doc);
    }

    return windowFiles;
}

void getScrollAreas(AXUIElementRef element, QList<AXUIElementRef>* scrollAreas) {
    if (element == nil) return;


    if (isElementKindOf(element, kAXScrollAreaRole)) {
        scrollAreas->append(element);
        return;
    }

    CFArrayRef childrens;
    AXError error = AXUIElementCopyAttributeValue(element, kAXChildrenAttribute, (CFTypeRef*) &childrens);

    if (error != 0) {
        return;
    }

    int nbChildrens = CFArrayGetCount(childrens);

    if (nbChildrens == 0) {
        return;
    }

    for (int i = 0; i < nbChildrens; ++i) {
        AXUIElementRef children = (AXUIElementRef) CFArrayGetValueAtIndex(childrens, i);
        getScrollAreas(children, scrollAreas);
    }
}


bool getScrollbarPositions(AXUIElementRef scrollArea, double* horizontalPos, double* verticalPos) {
    AXUIElementRef verticalScrollbar = (AXUIElementRef) getAttributeValue(scrollArea, kAXVerticalScrollBarAttribute);
    AXUIElementRef horizontalScrollbar = (AXUIElementRef) getAttributeValue(scrollArea, kAXHorizontalScrollBarAttribute);

    // We want the absolute pixel position of the scroller, so we need to get the size of the content of the scroll area
    CFArrayRef childrens;
    AXError error = AXUIElementCopyAttributeValue(scrollArea, kAXChildrenAttribute, (CFTypeRef*) &childrens);
    if (error) return false;

    int nbChildrens = CFArrayGetCount(childrens);
    if (nbChildrens == 0) return false;

    int maxHeight = 0;
    int maxWidth = 0;
    for (int i = 0; i < nbChildrens; ++i) {
        AXUIElementRef children = (AXUIElementRef) CFArrayGetValueAtIndex(childrens, i);
        CGSize size;
        AXValueRef _size =  (AXValueRef) getAttributeValue(children, kAXSizeAttribute);
        AXValueGetValue((AXValueRef) _size, kAXValueTypeCGSize, (void*) &size);

        if (size.height > maxHeight) maxHeight = size.height;
        if (size.width > maxWidth) maxWidth = size.width;
    }

    CGSize scrollBarSize;
    CGPoint scrollBarPos;
    getElementBounds(verticalScrollbar, &scrollBarSize, &scrollBarPos);
    maxHeight -= scrollBarSize.height;

    getElementBounds(horizontalScrollbar, &scrollBarSize, &scrollBarPos);
    maxWidth -= scrollBarSize.width;

    *horizontalPos = getScrollValue(horizontalScrollbar) * maxWidth;
    *verticalPos = getScrollValue(verticalScrollbar) * maxHeight;

    return true;
}



bool registerScrollCallback(processId pid, windowId wid) {
    windowScrollAreasMutex.lock();
    if (!windowScrollAreas.contains(wid)) {
        AXUIElementRef windowElement = getAXUIElementFromWindow(pid, wid);
        if (windowElement == NULL) {
            windowScrollAreasMutex.unlock();
            return false;
        }

        QList<AXUIElementRef> scrollAreasElement;
        getScrollAreas(windowElement, &scrollAreasElement);

        QList<scrollArea*> scrollAreas;
        for (auto element : scrollAreasElement) {
            scrollArea* sArea = new scrollArea;
            sArea->element = element;
            getScrollbarPositions(element, &sArea->horizontalPos, &sArea->verticalPos);
            scrollAreas.append(sArea);
        }

        windowScrollAreas[wid] = scrollAreas;
    }

    windowScrollAreasMutex.unlock();

    return true;
}

#define FOR_EACH(VAR, LIST, CODE) \
int count = CFArrayGetCount(LIST); \
for (int i = 0; i < count; ++i) { \
    AXUIElementRef VAR = (AXUIElementRef) CFArrayGetValueAtIndex(LIST, i); \
    CODE \
} \


void lookForOpenedFiles(processId pid) {
    // AFAIK the only way to get the opened file from a window is through the accessibility API.
    AXUIElementRef application = AXUIElementCreateApplication(pid);
    CFArrayRef windows;
    AXUIElementCopyAttributeValue(application, kAXWindowsAttribute, (CFTypeRef*) &windows);

    if (windows != NULL) {
        FOR_EACH(window, windows, {
            std::string document = getDocumentFromWindow(window);
            if (!document.empty()) {
                onFileOpened(document.c_str(), pid);
            }
         });

        CFRelease(windows);
    }

    CFRelease(application);
}

static void* lookForOpenedFilesThread(void*) {
    // Some explanations here:
    // On macOS, a window can be "linked" to a file and this information can be accessed through the Accessibility API
    // While not all application support it, this has the advantage of not requiring root permissions and also allows us to scan for files opened *before* Chameleon was started.
    // So this method can work in addition to dtrace, or as an alternative for those not wanting to enable dtrace/give root permission.
    // Now, AFAIK, the Accessibility API is the only way to get the document linked to a window.
    // This part is implemented in lookForOpenedFiles(pid) which will use the Accessibility API to scan a process, find windows and find the documents.
    // However, I noticed that lookForOpenedFiles(pid) can be extremely expensive, especially on newer versions of MacOS in which accessibility requests can be quite slow
    // So my first naive implementation enumerating all processes (using GetNextProcess) and then calling lookForOpenedFiles(pid) on all of them was extremely slow.
    // Instead, I now reduce the number of calls to lookForOpenedFiles(pid) by first filtering processes to keep only those with at least one window.
    // I do so using CGWindowListCopyWindowInfo which returns the list of opened windows, and then get the process id that the window belongs to
    // Still a little costly, but I am not aware of any faster alternative

    NSArray *windows = (NSArray *)CGWindowListCopyWindowInfo(kCGWindowListOptionAll | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    // We store pids in a set to avoid duplicates
    std::set<pid_t> pids;

    for (NSDictionary *cocoaWindow in windows) {
        pid_t windowPid = [[cocoaWindow objectForKey:(id)kCGWindowOwnerPID] integerValue];
        pids.insert(windowPid);
    }

    for (auto pid : pids) {
        lookForOpenedFiles(pid);
    }

    CFRelease(windows);

    return NULL;
}

void lookForOpenedFiles() {
    pthread_t output_thread;
    pthread_create(&output_thread, NULL, lookForOpenedFilesThread, (void*) NULL);
}

void onDocumentScrolled() {
    windowScrollAreasMutex.lock();
    QHash<windowId, QList<scrollArea*>>::iterator i;

    for (i = windowScrollAreas.begin(); i != windowScrollAreas.end(); ++i) {
        for (auto sArea : i.value()) {
            double horizontalPos;
            double verticalPos;
            if (getScrollbarPositions(sArea->element, &horizontalPos, &verticalPos)) {
                CGSize size;
                CGPoint point;
                getElementBounds(sArea->element, &size, &point);

                if (horizontalPos != sArea->horizontalPos || verticalPos != sArea->verticalPos ||
                        sArea->x != point.x || sArea->y != point.y) {
                    sArea->horizontalPos = horizontalPos;
                    sArea->verticalPos = verticalPos;
                    sArea->x = point.x;
                    sArea->y = point.y;

                    onWindowScrolled(i.key(), point.x, point.y, size.width, size.height, horizontalPos, verticalPos);
                }
            }
        }
    }
    windowScrollAreasMutex.unlock();
}

bool requestAccessibilityPermission() {
    bool accessibilityEnabled = false;

    if (AXIsProcessTrustedWithOptions != NULL){
        NSDictionary *options = @{(__bridge id)kAXTrustedCheckOptionPrompt: @YES};
        accessibilityEnabled = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
    } else{
        accessibilityEnabled = AXIsProcessTrusted();
    }

   return accessibilityEnabled;
}

void accessibilityUpdateWindow(processId, windowId wid) {
    windowScrollAreasMutex.lock();
    // TODO : Update invalidated accessibility windows
    if (!windowScrollAreas.contains(wid)) {
        //registerScrollCallback(pid, wid);
    }
    windowScrollAreasMutex.unlock();
}

void freeScrollAreas(windowId wid) {
    if (windowScrollAreas.contains(wid)) {
        for (auto sArea : windowScrollAreas[wid]) {
            CFRelease(sArea->element);
            delete sArea;
        }
    }
}

void accessibilityDestroyWindow(windowId wid) {
    windowScrollAreasMutex.lock();
    freeScrollAreas(wid);
    windowScrollAreas.remove(wid);
    windowScrollAreasMutex.unlock();
}


void freeRegisteredScrollCallbacks() {
    windowScrollAreasMutex.lock();

    QMutableHashIterator<windowId, QList<scrollArea*>> i(windowScrollAreas);
    while (i.hasNext()) {
        i.next();
        freeScrollAreas(i.key());
        i.remove();
    }

    windowScrollAreasMutex.unlock();
}
