#pragma once

#include <functional>
#include <QString>

class QWidget;
class QRect;

namespace MacPlatformIntegration {

void configureApplication();
void preparePopoverWindow(QWidget* widget);
void setStatusItemButton(void* button);
void showPopover(QWidget* content, const QRect& anchorRect);
void closePopover();
void detachPopoverContent(QWidget* content);
bool isPopoverVisible();
void showNotification(const QString& title,
                      const QString& message,
                      const QString& actionUrl = QString());
void setNotificationActionHandler(std::function<void(const QString&)> handler);
bool showConfirmationAlert(const QString& title,
                           const QString& message,
                           const QString& informativeText,
                           const QString& confirmButton,
                           const QString& cancelButton);
void showInfoAlert(const QString& title,
                   const QString& message,
                   const QString& button);
void showWarningAlert(const QString& title,
                      const QString& message,
                      const QString& button);
bool isTelegramRunning();
bool isUrlSchemeRegistered(const QString& url);

} // namespace MacPlatformIntegration
