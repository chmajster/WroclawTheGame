#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeoMeshLibrary.generated.h"
UCLASS()
class WROCLAWTHEGAME_API UGeoMeshLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static class UStaticMesh *BakeMesh(const FString &AssetPath, const TArray<FVector> &Vertices,
                                       const TArray<int32> &Triangles);
};
