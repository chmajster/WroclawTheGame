#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/CityDefinition.h"
#include "CityCoverageSubsystem.generated.h"

UCLASS()
class WROCLAWTHEGAME_API UCityCoverageSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
  public:
    bool bShowOverlay = false;
    static FLinearColor Color(ECityCoverageStatus Status);
    static FString Label(ECityCoverageStatus Status);
    FString ReportText() const;
};
