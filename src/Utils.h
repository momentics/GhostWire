#pragma once

#include <QCoreApplication>
#include <QString>
#include <QPoint>

/// Форматировать размер в байтах в читаемый вид (Б, КБ, МБ, ГБ, ТБ).
/// Вынесен в класс, чтобы lupdate приписывал переводы собственному контексту,
/// а не общему QObject (свободная функция не имеет корректного контекста).
class ByteFormatter {
public:
    static QString format(double bytes);

private:
    /// Перевод с явным контекстом для lupdate.
    static QString tr(const char* text) {
        return QCoreApplication::translate("ByteFormatter", text);
    }
};

/// Рассчитать позицию контекстного меню трея для Linux
/// Определяет положение панели задач через availableGeometry() и привязывает
/// меню к соответствующему краю экрана.
/// @param menuWidth Ширина меню
/// @param menuHeight Высота меню
/// @return Экранная позиция для move()
QPoint calculateLinuxTrayMenuPosition(int menuWidth, int menuHeight);
