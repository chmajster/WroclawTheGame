#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SurfaceQualityLibrary.generated.h"

UCLASS()
class WROCLAWTHEGAME_API USurfaceQualityLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    // Editor build gate: wait for shaders and return all SM6 errors, never accept the default fallback.
    UFUNCTION(BlueprintCallable, Category = "Surface Quality")
    static TArray<FString> ValidateSurfaceShaders(const TArray<class UMaterialInterface *> &Materials);
};
