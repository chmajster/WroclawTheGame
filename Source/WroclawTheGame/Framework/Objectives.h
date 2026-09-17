#pragma once
#include "Content/ChapterCatalog.h"
#include <functional>
#include <map>
#include <set>
#include <memory>
#include <unordered_map>
namespace Wroclaw
{
struct ObjectiveContext
{
    const std::set<std::string> &completed;
    const std::map<std::string, int> &inventory;
    const std::set<std::string> &evidence;
    const std::map<std::string, double> &counters;
};
class ObjectiveEvaluator
{
  public:
    virtual ~ObjectiveEvaluator() = default;
    virtual bool Complete(const ObjectiveDefinition &, const ObjectiveContext &) const = 0;
};
class EventObjective final : public ObjectiveEvaluator
{
  public:
    bool Complete(const ObjectiveDefinition &d, const ObjectiveContext &c) const override
    {
        return c.completed.count(d.target) > 0;
    }
};
class ItemObjective final : public ObjectiveEvaluator
{
  public:
    bool Complete(const ObjectiveDefinition &d, const ObjectiveContext &c) const override
    {
        auto it = c.inventory.find(d.target);
        return it != c.inventory.end() && it->second >= d.amount;
    }
};
class EvidenceObjective final : public ObjectiveEvaluator
{
  public:
    bool Complete(const ObjectiveDefinition &d, const ObjectiveContext &c) const override
    {
        return c.evidence.count(d.target) > 0;
    }
};
class CounterObjective final : public ObjectiveEvaluator
{
  public:
    bool Complete(const ObjectiveDefinition &d, const ObjectiveContext &c) const override
    {
        auto it = c.counters.find(d.target);
        return it != c.counters.end() && it->second >= d.amount;
    }
};
class ObjectiveRegistry
{
    std::unordered_map<std::string, std::shared_ptr<ObjectiveEvaluator>> evaluators;

  public:
    ObjectiveRegistry()
    {
        auto events = std::make_shared<EventObjective>();
        for (const auto *type : {"Interact", "ReachLocation", "TalkToNPC", "SolvePuzzle", "EscapeArea",
                                 "LosePursuit", "DefeatEnemy", "HackTerminal", "PhotographObject",
                                 "FollowNPC", "ProtectNPC", "SearchArea", "UseItem"})
            Register(type, events);
        Register("FindItem", std::make_shared<ItemObjective>());
        Register("CollectEvidence", std::make_shared<EvidenceObjective>());
        Register("SurviveTime", std::make_shared<CounterObjective>());
    }
    void Register(const std::string &type, std::shared_ptr<ObjectiveEvaluator> evaluator)
    {
        evaluators[type] = std::move(evaluator);
    }
    bool Complete(const ObjectiveDefinition &d, const ObjectiveContext &c) const
    {
        auto it = evaluators.find(d.type);
        return it != evaluators.end() && it->second && it->second->Complete(d, c);
    }
};
} // namespace Wroclaw
