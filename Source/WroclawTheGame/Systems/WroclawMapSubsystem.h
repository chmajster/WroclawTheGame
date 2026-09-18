#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/CityDefinition.h"
#include "WroclawMapSubsystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UWroclawMapSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
  public:
    void RegisterCity(UCityDefinition *City);
    const UCityDefinition *GetCity() const { return Definition; }
    const FCitySectorDefinition *SectorAt(const FVector &Position) const;

    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Map")
    bool SetWaypoint(FName SectorId);
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Map")
    bool CycleWaypoint();
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Map")
    void ClearWaypoint() { WaypointSector = NAME_None; }
    UFUNCTION(BlueprintPure, Category = "Wroclaw|Map")
    FName GetWaypoint() const { return WaypointSector; }

    FString PhoneMapText() const;

  private:
    const FCitySectorDefinition *FindSector(FName SectorId) const;
    UPROPERTY() TObjectPtr<UCityDefinition> Definition;
    UPROPERTY() FName WaypointSector;
};
