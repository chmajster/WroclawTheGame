#pragma once
#include "Core/SliceProgress.h"
#include <functional>
#include <memory>
#include <unordered_map>
namespace Wroclaw
{
// Subscribers can unregister during dispatch. New subscribers start on the next event.
struct GameplayEvent
{
    std::string type, subject;
    double value = 0;
};
class EventBus
{
    size_t next = 1;
    std::map<size_t, std::function<void(const GameplayEvent &)>> listeners;

  public:
    size_t Subscribe(std::function<void(const GameplayEvent &)> callback)
    {
        auto id = next++;
        listeners.emplace(id, std::move(callback));
        return id;
    }
    void Unsubscribe(size_t id)
    {
        listeners.erase(id);
    }
    void Publish(const GameplayEvent &event)
    {
        auto snapshot = listeners;
        for (const auto &entry : snapshot)
            if (listeners.count(entry.first))
                entry.second(event);
    }
};
enum class QuestState
{
    Locked,
    Available,
    Active,
    Completed,
    Failed,
    Suspended
};
enum class PuzzleState
{
    Inactive,
    Available,
    InProgress,
    Solved,
    Failed
};
class QuestFramework
{
    const QuestDef *Find(const std::string &id) const
    {
        for (const auto &q : Quests())
            if (q.id == id)
                return &q;
        for (const auto &q : SideQuests())
            if (q.id == id)
                return &q;
        return nullptr;
    }

  public:
    bool Complete(const QuestDef &q, const Progress &p) const
    {
        return p.QuestComplete(q);
    }
    QuestState State(const QuestDef &q, const Progress &p) const
    {
        if (Complete(q, p))
            return QuestState::Completed;
        if (p.Tagged("Quest." + q.id + ".Failed"))
            return QuestState::Failed;
        if (p.Tagged("Quest." + q.id + ".Suspended"))
            return QuestState::Suspended;
        for (const auto &id : q.prerequisites)
        {
            const auto *parent = Find(id);
            if (!parent || !Complete(*parent, p))
                return QuestState::Locked;
        }
        bool started = p.Tagged("Quest." + q.id + ".Active");
        for (const auto &id : q.all)
            started |= p.Done(id);
        for (const auto &id : q.any)
            started |= p.Done(id);
        return started ? QuestState::Active : QuestState::Available;
    }
};
struct PuzzleFramework
{
    static PuzzleState State(const ActionDef &d, const Progress &p)
    {
        if (p.Done(d.id))
            return PuzzleState::Solved;
        if (!p.CanDo(d))
            return PuzzleState::Inactive;
        auto it = p.locks.find(d.id);
        if (it != p.locks.end())
        {
            if (it->second.until > p.elapsed)
                return PuzzleState::Failed;
            if (it->second.failures)
                return PuzzleState::InProgress;
        }
        return PuzzleState::Available;
    }
    static std::string Hint(const ActionDef &d, const Progress &p)
    {
        double age = p.elapsed - p.questSince;
        int level = age >= d.hintDelay * 3 ? 2 : (age >= d.hintDelay * 2 ? 1 : (age >= d.hintDelay ? 0 : -1));
        return level >= 0 && level < static_cast<int>(d.hints.size()) ? d.hints[level] : "";
    }
};
} // namespace Wroclaw
