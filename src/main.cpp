#include "Application.h"
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

    if (!application.initialize()) {
        qCritical() << "Не удалось инициализировать приложение";
        return 1;
    }

    return app.exec();
}
