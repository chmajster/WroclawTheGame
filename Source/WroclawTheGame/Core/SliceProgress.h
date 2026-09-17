#pragma once
#include "Content/ChapterCatalog.h"
#include "Framework/Objectives.h"
#include "Content/WorldCatalog.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
namespace Wroclaw
{
enum class Result
{
    Applied,
    AlreadyDone,
    Locked,
    MissingClue,
    Wrong,
    Cooldown,
    Invalid,
    Unsafe
};
enum Achievement : uint32_t
{
    FirstSteps = 1,
    EscapeArtist = 2,
    Unseen = 4,
    Detective = 8,
    Pacifist = 16,
    QuickThinking = 32
};
struct LockState
{
    int failures = 0;
    double until = 0;
};
struct Progress
{
    static constexpr int Version = 3;
    int variant = 0, kills = 0, medkitsUsed = 0, distractionsUsed = 0;
    double elapsed = 0, questSince = 0;
    bool courtyardDetected = false, chaseStarted = false, garageUnderThreat = false;
    std::vector<std::string> history, timeline;
    std::set<std::string> completed, evidence, neutralized, tags;
    std::map<std::string, int> inventory, used;
    std::map<std::string, LockState> locks;
    std::map<std::string, double> counters;
    ObjectiveRegistry objectives;
    bool QuestComplete(const QuestDef &q) const
    {
        ObjectiveContext context{completed, inventory, evidence, counters};
        for (const auto &o : q.objectives)
            if (!objectives.Complete(o, context))
                return false;
        return Any(q.any);
    }
    explicit Progress(int Variant = 0) : variant(Variant) {}
    static const ActionDef *Find(const std::string &id)
    {
        for (const auto &a : Catalog())
            if (a.id == id)
                return &a;
        return nullptr;
    }
    bool Done(const std::string &id) const
    {
        return completed.count(id) != 0;
    }
    bool Has(const std::string &id) const
    {
        auto i = inventory.find(id);
        return i != inventory.end() && i->second > 0;
    }
    bool All(const std::vector<std::string> &ids) const
    {
        for (const auto &id : ids)
            if (!Done(id))
                return false;
        return true;
    }
    bool Any(const std::vector<std::string> &ids) const
    {
        for (const auto &id : ids)
            if (Done(id))
                return true;
        return ids.empty();
    }
    int Current() const
    {
        int i = 0;
        for (const auto &q : Quests())
        {
            if (!QuestComplete(q))
                return i;
            ++i;
        }
        return i;
    }
    bool Tagged(const std::string &tag) const
    {
        return tags.count(tag) != 0;
    }
    bool Finished() const
    {
        return Tagged(ChapterCompleteTag) && Current() == static_cast<int>(Quests().size());
    }
    bool IsMainCampaignCompleted() const
    {
        for (const auto &tag : CampaignChapters)
            if (!Tagged(tag))
                return false;
        return !CampaignChapters.empty();
    }
    static const ItemDef *Item(const std::string &id)
    {
        for (const auto &item : Items())
            if (item.id == id)
                return &item;
        return nullptr;
    }
    bool Chapter2Unlocked() const
    {
        return Finished();
    }
    int HintLevel() const
    {
        double age = elapsed - questSince;
        return age >= 300 ? 3 : (age >= 180 ? 2 : (age >= 90 ? 1 : 0));
    }
    void Tick(double dt)
    {
        if (std::isfinite(dt) && dt > 0 && dt < 5 && !Finished())
            elapsed += dt;
    }
    bool CanDo(const ActionDef &a) const
    {
        if (!All(a.prerequisites) || !Any(a.anyOf))
            return false;
        for (const auto &tag : a.requireTags)
            if (!Tagged(tag))
                return false;
        for (const auto &item : a.items)
            if (!Has(item))
                return false;
        if (!a.choice.empty())
            for (const auto &other : Catalog())
                if (other.choice == a.choice && other.id != a.id && Done(other.id))
                    return false;
        return true;
    }
    std::string Answer(const ActionDef &a) const
    {
        if (a.answer == "@variant")
            return variant >= 0 && variant < static_cast<int>(LightVariants().size())
                       ? LightVariants()[variant]
                       : "";
        return a.answer;
    }
    Result Apply(const std::string &id, bool threat = false)
    {
        const auto *a = Find(id);
        if (!a)
            return Result::Invalid;
        if (Done(id))
            return Result::AlreadyDone;
        if (Finished())
            return Result::Locked;
        if (!CanDo(*a))
            return Result::Locked;
        if (!All(a->clues))
            return Result::MissingClue;
        if (a->safe && threat)
            return Result::Unsafe;
        for (const auto &item : a->consume)
            if (!Has(item))
                return Result::Locked;
        const int previous = Current();
        completed.insert(id);
        history.push_back(id);
        timeline.push_back("action:" + id);
        if (!a->evidence.empty())
            evidence.insert(a->evidence);
        for (const auto &item : a->reward)
        {
            const auto *definition = Item(item);
            if (definition)
                inventory[item] = std::min(definition->stack, inventory[item] + 1);
        }
        for (const auto &item : a->consume)
            --inventory[item];
        tags.insert(a->setTags.begin(), a->setTags.end());
        chaseStarted = Tagged("State.World.ChaseStarted");
        if (threat && std::find(a->tags.begin(), a->tags.end(), "Achievement.UnderThreat") != a->tags.end())
            garageUnderThreat = true;
        if (Current() != previous)
            questSince = elapsed;
        return Result::Applied;
    }
    Result Submit(const std::string &id, const std::string &input, bool threat = false)
    {
        const auto *a = Find(id);
        if (!a || a->answer.empty())
            return Result::Invalid;
        if (Done(id))
            return Result::AlreadyDone;
        if (Finished())
            return Result::Locked;
        if (!CanDo(*a))
            return Result::Locked;
        if (!All(a->clues))
            return Result::MissingClue;
        auto &lock = locks[id];
        if (elapsed < lock.until)
            return Result::Cooldown;
        if (input != Answer(*a))
        {
            if (++lock.failures >= 3)
            {
                lock.failures = 0;
                lock.until = elapsed + 8;
            }
            return Result::Wrong;
        }
        const auto result = Apply(id, threat);
        if (result == Result::Applied)
        {
            lock.failures = 0;
            lock.until = 0;
        }
        return result;
    }
    bool Use(const std::string &id)
    {
        const auto *item = Item(id);
        if (!item || !item->usable || !Has(id))
            return false;
        --inventory[id];
        ++used[id];
        timeline.push_back("use:" + id);
        medkitsUsed = used.count("medkit") ? used.at("medkit") : 0;
        distractionsUsed = used.count("distraction") ? used.at("distraction") : 0;
        return true;
    }
    uint32_t Achievements() const
    {
        uint32_t value = Tagged("Achievement.FirstSteps") ? FirstSteps : 0u;
        if (Tagged("Achievement.EscapeArtist"))
            value |= EscapeArtist;
        if (Tagged("Achievement.CourtyardPassed") && !courtyardDetected)
            value |= Unseen;
        bool allEvidence = true;
        for (const auto &a : Catalog())
            if (!a.evidence.empty() && !evidence.count(a.evidence))
                allEvidence = false;
        if (allEvidence)
            value |= Detective;
        if (Finished() && kills == 0)
            value |= Pacifist;
        if (garageUnderThreat)
            value |= QuickThinking;
        return value;
    }
    static bool ReplayTimeline(const std::vector<std::string> &events, Progress &out, bool underThreat)
    {
        if (events.size() > Catalog().size() + 1000)
            return false;
        for (const auto &event : events)
        {
            if (event.rfind("use:", 0) == 0)
            {
                if (!out.Use(event.substr(4)))
                    return false;
                continue;
            }
            if (event.rfind("action:", 0) != 0)
                return false;
            const auto *action = Find(event.substr(7));
            if (!action)
                return false;
            const bool threat = underThreat && std::find(action->tags.begin(), action->tags.end(),
                                                         "Achievement.UnderThreat") != action->tags.end();
            if (out.Apply(action->id, threat) != Result::Applied)
                return false;
        }
        return true;
    }
    // Replays stable event IDs and derives inventory/evidence. Rejects inconsistent or future snapshots.
    bool Valid() const
    {
        if (variant < 0 || variant >= static_cast<int>(LightVariants().size()) || kills < 0 || kills > 1000 ||
            medkitsUsed < 0 || distractionsUsed < 0 || !std::isfinite(elapsed) || elapsed < 0 ||
            !std::isfinite(questSince) || questSince < 0 || questSince > elapsed ||
            history.size() > Catalog().size() || timeline.size() > Catalog().size() + 1000)
            return false;
        for (const auto &id : neutralized)
            if (std::none_of(Guards().begin(), Guards().end(), [&](const auto &g) { return g.id == id; }))
                return false;
        for (const auto &pair : counters)
            if (pair.first.empty() || !std::isfinite(pair.second) || pair.second < 0)
                return false;
        Progress replay(variant);
        replay.counters = counters;
        if (!ReplayTimeline(timeline, replay, garageUnderThreat))
            return false;
        if (replay.history != history || replay.used != used)
            return false;
        if (replay.medkitsUsed != medkitsUsed || replay.distractionsUsed != distractionsUsed)
            return false;
        if (replay.completed != completed || replay.inventory != inventory || replay.evidence != evidence ||
            replay.tags != tags || replay.chaseStarted != chaseStarted ||
            replay.garageUnderThreat != garageUnderThreat)
            return false;
        for (const auto &pair : locks)
        {
            const auto *a = Find(pair.first);
            if (!a || a->answer.empty() || pair.second.failures < 0 || pair.second.failures > 2 ||
                !std::isfinite(pair.second.until) || pair.second.until < 0 ||
                pair.second.until > elapsed + 8.01)
                return false;
        }
        return true;
    }
};
} // namespace Wroclaw
