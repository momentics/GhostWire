#include "Application.h"
#include "SingleInstanceGuard.h"
#include "version.h"
#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QLocalServer>
#include <QTranslator>
#include <QLocale>
#include <QtGlobal>
#include <memory>

int main(int argc, char* argv[]) {
    // Включаем сглаживание и поддержку HiDPI для всего приложения
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    // Сохраняем дробные масштабы Windows вроде 125% и 175%.
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("GhostWire Desktop");
    app.setApplicationVersion(GHOSTWIRE_VERSION);
    app.setQuitOnLastWindowClosed(false); // Нет окон — не выходим автоматически

    // ─── Проверка единственного экземпляра ──────────────────────────────────
    // Сначала пытаемся занять сокет (атомарная операция — только один процесс
    // может успешно вызвать listen() на одном имени). Если не удалось — значит,
    // приложение уже запущено; отправляем команду первичному экземпляру и выходим.
    auto guard = std::make_unique<SingleInstanceGuard>(&app);

    if (!guard->isPrimaryInstance()) {
        // Сокет занят — отправляем команду существующему экземпляру.
        if (SingleInstanceGuard::notifyPrimaryInstance(COMMAND_SHOW_MENU)) {
            qDebug() << "main: приложение уже запущено, отправили команду show_menu, завершаем";
            return 0;
        }

        // Уведомление не доставлено — первичный экземпляр может быть зависшим.
        // Пытаемся повторно занять сокет.
        QLocalServer::removeServer(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME));
        guard = std::make_unique<SingleInstanceGuard>(&app);

        if (!guard->isPrimaryInstance()) {
            qCritical() << "main: невозможно запустить — сокет занят, но экземпляр не отвечает";
            return 1;
        }

        qDebug() << "main: первичный экземпляр завис — перезаняли сокет, запускаемся";
    }

    qDebug() << "main: первичный экземпляр, запускаемся";

    // ─── Мультиязычность: определяем язык ОС ───────────────────────────────
    QLocale locale = QLocale::system();
    QString lang = locale.name().left(2); // "ru", "en", "de", ...

    // Загружаем стандартные Qt-переводы (кнопки QMessageBox и т.д.)
    // windeployqt кладёт qt_XX.qm в translations/ рядом с exe
    QTranslator* qtTranslator = new QTranslator(&app);
    QString qtTransPath = app.applicationDirPath() + "/translations/qt_" + lang + ".qm";
    if (qtTranslator->load(qtTransPath)) {
        app.installTranslator(qtTranslator);
        qDebug() << "main: loaded Qt translation for" << lang;
    } else {
        delete qtTranslator;
    }

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
    QObject::connect(guard.get(), &SingleInstanceGuard::commandReceived,
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
