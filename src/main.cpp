#include "Application.h"
#include "version.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    // Включаем сглаживание и поддержку HiDPI для всего приложения
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("GhostWire Desktop");
    app.setApplicationVersion(GHOSTWIRE_VERSION);
    app.setQuitOnLastWindowClosed(false); // Нет окон — не выходим автоматически

    Application application;

    if (!application.initialize()) {
        qCritical() << "Не удалось инициализировать приложение";
        return 1;
    }

    return app.exec();
}
