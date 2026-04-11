#pragma once

#include <QString>

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
