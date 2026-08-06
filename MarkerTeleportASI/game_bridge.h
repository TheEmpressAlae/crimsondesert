#pragma once

#include <cstdint>

namespace marker_teleport {

struct TeleportRequest {
    float fallbackHeight{1200.0F};
    std::uint32_t invulnerabilityMilliseconds{10000};
};

enum class TeleportResult {
    Success,
    NotReady,
    NoPlayer,
    NoMarker,
    UnsafeContext,
    InvalidCoordinates,
    WriteFailed,
};

class GameBridge final {
public:
    // Resolves current-build interfaces and validates every signature. Must
    // return false unless each required target has exactly one safe match.
    bool Initialize();
    void Shutdown();

    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] TeleportResult TeleportToMarker(const TeleportRequest& request);
    void ServiceProtectionExpiry(std::uint64_t nowMilliseconds);

private:
    bool ready_{false};
    bool protectionReady_{false};
    std::uint64_t protectionDeadline_{0};
};

}  // namespace marker_teleport
