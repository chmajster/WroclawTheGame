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
    FString PhoneMapText() const;
  private:
    UPROPERTY() TObjectPtr<UCityDefinition> Definition;
};
