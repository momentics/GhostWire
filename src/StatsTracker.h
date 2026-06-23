#pragma once

#include <cstdint>

struct GhostWireProxyStats;
enum GhostWireProxyState : int;

/// Отслеживает дельты RX/TX, пики скорости и изменения WS-соединений.
/// Инкапсулирует логику расчёта, вынесенную из Application (SRP).
class StatsTracker {
public:
    StatsTracker() = default;

    /// Сбросить все накопленные данные (при остановке прокси).
    void reset();

    /// Обработать новый тик статистики.
    /// @param stats Текущая статистика из библиотеки.
    /// @param proxyState Текущее состояние прокси.
    /// @param[out] outPeakRx Пиковая скорость приёма (байт/сек).
    /// @param[out] outPeakTx Пиковая скорость передачи (байт/сек).
    /// @param[out] outRxRate Скорость приёма за текущий тик (байт/сек, 0 если первый тик).
    /// @param[out] outTxRate Скорость передачи за текущий тик (байт/сек, 0 если первый тик).
    /// @param[out] outHasConnections Есть ли активные WS-соединения.
    /// @return true, если прокси ещё работает; false, если перешёл в OFFLINE.
    bool processTick(const GhostWireProxyStats& stats,
                     GhostWireProxyState proxyState,
                     double& outPeakRx,
                     double& outPeakTx,
                     double& outRxRate,
                     double& outTxRate,
                     bool& outHasConnections);

    /// Текущий пик приёма.
    double peakRx() const { return m_peakRx; }

    /// Текущий пик передачи.
    double peakTx() const { return m_peakTx; }

private:
    uint64_t m_prevBytesReceived = 0;
    uint64_t m_prevBytesSent = 0;
    double   m_peakRx = 0;
    double   m_peakTx = 0;
    bool     m_hasPrevStats = false;
    uint64_t m_prevWsActive = 0;
};
