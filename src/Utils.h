#pragma once

#include <QObject>
#include <QString>
#include <QPoint>

/// Форматировать размер в байтах в читаемый вид (Б, КБ, МБ, ГБ, ТБ)
inline QString formatBytes(double bytes) {
    if (bytes < 1024.0)
        return QObject::tr("%1 Б").arg(static_cast<int>(bytes));
    if (bytes < 1024.0 * 1024.0)
        return QObject::tr("%1 КБ").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0 * 1024.0)
        return QObject::tr("%1 МБ").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0 * 1024.0 * 1024.0)
        return QObject::tr("%1 ГБ").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
    return QObject::tr("%1 ТБ").arg(bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

/// Рассчитать позицию контекстного меню трея для Linux (Вариант C).
/// Определяет положение панели задач через availableGeometry() и привязывает
/// меню к соответствующему краю экрана.
/// @param menuWidth Ширина меню
/// @param menuHeight Высота меню
/// @return Экранная позиция для move()
QPoint calculateLinuxTrayMenuPosition(int menuWidth, int menuHeight);
