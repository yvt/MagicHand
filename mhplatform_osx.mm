#include "mhplatform.h"
#include <QuartzCore/QuartzCore.h>
#include <QScreen>
#include <algorithm>
#include <AppKit/AppKit.h>

namespace MHPlatform {

    static CGPoint lastPoint;
    static bool lButtonDown = false;
    static bool rButtonDown = false;

    QSizeF screenSize() {
        NSRect screenRect;
        NSArray *screenArray = [NSScreen screens];
        unsigned screenCount = [screenArray count];
        double width = 0, height = 0;

        for (unsigned index = 0; index < screenCount; index++) {
             NSScreen *screen = [screenArray objectAtIndex: index];
             screenRect = [screen visibleFrame];
             width = std::max(width, screenRect.size.width + screenRect.origin.x);
             height = std::max(height, screenRect.size.height + screenRect.origin.y);
        }

        return QSizeF(width, height);
    }

    void mouseMotion(QPointF pt) {
        lastPoint = CGPointMake(pt.x(), pt.y());
        CGEventRef e = CGEventCreateMouseEvent(nullptr,
                                    kCGEventMouseMoved,
                                    lastPoint,
                                    0);
        CGEventPost(kCGHIDEventTap, e);
        CFRelease(e);
        if (lButtonDown) {
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventLeftMouseDragged,
                                        lastPoint,
                                        0);
            CGEventPost(kCGHIDEventTap, e);
            CFRelease(e);
        }
        if (rButtonDown) {
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventRightMouseDragged,
                                        lastPoint,
                                        0);
            CGEventPost(kCGHIDEventTap, e);
            CFRelease(e);
        }
    }

    void mouseButtonDown(MouseButton button) {
        CGEventRef e;
        switch (button) {
        case MouseButton::Left:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventLeftMouseDown,
                                        lastPoint,
                                        kCGMouseButtonLeft);
            lButtonDown = true;
            break;
        case MouseButton::Right:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventRightMouseDown,
                                        lastPoint,
                                        kCGMouseButtonRight);
            rButtonDown = true;
            break;
        case MouseButton::Center:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventOtherMouseDown,
                                        lastPoint,
                                        kCGMouseButtonCenter);
            break;
        }
        CGEventPost(kCGHIDEventTap, e);
        CFRelease(e);
    }

    void mouseButtonUp(MouseButton button) {
        CGEventRef e;
        switch (button) {
        case MouseButton::Left:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventLeftMouseUp,
                                        lastPoint,
                                        kCGMouseButtonLeft);
            lButtonDown = false;
            break;
        case MouseButton::Right:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventRightMouseUp,
                                        lastPoint,
                                        kCGMouseButtonRight);
            rButtonDown = false;
            break;
        case MouseButton::Center:
            e = CGEventCreateMouseEvent(nullptr,
                                        kCGEventOtherMouseUp,
                                        lastPoint,
                                        kCGMouseButtonCenter);
            break;
        }
        CGEventPost(kCGHIDEventTap, e);
        CFRelease(e);
    }

}


