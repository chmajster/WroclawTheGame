#pragma once
#include "CoreMinimal.h"
#include "Core/WantedState.h"
#include "Subsystems/WorldSubsystem.h"
#include "DistrictGameplaySubsystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UDistrictGameplaySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float Dt) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDistrictGameplaySubsystem, STATGROUP_Tickables); }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override { return Type==EWorldType::Game || Type==EWorldType::PIE; }
    bool IsActive() const { return Registry != nullptr; }
    class AWTGDistrictRegistry* GetRegistry() const { return Registry; }
    void ReportCrime(float Severity, const FString& Reason);
    int32 WantedLevel() const { return Wanted.Level(); }
    bool IsSearching() const;
    void RecoverPlayer();
    Wroclaw::WantedState Wanted;
    uint32 RecoverySerial=0;
private:
    UPROPERTY() TObjectPtr<class AWTGDistrictRegistry> Registry;
    float Accumulator=0;
    double NextPoliceAttempt=0;
    bool bLoggedReady=false;
    double NextReadyDiagnostic=5;
};
