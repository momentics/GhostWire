#include "StatsTracker.h"
#include "Config.h"

void StatsTracker::reset() {
    m_prevBytesReceived = 0;
    m_prevBytesSent = 0;
    m_peakRx = 0;
    m_peakTx = 0;
    m_hasPrevStats = false;
    m_prevWsActive = 0;
}

bool StatsTracker::processTick(const GhostWireProxyStats& stats,
                               GhostWireProxyState proxyState,
                               double& outPeakRx,
                               double& outPeakTx,
                               double& outRxRate,
                               double& outTxRate,
                               bool& outHasConnections) {
    if (proxyState == GHOSTWIRE_PROXY_OFFLINE) {
        reset();
        outPeakRx = 0;
        outPeakTx = 0;
        outRxRate = 0;
        outTxRate = 0;
        outHasConnections = false;
        return false;
    }

    // Отслеживаем изменение количества WS-соединений для индикации.
    if (stats.websocket_active != m_prevWsActive || !m_hasPrevStats
        || proxyState == GHOSTWIRE_PROXY_DEGRADED) {
        m_prevWsActive = stats.websocket_active;
        outHasConnections =
            proxyState == GHOSTWIRE_PROXY_ONLINE && stats.websocket_active > 0;
    } else {
        outHasConnections = false;
    }

    outRxRate = 0;
    outTxRate = 0;

    // Рассчитать дельту RX/TX и обновить пики.
    if (m_hasPrevStats) {
        double rxDelta = 0;
        double txDelta = 0;

        // Защита от wrap-around.
        if (stats.bytes_received >= m_prevBytesReceived)
            rxDelta = static_cast<double>(stats.bytes_received - m_prevBytesReceived);
        if (stats.bytes_sent >= m_prevBytesSent)
            txDelta = static_cast<double>(stats.bytes_sent - m_prevBytesSent);

        double intervalSec = Config::STATS_POLL_INTERVAL_MS / 1000.0;
        double rxRate = rxDelta / intervalSec;
        double txRate = txDelta / intervalSec;

        outRxRate = rxRate;
        outTxRate = txRate;

        if (rxRate > m_peakRx) m_peakRx = rxRate;
        if (txRate > m_peakTx) m_peakTx = txRate;
    }

    outPeakRx = m_peakRx;
    outPeakTx = m_peakTx;

    m_prevBytesReceived = stats.bytes_received;
    m_prevBytesSent = stats.bytes_sent;
    m_hasPrevStats = true;

    return true;
}
