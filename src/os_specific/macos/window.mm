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
#include <opencv2/opencv.hpp>
#include "../window.h"
#include <QDebug>
#include <iostream>
#include <pthread.h>
#include "accessibility.h"
#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>


#define DTRACE_PRGRM "\n\
#pragma D option quiet\n\
#pragma D option switchrate=10hz\n\
\n\
\n\
syscall::open:entry, syscall::open_nocancel:entry, syscall::open_extended:entry {\n\
    self->pathp = arg0;\n\
}\n\
\n\
syscall::open:return, syscall::open_nocancel:return, syscall::open_extended:return {\n\
    printf(\"%d\\t%d\\t%s\\n\", pid, (int)arg0, copyinstr(self->pathp));\n\
    self->pathp = 0;\n\
}\n\
\n\
syscall::open:return, syscall::open_nocancel:return, syscall::open_extended:return {\n\
    self->pathp = 0;\n\
}"

CFMachPortRef eventTap;
CGEventRef eventsCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon);

static void* opensnoop_thread(void* args) {
    FILE* pipe = (FILE*) args;

    if (pipe) {
        int fd = fileno(pipe);
        fcntl(fd, F_SETFL, O_NONBLOCK);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        while (true) {
            QString str;
            select(fd + 1, &fds, NULL, NULL, NULL);
            char buf[1024];
            size_t nread;
            do {
                nread = fread(buf, 1, sizeof(buf) - 1, pipe);
                if (nread > 0) {
                    buf[nread] = 0;
                    str += QString::fromUtf8(buf);
                }
            } while (nread == sizeof(buf) - 1);

            QStringList lines = str.split("\n", QString::SkipEmptyParts);
            for (auto line : lines) {
                QStringList cols = line.split("\t", QString::SkipEmptyParts);
                if (cols.size() == 3 && cols.at(1) != "-1") {
                    int pid = cols.at(0).toInt();
                    if (pid != getpid() && !cols.at(2).startsWith("/dev/")) {
                        onFileOpened(cols.at(2).toLocal8Bit().constData(), pid);
                    }
                }
            }
        }
    }
    return NULL;
}

bool requestScreenCapturePermission() {
    bool screenRecordingEnabled = false;
    // Since Catalina, we also need the Screen Capture permissions to loop through open windows (and capture them)
    if (CGRequestScreenCaptureAccess != NULL) {
        CGRequestScreenCaptureAccess();
        screenRecordingEnabled = CGPreflightScreenCaptureAccess();
    } else {
        // Backward compatibility
        // If the API is unavailable, we make sure we can take screenshots (should force the prompt for granting permissions on newer systems)
        CGImageRef screenshot = CGWindowListCreateImage(
            CGRectMake(0, 0, 1, 1),
            kCGWindowListOptionOnScreenOnly,
            kCGNullWindowID,
            kCGWindowImageDefault);

        if (screenshot != NULL) {
            screenRecordingEnabled = true;
            CFRelease(screenshot);
        }
    }
    return screenRecordingEnabled;
}

void initialize() {
    eventTap = CGEventTapCreate(kCGAnnotatedSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, kCGEventMaskForAllEvents, eventsCallback, 0);
    Q_ASSERT_X(eventTap != NULL, "Event Tap", "Could not create the event tap");
    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
}

bool installFileOpenHook() {
    AuthorizationRef authref = 0;
    FILE *pipe = 0;

    if (AuthorizationCreate(0, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &authref) != errAuthorizationSuccess) {
        return false;
    }

    char opt[] = "-n";
    char dtrace_prgrm[] = DTRACE_PRGRM;
    char* const args[] = {opt, dtrace_prgrm, NULL};

    if (AuthorizationExecuteWithPrivileges(authref, "/usr/sbin/dtrace", kAuthorizationFlagDefaults, args, &pipe) != errAuthorizationSuccess) {
            AuthorizationFree(authref, kAuthorizationFlagDestroyRights);
            return false;
    }

    pthread_t output_thread;
    pthread_create(&output_thread, NULL, opensnoop_thread, (void*) pipe);

    return true;
}

QSet<windowId> openedWindows;
void updateOpenedWindows() {
    NSArray *windows = (NSArray *)CGWindowListCopyWindowInfo(kCGWindowListOptionAll | kCGWindowListExcludeDesktopElements, kCGNullWindowID);

    QSet<windowId> closedWindows(openedWindows);
    openedWindows.clear();

    for (NSDictionary *cocoaWindow in windows) {
        CGRect windowBounds;

        windowId windowNumber = [[cocoaWindow objectForKey:(id)kCGWindowNumber] integerValue];
        processId windowPid = [[cocoaWindow objectForKey:(id)kCGWindowOwnerPID] integerValue];
        bool isOnScreen = [[cocoaWindow objectForKey:(id)kCGWindowIsOnscreen] boolValue];

        const char* windowTitle = [(__bridge NSString *)[cocoaWindow objectForKey:(id)kCGWindowName] UTF8String];
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef) [cocoaWindow objectForKey:(id)kCGWindowBounds], &windowBounds);

        onWindowUpdated(windowNumber, windowPid, windowBounds.origin.x, windowBounds.origin.y, windowBounds.size.width, windowBounds.size.height, isOnScreen, windowTitle);
        accessibilityUpdateWindow(windowNumber, windowPid);

        openedWindows.insert(windowNumber);
        closedWindows.remove(windowNumber);
    }

    CFRelease(windows);

    for (auto windowNumber : closedWindows) {
        onWindowDestroyed(windowNumber);
        accessibilityDestroyWindow(windowNumber);
    }
}

cv::Rect cg2cv(CGRect rect) {
    return cv::Rect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

bool isWindowRectHidden(windowId wid,int x, int y, int width, int height) {
    NSArray *windows = (NSArray *)CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenAboveWindow, wid);

    bool res = false;
    for (NSDictionary *window in windows) {
        NSInteger windowLayer = [[window objectForKey:(id)kCGWindowLayer] integerValue];
        if (windowLayer < kCGScreenSaverWindowLevelKey) {
            CGRect wndBounds;
            CGRectMakeWithDictionaryRepresentation((CFDictionaryRef) [window objectForKey:(id)kCGWindowBounds], &wndBounds);

            cv::Rect wndRect = cg2cv(wndBounds);
            cv::Rect rect(x, y, width, height);

            if ((rect & wndRect).area() > 0) {
                res = true;
                break;
            }
        }
    }

    CFRelease(windows);
    return res;
}


screenshot captureScreenshot(windowId windowId) {
    screenshot screenData;
    CGImageRef imageRef = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, windowId, kCGWindowImageBoundsIgnoreFraming | kCGWindowImageNominalResolution);

    screenData.width = CGImageGetWidth(imageRef);
    screenData.height = CGImageGetHeight(imageRef);
    screenData.bits_per_pixels = CGImageGetBitsPerPixel(imageRef);

    CGColorSpaceRef colorSpace = CGImageGetColorSpace(imageRef);
    CGFloat cols = CGImageGetWidth(imageRef);
    CGFloat rows = CGImageGetHeight(imageRef);

    // (TODO : remove the use of opencv)
    cv::Mat* screenshotMat = new cv::Mat(rows, cols, CV_8UC4);

    CGContextRef contextRef = CGBitmapContextCreate(screenshotMat->data, cols, rows, 8, screenshotMat->step[0],
            colorSpace, kCGImageAlphaNoneSkipLast | kCGBitmapByteOrderDefault);

    screenData.pixels = (unsigned char*) screenshotMat->data;
    screenData._data = (void*) screenshotMat;

    CGContextDrawImage(contextRef, CGRectMake(0, 0, cols, rows), imageRef);
    CGContextRelease(contextRef);
    CGImageRelease(imageRef);

    return screenData;
}

void clearCapturedScreenshotMemory(screenshot scrnsht) {
    ((cv::Mat*) (scrnsht._data))->release();
}

void setActivationEnabled(bool activation) {
    if (activation) {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];
    } else {
        [NSApp setActivationPolicy: NSApplicationActivationPolicyProhibited];
    }
}

bool isWindowPartHidden(windowId wid,int x, int y, int width, int height) {
    NSArray *windows = (NSArray *)CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenAboveWindow, wid);

    bool res = false;
    for (NSDictionary *window in windows) {
        NSInteger windowLayer = [[window objectForKey:(id)kCGWindowLayer] integerValue];
        if (windowLayer < kCGScreenSaverWindowLevelKey) {
            CGRect wndBounds;
            CGRectMakeWithDictionaryRepresentation((CFDictionaryRef) [window objectForKey:(id)kCGWindowBounds], &wndBounds);

            cv::Rect wndRect = cg2cv(wndBounds);
            cv::Rect rect(x, y, width, height);

            if ((rect & wndRect).area() > 0) {
                res = true;
                break;
            }
        }
    }

    CFRelease(windows);
    return res;
}


// Callback receiving all the system's events
CGEventRef eventsCallback(__unused CGEventTapProxy proxy,
                             CGEventType type,
                             CGEventRef event,
                             __unused void *refcon)
{
    if (type == (CGEventType) -2 || type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        // The event tap has been disabled by the system, we have to reactivate it
        NSLog(@"event tap has been disabled %i", type);
        CGEventTapEnable(eventTap, true);
        return event;
    }

    // To avoid warnings
    NSEventType nsType = (NSEventType) type;

    NSEvent *nsEvent = [NSEvent eventWithCGEvent:event];


    if (nsType == NSMouseMoved) {
        CGPoint pt = CGEventGetLocation(event);
        onMouseMoved((int) pt.x, (int) pt.y);
    }

    if (nsType == NSEventTypeScrollWheel || nsType == NSEventTypeMouseEntered) {
        onDocumentScrolled();
    }

    // To handle scroll by clicking on the scrollbar. We do not want it to be called if we are interacting with one of our own window
    if (nsType == NSEventTypeLeftMouseDragged && ![nsEvent window]) {
        onDocumentScrolled();
    }

    // send event to the next application
    return event;
}
