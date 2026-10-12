#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace Wroclaw
{
enum class DeliveryStage : uint8_t { Meet, GetVehicle, Drive, Deliver, Complete, Failed };
struct DeliveryState
{
    DeliveryStage Stage=DeliveryStage::Meet;
    float Remaining=420;
    uint32_t Attempts=0;
    bool Active() const { return Stage==DeliveryStage::GetVehicle || Stage==DeliveryStage::Drive || Stage==DeliveryStage::Deliver; }
    bool Start(float Limit)
    {
        if (Stage!=DeliveryStage::Meet && Stage!=DeliveryStage::Failed) return false;
        if (!std::isfinite(Limit) || Limit<=0) return false;
        Remaining=Limit;++Attempts;Stage=DeliveryStage::GetVehicle;return true;
    }
    void EnterCar() { if (Stage==DeliveryStage::GetVehicle) Stage=DeliveryStage::Drive; }
    void Tick(float Dt,bool Dead,bool LostVehicle,bool Arrived,bool Stopped)
    {
        if (!Active()) return;
        if (std::isfinite(Dt) && Dt>0) Remaining=std::max(0.f,Remaining-Dt);
        if (Dead || LostVehicle || Remaining<=0) {Stage=DeliveryStage::Failed;return;}
        if (Stage==DeliveryStage::Drive && Arrived && Stopped) Stage=DeliveryStage::Deliver;
        if (Stage==DeliveryStage::Deliver && !Arrived) Stage=DeliveryStage::Drive;
    }
    bool CanDeliver(bool OnFoot,bool PlayerNear,bool CarNear,bool Stopped,int Wanted) const
    { return Stage==DeliveryStage::Deliver && OnFoot && PlayerNear && CarNear && Stopped && Wanted==0; }
    bool Complete(bool RewardApplied)
    {
        if (Stage!=DeliveryStage::Deliver || !RewardApplied) return false;
        Stage=DeliveryStage::Complete;return true;
    }
};
}
