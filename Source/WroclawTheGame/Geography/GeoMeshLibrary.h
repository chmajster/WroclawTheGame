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
    // Editor source vertices, independent of conservative Nanite render bounds.
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static FBox GetAuthoredMeshBounds(class UStaticMesh* Mesh, int32 LodIndex = 0);
};
