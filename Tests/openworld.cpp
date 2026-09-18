#include "Framework/OpenWorldState.h"
#include <cassert>
#include <iostream>
using namespace Wroclaw;
struct Never final : ObjectiveEvaluator
{
    bool Complete(const ObjectiveDefinition &, const ObjectiveContext &) const override
    {
        return false;
    }
};
int main()
{
    EventBus bus;
    int calls = 0;
    size_t sub = 0;
    sub = bus.Subscribe([&](const GameplayEvent &) {
        ++calls;
        bus.Unsubscribe(sub);
    });
    bus.Publish({"one", "a", 0});
    bus.Publish({"two", "b", 0});
    assert(calls == 1);
    Progress p;
    QuestFramework quests;
    assert(quests.State(Quests().front(), p) == QuestState::Available);
    assert(quests.State(Quests()[1], p) == QuestState::Locked);
    p.objectives.Register("extension", std::make_shared<Never>());
    ObjectiveContext context{p.completed, p.inventory, p.evidence, p.counters};
    assert(!p.objectives.Complete({"extension", "x", 1}, context));
    assert(!p.objectives.Complete({"unknown", "x", 1}, context));
    p.counters["survival"] = 20;
    assert(p.objectives.Complete({"SurviveTime", "survival", 15}, context));
    WorldState world;
    assert(world.Valid());
    world.AddHeat(300);
    assert(world.HeatLevel() == 5);
    assert(world.HeatResponse() == HeatResponseLevel::Manhunt);
    world.heat = 0;
    assert(world.HeatResponse() == HeatResponseLevel::Quiet);
    world.heat = 25;
    assert(world.HeatResponse() == HeatResponseLevel::Watch);
    world.heat = 45;
    assert(world.HeatResponse() == HeatResponseLevel::Search);
    world.heat = 65;
    assert(world.HeatResponse() == HeatResponseLevel::Pursuit);
    world.heat = 85;
    assert(world.HeatResponse() == HeatResponseLevel::Lockdown);
    world.heat = 100;
    assert(world.HeatResponse() == HeatResponseLevel::Manhunt);
    world.AddHeat(-500);
    assert(world.heat == 0);
    assert(!world.SetWeather("missing"));
    assert(!world.SetTime(-1));
    assert(world.SetWeather("Fog"));
    world.Enter(world.District(11000, 6000, 100));
    assert(world.district == "nadodrze");
    world.Tick(1, true, false);
    assert(world.heat == 15);
    world.Tick(1, true, false);
    assert(world.heat == 15);
    world.Tick(1, false, true);
    assert(world.heat < 5);
    assert(world.quiet == 1);
    assert(WorldState::Visibility(100, 1, false, 100, true, 1, false) == 0);
    assert(WorldState::Visibility(100, 1, true, 0, false, .5, false) <
           WorldState::Visibility(100, 1, false, 100, false, 1, false));
    const auto &home = Locations().back();
    assert(!world.Rest(home, false));
    for (const auto &location : Locations())
        if (location.safehouse)
        {
            world.Discover(location.id, true);
            assert(!world.Rest(location, true));
            assert(world.Rest(location, false));
        }
    for (const auto &event : WorldEvents())
        if (event.hostile)
        {
            world.heat = event.min_heat;
            world.quiet = 0;
            assert(!world.Eligible(event, p));
        }
    world.cooldowns[WorldEvents().front().id] = 90;
    world.photos.insert("car");
    auto text = world.Serialize();
    WorldState restored;
    assert(WorldState::Deserialize(text, restored));
    assert(restored.Serialize() == text);
    assert(restored.Random() == world.Random());
    assert(!WorldState::Deserialize("999 " + text, restored));
    assert(!WorldState::Deserialize(text + " trailing", restored));
    WorldState bad = world;
    bad.heat = -1;
    assert(!bad.Valid());
    bad = world;
    bad.discoveries["invented"] = 1;
    assert(!bad.Valid());
    bad = world;
    bad.cooldowns["unknown"] = 2;
    assert(!bad.Valid());
    assert(!world.Combine(Combinations().front(), p));
    assert(!p.IsMainCampaignCompleted());
    auto seed = world.seed;
    for (int i = 0; i < 1000; ++i)
    {
        world.Tick(.5, false, false);
        world.SelectEvent(p);
    }
    assert(world.Valid());
    assert(world.seed != seed);
    assert(WorldState::Simulation(100, Guards().front()) == SimulationLevel::Full);
    assert(WorldState::Simulation(100000, Guards().front()) == SimulationLevel::Dormant);
    std::cout << "PASS: event bus, objective extension, quest prerequisites, Heat, director, discovery, "
                 "weather, visibility and world save round trip\n";
}
