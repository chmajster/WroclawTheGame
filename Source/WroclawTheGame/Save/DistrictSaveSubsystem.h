#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DistrictSaveSubsystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UDistrictSaveSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Tick(float Dt) override;
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDistrictSaveSubsystem,STATGROUP_Tickables); }
    virtual bool DoesSupportWorldType(EWorldType::Type Type) const override { return Type==EWorldType::Game || Type==EWorldType::PIE; }
    bool Save(bool Notify=true);
    bool LoadSaved();
    bool NewRun();
    bool HasSave() const;
    bool IsWriteBlocked() const {return bWriteBlocked;}
    FString Status=TEXT("Brak zapisu");
    FString Slot=TEXT("WroclawDistrict_v1");
private:
    bool bTriedAutoLoad=false,bWriteBlocked=false;
    float AutoSaveSeconds=0;
    class UDistrictSaveGame* Capture() const;
    bool Apply(const class UDistrictSaveGame* Data);
    void Report(const FString& Message);
};
