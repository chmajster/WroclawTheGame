#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
namespace Wroclaw
{
struct RoadPoint
{
    double x = 0, y = 0, z = 0;
};
enum class RaceStatus
{
    Idle,
    Running,
    Finished,
    Failed
};
class RaceProgress
{
  public:
    std::vector<RoadPoint> checkpoints;
    RaceStatus status = RaceStatus::Idle;
    unsigned next = 0;
    double elapsed = 0, limit = 0, radius = 250, cargo = 100;
    bool delivery = false;
    RoadPoint previous, courseStart;
    bool Start(const std::vector<RoadPoint> &points, RoadPoint start, double seconds, bool isDelivery = false)
    {
        if (!std::isfinite(start.x) || !std::isfinite(start.y) || !std::isfinite(start.z))
            return false;
        if (points.size() < 2 || !std::isfinite(seconds) || seconds <= 0)
            return false;
        for (const auto &p : points)
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
                return false;
        checkpoints = points;
        previous = start;
        courseStart = start;
        elapsed = 0;
        limit = seconds;
        next = 0;
        cargo = 100;
        delivery = isDelivery;
        status = RaceStatus::Running;
        return true;
    }
    void Tick(double dt, RoadPoint position, double damage = 0)
    {
        if (status != RaceStatus::Running || !std::isfinite(dt) || dt <= 0)
            return;
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) ||
            !std::isfinite(damage))
        {
            status = RaceStatus::Failed;
            return;
        }
        elapsed += dt;
        cargo = std::max(0., cargo - std::max(0., damage));
        if (elapsed > limit || (delivery && cargo <= 0))
        {
            status = RaceStatus::Failed;
            return;
        }
        const double dx = position.x - previous.x, dy = position.y - previous.y, dz = position.z - previous.z;
        const double length = dx * dx + dy * dy + dz * dz;
        // Consume only the next ordered gate, intersecting the swept movement segment.
        while (next < checkpoints.size() && length > 1e-9)
        {
            const auto &point = checkpoints[next];
            const auto &origin = next ? checkpoints[next - 1] : courseStart;
            if (dx * (point.x - origin.x) + dy * (point.y - origin.y) <= 0)
                break;
            const double t = std::clamp(
                ((point.x - previous.x) * dx + (point.y - previous.y) * dy + (point.z - previous.z) * dz) /
                    length,
                0., 1.);
            const double ex = previous.x + t * dx - point.x, ey = previous.y + t * dy - point.y,
                         ez = previous.z + t * dz - point.z;
            if (ex * ex + ey * ey + ez * ez > radius * radius)
                break;
            ++next;
        }
        previous = position;
        if (next == checkpoints.size())
            status = RaceStatus::Finished;
    }
};
} // namespace Wroclaw
