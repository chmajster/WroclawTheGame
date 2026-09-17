#include "Framework/RaceProgress.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace Wroclaw;
int main()
{
    const std::vector<RoadPoint> gates={{1000,0,0},{2000,0,0},{3000,0,0}};
    RaceProgress race;
    assert(race.Start(gates,{0,0,0},60));
    race.Tick(1,{2200,0,0});assert(race.next==2); // swept segment catches fast crossings
    race.Tick(1,{3100,0,0});assert(race.status==RaceStatus::Finished);
    assert(race.Start(gates,{0,0,0},60));race.Tick(1,{0,900,0});race.Tick(1,{3000,900,0});
    assert(race.next==0);race.Tick(1,{3000,0,0});assert(race.next==0); // cannot skip gates
    assert(race.Start(gates,{0,0,0},60));race.previous={1400,0,0};race.Tick(1,{900,0,0});assert(race.next==0); // wrong direction
    assert(race.Start(gates,{0,0,0},1));race.Tick(2,{3000,0,0});assert(race.status==RaceStatus::Failed);
    assert(race.Start(gates,{0,0,0},60,true));race.Tick(1,{900,0,0},100);assert(race.status==RaceStatus::Failed);
    assert(race.Start(gates,{0,0,0},60,true));assert(race.elapsed==0 && race.next==0 && race.cargo==100);
    race.Tick(1,{1100,0,0},30);assert(race.cargo==70 && race.next==1);
    race.Tick(1,{std::numeric_limits<double>::quiet_NaN(),0,0});assert(race.status==RaceStatus::Failed);
    std::cout<<"PASS: race order, direction, swept gates, timeout, delivery damage and restart\n";
}
