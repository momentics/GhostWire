#include "Application.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GhostWire");
    app.setQuitOnLastWindowClosed(false); // Нет окон — не выходим автоматически

    Application application;

    if (!application.initialize()) {
        qCritical() << "Не удалось инициализировать приложение";
        return 1;
    }

    return app.exec();
}
