#include "Utils.h"
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QPoint>
#include <QCursor>

QPoint calculateLinuxTrayMenuPosition(int menuWidth, int menuHeight) {
    // На Linux QSystemTrayIcon::geometry() возвращает пустой QRect.
    // Определяем положение панели (taskbar) через разницу между geometry() и availableGeometry().
    // Панель сверху: availableGeometry.top() > geometry.top()
    // Панель снизу: availableGeometry.bottom() < geometry.bottom()
    // Иначе — боковая панель или плавающая (fallback к курсору).

    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeom = screen->geometry();
    QRect availableGeom = screen->availableGeometry();

    bool panelAtTop = (availableGeom.top() > screenGeom.top());
    bool panelAtBottom = (availableGeom.bottom() < screenGeom.bottom());

    QPoint cursorPos = QCursor::pos();
    int menuX, menuY;
    const int margin = 4;

    if (panelAtTop) {
        // Панель сверху (стандартный Ubuntu GNOME) — меню сразу под панелью
        menuX = cursorPos.x();
        menuY = availableGeom.top() + margin;
    } else if (panelAtBottom) {
        // Панель снизу — меню над панелью
        menuX = cursorPos.x();
        menuY = availableGeom.bottom() - menuHeight - margin;
    } else {
        // Боковая панель или неизвестная конфигурация — fallback к курсору
        // как лучшая попытка
        menuX = cursorPos.x();
        menuY = cursorPos.y();

        // Пробуем показать над курсором, если помещается
        if (menuY - menuHeight > availableGeom.top()) {
            menuY = menuY - menuHeight - margin;
        } else {
            menuY = menuY + margin;
        }
    }

    // Горизонтальная коррекция: не выходить за границы экрана
    if (menuX + menuWidth > availableGeom.right()) {
        menuX = availableGeom.right() - menuWidth;
    }
    if (menuX < availableGeom.left()) {
        menuX = availableGeom.left();
    }

    // Вертикальная коррекция
    if (menuY + menuHeight > availableGeom.bottom()) {
        menuY = availableGeom.bottom() - menuHeight;
    }
    if (menuY < availableGeom.top()) {
        menuY = availableGeom.top();
    }

    return QPoint(menuX, menuY);
}
