#include "MacPlatformIntegration.h"

#include <QApplication>
#include <QCoreApplication>
#include <QRect>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>

#include <functional>

#import <AppKit/AppKit.h>
#import <UserNotifications/UserNotifications.h>

namespace {
std::function<void(const QString&)> g_notificationActionHandler;

QString fromNSString(NSString* value) {
    if (!value)
        return QString();

    return QString::fromUtf8(value.UTF8String);
}
} // namespace

@interface GhostWireNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation GhostWireNotificationDelegate
- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler {
    Q_UNUSED(center);
    Q_UNUSED(notification);
    completionHandler(UNNotificationPresentationOptionBanner | UNNotificationPresentationOptionList);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
 didReceiveNotificationResponse:(UNNotificationResponse*)response
          withCompletionHandler:(void (^)(void))completionHandler {
    Q_UNUSED(center);
    NSString* urlString = response.notification.request.content.userInfo[@"actionUrl"];
    if (urlString.length > 0) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (g_notificationActionHandler) {
                g_notificationActionHandler(fromNSString(urlString));
            } else {
                NSURL* url = [NSURL URLWithString:urlString];
                if (url) {
                    [NSWorkspace.sharedWorkspace openURL:url];
                }
            }
            completionHandler();
        });
        return;
    }

    completionHandler();
}
@end

namespace {
NSString* toNSString(const QString& value) {
    return [NSString stringWithCharacters:reinterpret_cast<const unichar*>(value.utf16())
                                   length:static_cast<NSUInteger>(value.length())];
}

NSStatusBarButton* g_statusButton = nil;
GhostWireNotificationDelegate* g_notificationDelegate = nil;
NSString* const kTelegramBundleIdentifier = @"ru.keepcoder.Telegram";

NSView* nativeViewForWidget(QWidget* widget) {
    if (!widget)
        return nil;

    void* nativeView = reinterpret_cast<void*>(static_cast<quintptr>(widget->winId()));
    return (__bridge NSView*)nativeView;
}

NSRect toNativeRect(const QRect& rect) {
    NSScreen* screen = NSScreen.mainScreen;
    const CGFloat screenHeight = screen ? screen.frame.size.height : 0.0;
    return NSMakeRect(rect.x(),
                      screenHeight - rect.y() - rect.height(),
                      rect.width(),
                      rect.height());
}

void activateForModalAlert() {
    [NSApp activateIgnoringOtherApps:YES];
}

NSAlert* makeAlert(const QString& title,
                   const QString& message,
                   NSAlertStyle style) {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = toNSString(title);
    alert.informativeText = toNSString(message);
    alert.alertStyle = style;
    return alert;
}

QString alertDetailsText(const QString& message, const QString& informativeText) {
    if (message.isEmpty())
        return informativeText;
    if (informativeText.isEmpty())
        return message;
    return QStringLiteral("%1\n\n%2").arg(message, informativeText);
}

class PopoverHost final {
public:
    PopoverHost()
        : nativeWidget(new QWidget())
        , layout(new QVBoxLayout(nativeWidget))
        , controller([[NSViewController alloc] init])
        , popover([[NSPopover alloc] init])
    {
        nativeWidget->setAttribute(Qt::WA_NativeWindow, true);
        nativeWidget->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        nativeWidget->setLayout(layout);

        controller.view = nativeViewForWidget(nativeWidget);
        popover.contentViewController = controller;
        popover.behavior = NSPopoverBehaviorTransient;
        popover.animates = YES;
        closeObserver = [NSNotificationCenter.defaultCenter
            addObserverForName:NSPopoverDidCloseNotification
                        object:popover
                         queue:nil
                    usingBlock:^(NSNotification*) {
            if (content) {
                content->hide();
            }
        }];
    }

    ~PopoverHost() {
        if (closeObserver) {
            [NSNotificationCenter.defaultCenter removeObserver:closeObserver];
        }
        [popover close];
        if (content) {
            content->setParent(nullptr);
        }
        delete nativeWidget;
    }

    void setContent(QWidget* widget) {
        if (content == widget)
            return;

        if (content) {
            layout->removeWidget(content);
            content->setParent(nullptr);
        }

        content = widget;
        if (!content)
            return;

        content->hide();
        content->setParent(nativeWidget);
        content->setWindowFlags(Qt::Widget);
        layout->addWidget(content);
    }

    void show(QWidget* widget, const QRect& anchorRect) {
        setContent(widget);
        if (!content)
            return;

        content->adjustSize();
        const QSize contentSize = content->sizeHint().expandedTo(content->minimumSizeHint());
        nativeWidget->resize(contentSize);
        popover.contentSize = NSMakeSize(contentSize.width(), contentSize.height());
        content->show();

        if (g_statusButton) {
            [popover showRelativeToRect:g_statusButton.bounds
                                 ofView:g_statusButton
                          preferredEdge:NSRectEdgeMinY];
            return;
        }

        NSRect buttonRect = toNativeRect(anchorRect);
        NSWindow* keyWindow = NSApp.keyWindow ?: NSApp.mainWindow;
        NSView* positioningView = keyWindow.contentView;
        if (positioningView) {
            NSRect windowRect = [keyWindow convertRectFromScreen:buttonRect];
            NSRect viewRect = [positioningView convertRect:windowRect fromView:nil];
            [popover showRelativeToRect:viewRect
                                 ofView:positioningView
                          preferredEdge:NSRectEdgeMinY];
        } else {
            [popover showRelativeToRect:NSMakeRect(0, 0, 1, 1)
                                 ofView:nativeViewForWidget(nativeWidget)
                          preferredEdge:NSRectEdgeMinY];
        }
    }

    void close() {
        [popover close];
        if (content) {
            content->hide();
        }
    }

    void detachContent(QWidget* widget) {
        if (!content || content != widget)
            return;

        [popover close];
        layout->removeWidget(content);
        content->hide();
        content->setParent(nullptr);
        content = nullptr;
    }

    bool isVisible() const {
        return popover.shown;
    }

private:
    QWidget* nativeWidget = nullptr;
    QVBoxLayout* layout = nullptr;
    QWidget* content = nullptr;
    NSViewController* controller = nil;
    NSPopover* popover = nil;
    id closeObserver = nil;
};

PopoverHost* g_popoverHost = nullptr;

void cleanupPopoverHost() {
    delete g_popoverHost;
    g_popoverHost = nullptr;
}

PopoverHost* popoverHost() {
    if (!g_popoverHost) {
        g_popoverHost = new PopoverHost();
        qAddPostRoutine(cleanupPopoverHost);
    }
    return g_popoverHost;
}
} // namespace

namespace MacPlatformIntegration {

void configureApplication() {
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

void preparePopoverWindow(QWidget* widget) {
    if (!widget)
        return;

    NSView* view = nativeViewForWidget(widget);
    NSWindow* window = view.window;
    if (!window)
        return;

    window.opaque = NO;
    window.hasShadow = YES;
    window.backgroundColor = [NSColor clearColor];
    window.level = NSStatusWindowLevel;
    window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces
        | NSWindowCollectionBehaviorFullScreenAuxiliary
        | NSWindowCollectionBehaviorTransient;
}

void setStatusItemButton(void* button) {
    g_statusButton = (__bridge NSStatusBarButton*)button;
}

void showPopover(QWidget* content, const QRect& anchorRect) {
    popoverHost()->show(content, anchorRect);
}

void closePopover() {
    if (g_popoverHost) {
        g_popoverHost->close();
    }
}

void detachPopoverContent(QWidget* content) {
    if (g_popoverHost) {
        g_popoverHost->detachContent(content);
    }
}

bool isPopoverVisible() {
    return g_popoverHost && g_popoverHost->isVisible();
}

void showNotification(const QString& title, const QString& message, const QString& actionUrl) {
    NSString* notificationTitle = toNSString(title);
    NSString* notificationBody = toNSString(message);
    NSString* notificationActionUrl = toNSString(actionUrl);
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    if (!g_notificationDelegate) {
        g_notificationDelegate = [[GhostWireNotificationDelegate alloc] init];
        center.delegate = g_notificationDelegate;
    }

    [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert
                          completionHandler:^(BOOL granted, NSError*) {
        if (!granted)
            return;

        UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
        content.title = notificationTitle;
        content.body = notificationBody;
        if (notificationActionUrl.length > 0) {
            content.userInfo = @{@"actionUrl": notificationActionUrl};
        }

        NSString* identifier = [NSString stringWithFormat:@"com.momentics.GhostWireDesktop.%f",
                                NSDate.timeIntervalSinceReferenceDate];
        UNNotificationRequest* request =
            [UNNotificationRequest requestWithIdentifier:identifier
                                                 content:content
                                                 trigger:nil];
        [center addNotificationRequest:request withCompletionHandler:nil];
    }];
}

void setNotificationActionHandler(std::function<void(const QString&)> handler) {
    g_notificationActionHandler = std::move(handler);
}

bool showConfirmationAlert(const QString& title,
                           const QString& message,
                           const QString& informativeText,
                           const QString& confirmButton,
                           const QString& cancelButton) {
    activateForModalAlert();
    NSAlert* alert = makeAlert(
        title,
        alertDetailsText(message, informativeText),
        NSAlertStyleInformational);
    [alert addButtonWithTitle:toNSString(confirmButton)];
    [alert addButtonWithTitle:toNSString(cancelButton)];
    return [alert runModal] == NSAlertFirstButtonReturn;
}

void showInfoAlert(const QString& title, const QString& message, const QString& button) {
    activateForModalAlert();
    NSAlert* alert = makeAlert(title, message, NSAlertStyleInformational);
    [alert addButtonWithTitle:toNSString(button)];
    [alert runModal];
}

void showWarningAlert(const QString& title, const QString& message, const QString& button) {
    activateForModalAlert();
    NSAlert* alert = makeAlert(title, message, NSAlertStyleWarning);
    [alert addButtonWithTitle:toNSString(button)];
    [alert runModal];
}

bool isTelegramRunning() {
    NSArray<NSRunningApplication*>* telegramApps =
        [NSRunningApplication runningApplicationsWithBundleIdentifier:kTelegramBundleIdentifier];
    if (telegramApps.count > 0)
        return true;

    for (NSRunningApplication* app in NSWorkspace.sharedWorkspace.runningApplications) {
        NSString* appName = app.localizedName ?: app.bundleIdentifier ?: @"";
        if ([appName rangeOfString:@"Telegram" options:NSCaseInsensitiveSearch].location != NSNotFound)
            return true;
    }
    return false;
}

bool isUrlSchemeRegistered(const QString& url) {
    NSURL* nsUrl = [NSURL URLWithString:toNSString(url)];
    if (!nsUrl)
        return false;

    NSURL* appUrl = [NSWorkspace.sharedWorkspace URLForApplicationToOpenURL:nsUrl];
    return appUrl != nil;
}

} // namespace MacPlatformIntegration
