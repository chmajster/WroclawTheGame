#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NoiseSystem.generated.h"
class APawn;
UCLASS()
class WROCLAWTHEGAME_API UNoiseSystem : public UWorldSubsystem
{
    GENERATED_BODY()
  public:
    bool bDebug = false;
    void Report(FName Type, const FVector &Location, APawn *Source);
};
