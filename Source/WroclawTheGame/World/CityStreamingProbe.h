#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CityStreamingProbe.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ACityStreamingProbe : public AActor
{
    GENERATED_BODY()
  public:
    ACityStreamingProbe();
    UPROPERTY() TObjectPtr<class UWorldPartitionStreamingSourceComponent> Source;
};
