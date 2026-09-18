#pragma once
#include "Framework/GameplayFramework.h"
#include "Content/WorldCatalog.h"
#include <iomanip>
#include <sstream>
namespace Wroclaw
{
enum class DiscoveryState
{
    Unknown,
    Discovered,
    Visited,
    Completed
};
enum class SimulationLevel
{
    Full,
    Simplified,
    Dormant
};
enum class HeatResponseLevel
{
    Quiet,
    Watch,
    Search,
    Pursuit,
    Lockdown,
    Manhunt
};
struct WorldState
{
    static constexpr int SaveVersion = 1;
    double heat = 0, hour = initial_hour, elapsed = 0, quiet = quiet_seconds, nextEvent = 0;
    uint32_t seed = 20260917;
    std::string district, weather = initial_weather;
    std::map<std::string, double> cooldowns;
    std::map<std::string, int> discoveries;
    std::set<std::string> flags, delivered, photos;
    bool wasThreat = false;
    double Random()
    {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        return seed / 4294967296.0;
    }
    int HeatLevel() const
    {
        return std::min(5, static_cast<int>(heat / 20));
    }
    HeatResponseLevel HeatResponse() const
    {
        return static_cast<HeatResponseLevel>(HeatLevel());
    }
    void AddHeat(double amount)
    {
        if (std::isfinite(amount))
            heat = std::clamp(heat + amount, 0.0, 100.0);
    }
    const WeatherDef *Weather() const
    {
        for (const auto &w : WeatherProfiles())
            if (w.id == weather)
                return &w;
        return nullptr;
    }
    bool SetWeather(const std::string &id)
    {
        for (const auto &w : WeatherProfiles())
            if (w.id == id)
            {
                weather = id;
                return true;
            }
        return false;
    }
    bool SetTime(double value)
    {
        if (!std::isfinite(value) || value < 0 || value >= 24)
            return false;
        hour = value;
        return true;
    }
    const DistrictDef *District(double x, double y, double z) const
    {
        for (const auto &d : Districts())
        {
            const auto &b = d.bounds;
            if (x >= b[0] && y >= b[1] && z >= b[2] && x <= b[3] && y <= b[4] && z <= b[5])
                return &d;
        }
        return nullptr;
    }
    void Enter(const DistrictDef *d)
    {
        const auto id = d ? d->id : "";
        if (id != district)
        {
            if (!district.empty())
                AddHeat(-6);
            district = id;
            if (d)
                flags.insert("District." + d->id + ".Discovered");
        }
    }
    void Discover(const std::string &id, bool visited)
    {
        auto &state = discoveries[id];
        state =
            std::max(state, static_cast<int>(visited ? DiscoveryState::Visited : DiscoveryState::Discovered));
    }
    bool Rest(const LocationDef &place, bool threat)
    {
        if (!place.safehouse || threat || !discoveries.count(place.id))
            return false;
        AddHeat(-30);
        flags.insert("Safehouse." + place.id + ".Unlocked");
        quiet = 0;
        return true;
    }
    void Tick(double dt, bool threat, bool hidden)
    {
        if (!std::isfinite(dt) || dt <= 0 || dt > 5)
            return;
        elapsed += dt;
        hour = std::fmod(hour + dt * clock_rate / 3600, 24.0);
        if (threat)
        {
            quiet = 0;
            if (!wasThreat)
                AddHeat(15);
        }
        else
        {
            quiet += dt;
            AddHeat(-dt * heat_decay * (hidden ? 3 : 1));
            if (wasThreat)
                AddHeat(-10);
        }
        wasThreat = threat;
    }
    bool Eligible(const WorldEventDef &e, const Progress &p) const
    {
        if (e.district != district || heat < e.min_heat || heat > e.max_heat || !p.All(e.prerequisites))
            return false;
        for (const auto &tag : e.require_tags)
            if (!p.Tagged(tag) && !flags.count(tag))
                return false;
        if (std::find(e.weathers.begin(), e.weathers.end(), weather) == e.weathers.end())
            return false;
        const bool time = e.start_hour <= e.end_hour ? (hour >= e.start_hour && hour < e.end_hour)
                                                     : (hour >= e.start_hour || hour < e.end_hour);
        if (!time || (e.hostile && (wasThreat || quiet < quiet_seconds)))
            return false;
        auto it = cooldowns.find(e.id);
        return it == cooldowns.end() || elapsed >= it->second;
    }
    const WorldEventDef *SelectEvent(const Progress &p, double x = 0, double y = 0, double z = 0,
                                     bool local = false)
    {
        if (elapsed < nextEvent)
            return nullptr;
        nextEvent = elapsed + event_interval;
        // Rotate the first candidate deterministically; no fixed first-definition bias.
        const auto &events = WorldEvents();
        if (events.empty())
            return nullptr;
        auto start = static_cast<size_t>(Random() * events.size());
        for (size_t i = 0; i < events.size(); ++i)
        {
            const auto &e = events[(start + i) % events.size()];
            const double dx = e.position[0] - x, dy = e.position[1] - y, dz = e.position[2] - z;
            if ((!local || dx * dx + dy * dy + dz * dz < 3500 * 3500) && Eligible(e, p) &&
                Random() < e.chance)
            {
                cooldowns[e.id] = elapsed + e.cooldown;
                AddHeat(e.heat);
                if (e.hostile)
                    quiet = 0;
                return &e;
            }
        }
        return nullptr;
    }
    std::vector<const MessageDef *> NewMessages(const Progress &p)
    {
        std::vector<const MessageDef *> out;
        for (const auto &m : Messages())
        {
            if (delivered.count(m.id) || heat < m.min_heat || !p.All(m.prerequisites))
                continue;
            bool ok = true;
            for (const auto &tag : m.tags)
                if (!p.Tagged(tag))
                    ok = false;
            if (ok)
            {
                delivered.insert(m.id);
                out.push_back(&m);
            }
        }
        return out;
    }
    static double Visibility(double distance, double light, bool crouched, double speed, bool hidden,
                             double weatherVisibility, bool flashlight)
    {
        if (hidden)
            return 0;
        double posture = crouched ? .5 : 1;
        double movement = speed > 350 ? 1.3 : (speed < 10 ? .7 : 1);
        return std::clamp((flashlight ? 1.2 : light) * posture * movement * weatherVisibility *
                              (1 - distance / 2200),
                          0.0, 1.0);
    }
    static SimulationLevel Simulation(double distance, const GuardDef &g)
    {
        return distance < g.full_range
                   ? SimulationLevel::Full
                   : (distance < g.simple_range ? SimulationLevel::Simplified : SimulationLevel::Dormant);
    }
    bool Combine(const CombinationDef &combo, Progress &p)
    {
        for (const auto &e : combo.evidence)
            if (!p.evidence.count(e))
                return false;
        const auto result = p.Apply(combo.action, wasThreat);
        if (result == Result::Applied)
        {
            flags.insert("Evidence." + combo.id + ".Confirmed");
            return true;
        }
        return result == Result::AlreadyDone;
    }
    bool Valid() const
    {
        if (!std::isfinite(heat) || heat < 0 || heat > 100 || !std::isfinite(hour) || hour < 0 ||
            hour >= 24 || !std::isfinite(elapsed) || elapsed < 0 || !std::isfinite(quiet) || quiet < 0 ||
            !std::isfinite(nextEvent) || nextEvent < 0 || !seed || !Weather())
            return false;
        if (!district.empty() && std::none_of(Districts().begin(), Districts().end(),
                                              [&](const auto &d) { return d.id == district; }))
            return false;
        for (const auto &e : cooldowns)
            if (!std::isfinite(e.second) || e.second < 0 ||
                std::none_of(
                    WorldEvents().begin(), WorldEvents().end(),
                    [&](const auto &d) { return d.id == e.first; }))
                return false;
        for (const auto &e : discoveries)
            if (e.second < 1 || e.second > 3 ||
                std::none_of(Locations().begin(), Locations().end(),
                             [&](const auto &d) { return d.id == e.first; }))
                return false;
        if (flags.size() > 2048 || photos.size() > 256 || delivered.size() > 1024)
            return false;
        for (const auto &s : flags)
            if (s.empty() || s.size() > 160)
                return false;
        for (const auto &s : delivered)
            if (std::none_of(Messages().begin(), Messages().end(), [&](const auto &d) { return d.id == s; }))
                return false;
        return true;
    }
    std::string Serialize() const
    {
        std::ostringstream o;
        o << std::setprecision(17) << SaveVersion << ' ' << heat << ' ' << hour << ' ' << elapsed << ' '
          << quiet << ' ' << nextEvent << ' ' << seed << ' ' << wasThreat << ' ' << std::quoted(district)
          << ' ' << std::quoted(weather) << '\n';
        o << cooldowns.size() << '\n';
        for (const auto &x : cooldowns)
            o << std::quoted(x.first) << ' ' << x.second << '\n';
        o << discoveries.size() << '\n';
        for (const auto &x : discoveries)
            o << std::quoted(x.first) << ' ' << x.second << '\n';
        for (const auto *set : {&flags, &delivered, &photos})
        {
            o << set->size() << '\n';
            for (const auto &x : *set)
                o << std::quoted(x) << '\n';
        }
        return o.str();
    }
    static bool Deserialize(const std::string &text, WorldState &out)
    {
        if (text.size() > 262144)
            return false;
        WorldState c;
        std::istringstream in(text);
        int version = 0;
        if (!(in >> version >> c.heat >> c.hour >> c.elapsed >> c.quiet >> c.nextEvent >> c.seed >>
              c.wasThreat >> std::quoted(c.district) >> std::quoted(c.weather)) ||
            version != SaveVersion)
            return false;
        size_t n;
        std::string id;
        double value;
        int state;
        if (!(in >> n) || n > 1024)
            return false;
        for (size_t i = 0; i < n; ++i)
        {
            if (!(in >> std::quoted(id) >> value) || !c.cooldowns.emplace(id, value).second)
                return false;
        }
        if (!(in >> n) || n > 1024)
            return false;
        for (size_t i = 0; i < n; ++i)
        {
            if (!(in >> std::quoted(id) >> state) || !c.discoveries.emplace(id, state).second)
                return false;
        }
        for (auto *set : {&c.flags, &c.delivered, &c.photos})
        {
            if (!(in >> n) || n > 2048)
                return false;
            for (size_t i = 0; i < n; ++i)
            {
                if (!(in >> std::quoted(id)) || !set->insert(id).second)
                    return false;
            }
        }
        in >> std::ws;
        if (!in.eof() || !c.Valid())
            return false;
        out = std::move(c);
        return true;
    }
};
} // namespace Wroclaw
