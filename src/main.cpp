#include "Application.h"
#include "SingleInstanceGuard.h"
#include "version.h"
#include <QApplication>
#include <QDebug>
#include <QTranslator>
#include <QLocale>

int main(int argc, char* argv[]) {
    // Включаем сглаживание и поддержку HiDPI для всего приложения
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("GhostWire Desktop");
    app.setApplicationVersion(GHOSTWIRE_VERSION);
    app.setQuitOnLastWindowClosed(false); // Нет окон — не выходим автоматически

    // ─── Проверка единственного экземпляра ──────────────────────────────────
    // Пытаемся отправить команду первичному экземпляру.
    // Если удалось — значит, приложение уже запущено, показываем меню и выходим.
    if (SingleInstanceGuard::notifyPrimaryInstance(COMMAND_SHOW_MENU)) {
        qDebug() << "main: приложение уже запущено, отправили команду show_menu, завершаем";
        return 0;
    }

    // Первичный экземпляр не найден — продолжаем запуск
    qDebug() << "main: первичный экземпляр не обнаружен, запускаемся";

    // Держим guard живым весь жизненный цикл приложения
    auto* guard = new SingleInstanceGuard(&app);

    // ─── Мультиязычность: определяем язык ОС ───────────────────────────────
    QLocale locale = QLocale::system();
    QString lang = locale.name().left(2); // "ru", "en", "de", ...

    // По умолчанию русский (source), если не ru
    if (lang != "ru") {
        QTranslator* translator = new QTranslator(&app);
        if (translator->load(":/translations/ghostwire_" + lang + ".qm")) {
            app.installTranslator(translator);
            qDebug() << "main: loaded translation for" << lang;
        } else {
            delete translator;
            qDebug() << "main: no translation for" << lang << ", using default (ru)";
        }
    }

    Application application;

    // Связываем команды guard с Application
    QObject::connect(guard, &SingleInstanceGuard::commandReceived,
                     &application, [&application](const QString& command) {
        if (command == QLatin1String(COMMAND_SHOW_MENU)) {
            application.showTrayMenuAtCursor();
        }
    });

    if (!application.initialize()) {
        qCritical() << "Не удалось инициализировать приложение";
        return 1;
    }

    return app.exec();
}
