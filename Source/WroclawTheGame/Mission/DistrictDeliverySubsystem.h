#pragma once
#include "CoreMinimal.h"
#include "Core/DeliveryState.h"
#include "Subsystems/WorldSubsystem.h"
#include "DistrictDeliverySubsystem.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UDistrictDeliverySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float Dt) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDistrictDeliverySubsystem, STATGROUP_Tickables); }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override { return Type==EWorldType::Game || Type==EWorldType::PIE; }
    void Interact(bool Receiver,class ASliceCharacter* Player);
    bool Restart();
    FString Objective() const;
    FVector Target() const;
    FString FailureReason;
    Wroclaw::DeliveryState State;
    static const FString RewardId;
private:
    UPROPERTY() TObjectPtr<class AWTGDistrictRegistry> Registry;
    UPROPERTY() TObjectPtr<class ADistrictDeliveryPoint> Giver;
    UPROPERTY() TObjectPtr<class ADistrictDeliveryPoint> Receiver;
    uint32 RecoveryAtStart=0;
    void CreatePoints();
};
