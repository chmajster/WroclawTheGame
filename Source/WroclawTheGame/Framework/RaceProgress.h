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
enum class RaceMode
{
    TimeTrial,
    Delivery,
    Escape,
    Follow,
    Navigation
};
class RaceProgress
{
  public:
    std::vector<RoadPoint> checkpoints;
    RaceStatus status = RaceStatus::Idle;
    RaceMode mode = RaceMode::TimeTrial;
    unsigned next = 0;
    double elapsed = 0, limit = 0, radius = 250, cargo = 100;
    double followSuspicion = 0, lostTarget = 0;
    bool delivery = false, pursuitLost = false;
    RoadPoint previous, courseStart;

    bool Start(const std::vector<RoadPoint> &points, RoadPoint start, double seconds, bool isDelivery = false)
    {
        return Start(points, start, seconds, isDelivery ? RaceMode::Delivery : RaceMode::TimeTrial);
    }
    bool Start(const std::vector<RoadPoint> &points, RoadPoint start, double seconds, RaceMode raceMode)
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
        followSuspicion = 0;
        lostTarget = 0;
        pursuitLost = false;
        mode = raceMode;
        delivery = raceMode == RaceMode::Delivery;
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
        if (mode == RaceMode::Follow)
        {
            previous = position;
            return;
        }

        const double dx = position.x - previous.x, dy = position.y - previous.y, dz = position.z - previous.z;
        const double length = dx * dx + dy * dy + dz * dz;
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
        if (mode != RaceMode::Escape && next == checkpoints.size())
            status = RaceStatus::Finished;
        else if (mode == RaceMode::Escape && pursuitLost)
            status = RaceStatus::Finished;
    }
    void MarkPursuitLost()
    {
        if (status == RaceStatus::Running && mode == RaceMode::Escape)
        {
            pursuitLost = true;
            status = RaceStatus::Finished;
        }
    }
    void TickFollow(double dt, double distance, bool targetVisible, bool targetFinished,
                    double minDistance, double maxDistance, double lostSeconds)
    {
        if (status != RaceStatus::Running || mode != RaceMode::Follow || !std::isfinite(dt) || dt <= 0)
            return;
        if (!std::isfinite(distance) || !std::isfinite(minDistance) || !std::isfinite(maxDistance) ||
            !std::isfinite(lostSeconds) || minDistance <= 0 || maxDistance <= minDistance || lostSeconds <= 0)
        {
            status = RaceStatus::Failed;
            return;
        }
        elapsed += dt;
        if (elapsed > limit)
        {
            status = RaceStatus::Failed;
            return;
        }

        if (!targetVisible || distance > maxDistance)
            lostTarget += dt;
        else
            lostTarget = 0;

        if (distance < minDistance)
        {
            const double closeness = std::clamp((minDistance - distance) / minDistance, 0.0, 1.0);
            followSuspicion = std::min(100.0, followSuspicion + dt * (15.0 + 35.0 * closeness));
        }
        else
            followSuspicion = std::max(0.0, followSuspicion - dt * 8.0);

        if (lostTarget > lostSeconds || followSuspicion >= 100.0)
        {
            status = RaceStatus::Failed;
            return;
        }
        if (targetFinished && distance <= maxDistance)
            status = RaceStatus::Finished;
    }
};
} // namespace Wroclaw
