#include "MacTrayManager.h"
#include "MacPlatformIntegration.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QScreen>
#include <QSize>

#import <AppKit/AppKit.h>

@interface GhostWireStatusItemTarget : NSObject
@property(nonatomic, assign) MacTrayManager* owner;
- (instancetype)initWithOwner:(MacTrayManager*)owner;
- (void)statusItemClicked:(id)sender;
@end

@implementation GhostWireStatusItemTarget
- (instancetype)initWithOwner:(MacTrayManager*)owner {
    self = [super init];
    if (self) {
        _owner = owner;
    }
    return self;
}

- (void)statusItemClicked:(id)sender {
    Q_UNUSED(sender);
    if (_owner) {
        _owner->handleStatusItemActivated();
    }
}
@end

namespace {

NSString* toNSString(const QString& value) {
    return [NSString stringWithCharacters:reinterpret_cast<const unichar*>(value.utf16())
                                   length:static_cast<NSUInteger>(value.length())];
}

NSImage* statusImageForState(GhostWireProxyState state, bool hasConnections) {
    NSString* symbolName = @"bolt.horizontal.circle";
    if (state == GHOSTWIRE_PROXY_ONLINE) {
        symbolName = hasConnections ? @"bolt.horizontal.circle.fill" : @"bolt.horizontal.circle";
    } else if (state == GHOSTWIRE_PROXY_DEGRADED) {
        symbolName = @"exclamationmark.triangle";
    }

    NSImage* image = [NSImage imageWithSystemSymbolName:symbolName
                      accessibilityDescription:@"GhostWire"];
    if (!image) {
        image = [NSImage imageNamed:NSImageNameNetwork];
    }
    [image setTemplate:YES];
    image.size = NSMakeSize(18.0, 18.0);
    return image;
}

QString tooltipText() {
    const QString version = QCoreApplication::applicationVersion();
    return version.isEmpty()
        ? QStringLiteral("GhostWire Desktop")
        : QStringLiteral("GhostWire Desktop v%1").arg(version);
}

} // namespace

MacTrayManager::MacTrayManager(QObject* parent)
    : ITrayManager(parent)
{
}

MacTrayManager::~MacTrayManager() {
    cleanup();
}

void MacTrayManager::init() {
    if (m_statusItem)
        return;

    MacPlatformIntegration::configureApplication();

    NSStatusItem* statusItem = [[NSStatusBar systemStatusBar]
        statusItemWithLength:NSSquareStatusItemLength];
    GhostWireStatusItemTarget* target =
        [[GhostWireStatusItemTarget alloc] initWithOwner:this];

    NSStatusBarButton* button = statusItem.button;
    button.target = target;
    button.action = @selector(statusItemClicked:);
    button.toolTip = toNSString(tooltipText());
    button.imagePosition = NSImageOnly;
    MacPlatformIntegration::setStatusItemButton((__bridge void*)button);

    m_statusItem = (__bridge_retained void*)statusItem;
    m_target = (__bridge_retained void*)target;
    updateStatusImage();
}

void MacTrayManager::cleanup() {
    if (m_statusItem) {
        NSStatusItem* statusItem = (__bridge_transfer NSStatusItem*)m_statusItem;
        MacPlatformIntegration::setStatusItemButton(nullptr);
        [[NSStatusBar systemStatusBar] removeStatusItem:statusItem];
        m_statusItem = nullptr;
    }

    if (m_target) {
        GhostWireStatusItemTarget* target =
            (__bridge_transfer GhostWireStatusItemTarget*)m_target;
        target.owner = nullptr;
        m_target = nullptr;
    }
}

void MacTrayManager::setState(GhostWireProxyState state) {
    const bool changed = (m_state != state);
    m_state = state;
    if (m_state != GHOSTWIRE_PROXY_ONLINE) {
        m_hasConnections = false;
    }
    if (changed) {
        updateStatusImage();
    }
}

void MacTrayManager::setConnectionsState(bool hasConnections) {
    const bool nextHasConnections = (m_state == GHOSTWIRE_PROXY_ONLINE) && hasConnections;
    if (m_hasConnections == nextHasConnections)
        return;

    m_hasConnections = nextHasConnections;
    updateStatusImage();
}

void MacTrayManager::showMessage(const QString& title, const QString& message,
                                 QSystemTrayIcon::MessageIcon,
                                 int) {
    MacPlatformIntegration::showNotification(title, message);
}

QRect MacTrayManager::trayIconGeometry() const {
    if (!m_statusItem)
        return QRect();

    NSStatusItem* statusItem = (__bridge NSStatusItem*)m_statusItem;
    NSStatusBarButton* button = statusItem.button;
    if (!button || !button.window)
        return QRect();

    NSRect screenRect = [button.window convertRectToScreen:button.frame];
    QScreen* screen = QGuiApplication::screenAt(QPoint(
        static_cast<int>(screenRect.origin.x),
        static_cast<int>(screenRect.origin.y)));
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen)
        return QRect();

    const QRect screenGeometry = screen->geometry();
    const int x = static_cast<int>(screenRect.origin.x);
    const int y = screenGeometry.top() + screenGeometry.height()
        - static_cast<int>(screenRect.origin.y + screenRect.size.height);

    return QRect(QPoint(x, y),
                 QSize(static_cast<int>(screenRect.size.width),
                       static_cast<int>(screenRect.size.height)));
}

void MacTrayManager::handleStatusItemActivated() {
    if (MacPlatformIntegration::isPopoverVisible()) {
        MacPlatformIntegration::closePopover();
        return;
    }

    emit iconClicked(trayIconGeometry());
}

void MacTrayManager::updateStatusImage() {
    if (!m_statusItem)
        return;

    NSStatusItem* statusItem = (__bridge NSStatusItem*)m_statusItem;
    statusItem.button.image = statusImageForState(m_state, m_hasConnections);
}
