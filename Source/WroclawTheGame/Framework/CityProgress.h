#pragma once
#include "Content/CityGameplayCatalog.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
namespace Wroclaw
{
class CityProgress
{
  public:
    std::set<std::string> completed;
    std::map<std::string, double> timers;
    static const CityActionDef *Find(const std::string &id)
    {
        for (const auto &action : CityActions()) if (action.id == id) return &action;
        return nullptr;
    }
    bool Available(const CityActionDef &action) const
    {
        return !completed.count(action.id) && std::all_of(action.prerequisites.begin(), action.prerequisites.end(),
            [&](const std::string &id) { return completed.count(id) != 0; });
    }
    bool Step(const std::string &id, double dt, bool inRange, bool vehicle, bool crouch, bool activate)
    {
        const auto *action = Find(id);
        if (!action || !Available(*action) || !std::isfinite(dt) || dt < 0) return false;
        if (!inRange || (action->vehicle && !vehicle) || (action->crouch && !crouch))
        {
            timers.erase(id); return false;
        }
        if (action->seconds > 0)
        {
            timers[id] += std::min(dt, 1.0);
            if (timers[id] < action->seconds) return false;
        }
        else if (!activate && action->kind != "event") return false;
        completed.insert(id); timers.erase(id); return true;
    }
    static bool ValidId(const std::string &id)
    {
        return !id.empty() && id.size() <= 96 && std::all_of(id.begin(), id.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        });
    }
    std::string Serialize() const
    {
        if (completed.size() > 10000) return {};
        std::ostringstream out; out << "WTGCITY1\n" << completed.size() << '\n';
        for (const auto &id : completed) { if (!ValidId(id)) return {}; out << id << '\n'; }
        return out.str();
    }
    static bool Deserialize(const std::string &text, CityProgress &result)
    {
        if (text.size() > 1000000) return false;
        std::istringstream input(text); std::string header, countText;
        if (!std::getline(input, header) || header != "WTGCITY1" || !std::getline(input, countText) ||
            countText.empty() || countText.size() > 5 ||
            !std::all_of(countText.begin(), countText.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return false;
        const auto count = std::stoul(countText); if (count > 10000) return false;
        CityProgress parsed;
        for (size_t i = 0; i < count; ++i)
        {
            std::string id;
            if (!std::getline(input, id) || !ValidId(id) || !parsed.completed.insert(id).second) return false;
        }
        if (input.peek() != std::char_traits<char>::eof()) return false;
        result = std::move(parsed); return true;
    }
};
}
