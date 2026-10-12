#pragma once
#include <algorithm>
#include <cmath>

namespace Wroclaw
{
// Independent of rendering/UE: offences reset the quiet interval; visible
// police prevent cooling. Decay is time-based, not dependent on frame count.
struct WantedState
{
    float Heat = 0;
    float QuietSeconds = 0;
    int Level() const { return Heat <= 0 ? 0 : std::min(3, int(std::ceil(Heat / 30.f))); }
    void Report(float Severity)
    {
        if (!std::isfinite(Severity) || Severity <= 0) return;
        Heat = std::min(90.f, Heat + Severity);
        QuietSeconds = 0;
    }
    void Tick(float Dt, bool Seen)
    {
        if (!std::isfinite(Dt) || Dt <= 0) return;
        if (Seen) { QuietSeconds = 0; return; }
        const float Before = QuietSeconds;
        QuietSeconds += Dt;
        const float Cooling = std::max(0.f, QuietSeconds - 18.f) - std::max(0.f, Before - 18.f);
        Heat = std::max(0.f, Heat - Cooling * 2.5f);
    }
};
}
