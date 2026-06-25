#include "SingleInstanceGuard.h"
#include <QCoreApplication>
#include <QByteArray>

SingleInstanceGuard::SingleInstanceGuard(QObject* parent)
    : QObject(parent)
{
    m_server = std::make_unique<QLocalServer>(this);

    // Атомарная попытка занять сокет — только один процесс может
    // успешно вызвать listen() на одном и том же имени сокета.
    if (m_server->listen(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME))) {
        m_isPrimary = true;
        qDebug() << "SingleInstanceGuard: первичный экземпляр, сокет создан";

        // Принимаем подключения от вторичных экземпляров
        connect(m_server.get(), &QLocalServer::newConnection,
                this, &SingleInstanceGuard::onNewConnection);
        return;
    }

    // Сокет занят — возможно, это stale-сокет после аварийного завершения.
    // Удаляем его и повторяем попытку.
    QLocalServer::removeServer(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME));

    if (m_server->listen(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME))) {
        m_isPrimary = true;
        qDebug() << "SingleInstanceGuard: первичный экземпляр, stale-сокет очищен и создан";

        connect(m_server.get(), &QLocalServer::newConnection,
                this, &SingleInstanceGuard::onNewConnection);
        return;
    }

    // Сокет занят другим процессом — этот экземпляр вторичный.
    qWarning() << "SingleInstanceGuard: вторичный экземпляр, сокет занят —"
               << m_server->errorString();
}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
    // Удаляем сокет-файл (Linux/macOS) или именованный канал (Windows)
    QLocalServer::removeServer(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME));
    qDebug() << "SingleInstanceGuard: сокет удалён";
}

void SingleInstanceGuard::onNewConnection() {
    while (QLocalSocket* socket = m_server->nextPendingConnection()) {
        // Читаем данные. Используем waitForReadyRead с небольшим таймаутом,
        // так как данные могут прийти не мгновенно.
        const int readTimeoutMs = 500;
        if (socket->waitForReadyRead(readTimeoutMs)) {
            QByteArray data = socket->readAll();
            QString command = QString::fromUtf8(data).trimmed();
            if (!command.isEmpty()) {
                qDebug() << "SingleInstanceGuard: получена команда —" << command;
                emit commandReceived(command);
            }
        } else {
            // Таймаут чтения — возможно, клиент уже отключился.
            // Пробуем прочитать то, что есть.
            QByteArray data = socket->readAll();
            if (!data.isEmpty()) {
                QString command = QString::fromUtf8(data).trimmed();
                if (!command.isEmpty()) {
                    qDebug() << "SingleInstanceGuard: получена команда (без waitForReadyRead) —" << command;
                    emit commandReceived(command);
                }
            }
        }

        socket->close();
        socket->deleteLater();
    }
}

bool SingleInstanceGuard::notifyPrimaryInstance(const QString& command) {
    QLocalSocket socket;
    socket.connectToServer(QLatin1String(SINGLE_INSTANCE_SOCKET_NAME));

    const int connectTimeoutMs = 500;
    if (!socket.waitForConnected(connectTimeoutMs)) {
        // Не удалось подключиться — первичного экземпляра нет
        qDebug() << "SingleInstanceGuard: не удалось подключиться к серверу (первичный экземпляр не запущен)";
        return false;
    }

    // Отправляем команду
    QByteArray data = command.toUtf8();
    qint64 written = socket.write(data);
    if (written != data.size()) {
        qWarning() << "SingleInstanceGuard: не удалось отправить команду";
        return true; // Подключение было — считаем, что первичный существует
    }
    socket.flush();

    // Даём серверу время прочитать данные перед отключением
    socket.waitForBytesWritten(200);
    socket.disconnectFromServer();

    qDebug() << "SingleInstanceGuard: команда отправлена —" << command;
    return true;
}
