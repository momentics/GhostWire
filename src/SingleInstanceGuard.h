#pragma once

#include <QLocalServer>
#include <QLocalSocket>
#include <QDebug>

/// Уникальное имя сокета для предотвращения запуска нескольких копий.
/// Формат: <app>-<hash>-single
///   - Windows: именованный канал \\.\pipe\GhostWireDesktop-7f3a9c2e-single
///   - Linux/macOS: UNIX socket /tmp/GhostWireDesktop-7f3a9c2e-single-<uid>
inline const char* SINGLE_INSTANCE_SOCKET_NAME = "GhostWireDesktop-7f3a9c2e-single";

/// Команда, отправляемая вторичным экземпляром первичному.
inline const char* COMMAND_SHOW_MENU = "show_menu";

/// Контроль единственного экземпляра приложения через QLocalServer/QLocalSocket.
///
/// Сценарий:
///   1. Первичный экземпляр создаёт QLocalServer и слушает подключения.
///   2. Вторичный экземпляр подключается, отправляет команду и завершается.
///   3. Первичный принимает команду, эмитит сигнал — UI реагирует (например, показывает меню).
class SingleInstanceGuard : public QObject {
    Q_OBJECT
public:
    explicit SingleInstanceGuard(QObject* parent = nullptr);
    ~SingleInstanceGuard();

    /// Попытаться уведомить запущенный экземпляр.
    /// Подключается к серверу, отправляет команду и отключается.
    /// \return true, если команда доставлена (вторичный экземпляр должен завершиться).
    static bool notifyPrimaryInstance(const QString& command);

    /// Вернуть true, если сервер успешно создан (этот экземпляр — первичный).
    bool isPrimaryInstance() const { return m_isPrimary; }

signals:
    /// Эмитируется, когда вторичный экземпляр отправил команду.
    void commandReceived(const QString& command);

private slots:
    void onNewConnection();

private:
    QLocalServer* m_server = nullptr;
    bool m_isPrimary = false;
};
