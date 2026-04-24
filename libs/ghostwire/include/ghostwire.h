#ifndef GHOSTWIRE_H
#define GHOSTWIRE_H

#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(GHOSTWIRE_STATIC)
#    define GHOSTWIRE_API
#  elif defined(GHOSTWIRE_BUILD_SHARED)
#    define GHOSTWIRE_API __declspec(dllexport)
#  else
#    define GHOSTWIRE_API __declspec(dllimport)
#  endif
#else
#  if defined(__GNUC__) && __GNUC__ >= 4
#    define GHOSTWIRE_API __attribute__((visibility("default")))
#  else
#    define GHOSTWIRE_API
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GwProxy GwProxy;

typedef enum GhostWireProxyState {
    GHOSTWIRE_PROXY_OFFLINE = 0,
    GHOSTWIRE_PROXY_ONLINE = 1,
    GHOSTWIRE_PROXY_DEGRADED = 2
} GhostWireProxyState;

typedef struct GhostWireProxyStats {
    bool running;
    uint64_t active_connections;
    uint64_t total_connections;
    uint64_t websocket_active;
    uint64_t bytes_received;
    uint64_t bytes_sent;
    uint64_t errors;
    uint64_t ip_rotations;
    uint64_t media_connections;
    uint64_t uptime_secs;
    uint64_t peak_active_connections;
    uint64_t rotation_success;
} GhostWireProxyStats;

typedef GhostWireProxyStats ProxyStats;

GHOSTWIRE_API GwProxy* ghostwire_proxy_create(const char* config_json);
GHOSTWIRE_API GwProxy* ghostwire_proxy_create_from_file(const char* config_path);
GHOSTWIRE_API void ghostwire_proxy_free(GwProxy* proxy);

GHOSTWIRE_API int ghostwire_proxy_start(GwProxy* proxy);
GHOSTWIRE_API void ghostwire_proxy_stop(GwProxy* proxy);
GHOSTWIRE_API GhostWireProxyState ghostwire_proxy_get_state(const GwProxy* proxy);

GHOSTWIRE_API void ghostwire_proxy_get_stats(
    const GwProxy* proxy,
    GhostWireProxyStats* out_stats
);

#ifdef __cplusplus
}
#endif

#endif
