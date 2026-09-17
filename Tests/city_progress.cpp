#include "Framework/CityProgress.h"
#include <cassert>
#include <iostream>
using Wroclaw::CityProgress;
int main()
{
    CityProgress state;
    assert(!state.Step("city.huby.finish",0,true,false,false,true));
    for (const auto &action:Wroclaw::CityActions())
    {
        if (action.kind=="finish") continue;
        assert(!state.Step(action.id,1,false,true,true,true));
        if(action.vehicle) assert(!state.Step(action.id,30,true,false,true,true));
        if(action.crouch) assert(!state.Step(action.id,0,true,false,false,true));
        bool applied=false;
        for(int i=0;i<21;++i) applied=state.Step(action.id,1,true,true,true,true)||applied;
        assert(applied); assert(!state.Step(action.id,1,true,true,true,true));
    }
    for(const auto &action:Wroclaw::CityActions())
        if(action.kind=="finish")assert(state.Step(action.id,0,true,false,false,true));
    assert(state.completed.size()==Wroclaw::CityActions().size());
    state.completed.insert("city.future-unknown.secret");
    CityProgress loaded;
    assert(CityProgress::Deserialize(state.Serialize(),loaded));
    assert(loaded.completed==state.completed && loaded.timers.empty());
    const auto snapshot=loaded.Serialize();
    for(const std::string bad:{"WTGCITY9\n0\n","WTGCITY1\n2\none\none\n","WTGCITY1\n1\n../outside\n","WTGCITY1\n10001\n","WTGCITY1\n0\nextra"})
    {assert(!CityProgress::Deserialize(bad,loaded));assert(loaded.Serialize()==snapshot);}
    CityProgress observation;
    for(int i=0;i<19;++i)assert(!observation.Step("city.gaj.clue1",1,true,true,false,false));
    assert(!observation.Step("city.gaj.clue1",1,false,true,false,false));
    for(int i=0;i<19;++i)assert(!observation.Step("city.gaj.clue1",1,true,true,false,false));
    assert(observation.Step("city.gaj.clue1",1,true,true,false,false));
    std::cout<<"PASS: all city activities, prerequisites, continuous observation, idempotence and forward-compatible bounded saves\n";
}
