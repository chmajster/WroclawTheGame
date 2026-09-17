#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WorldValidationLibrary.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UWorldValidationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable) static bool ValidatePartition(UWorld *World);
};
