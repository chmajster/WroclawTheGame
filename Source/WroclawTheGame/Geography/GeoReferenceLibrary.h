#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeoReferenceLibrary.generated.h"
class AGeoReferencingSystem;
UCLASS()
class WROCLAWTHEGAME_API UGeoReferenceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    // Geographic vectors use longitude, latitude, EGM96 orthometric height.
    // Only horizontal coordinates pass through the ellipsoidal CRS conversion.
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static FVector GeoToWorld(AGeoReferencingSystem *System, FVector LongitudeLatitudeHeight);
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static FVector WorldToGeo(AGeoReferencingSystem *System, FVector World);
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static FVector LatLonToWorld(AGeoReferencingSystem *System, double Latitude, double Longitude,
                                 double OrthometricHeight);
    UFUNCTION(BlueprintCallable, Category = "Wroclaw|Geography")
    static FVector ProjectedToWorld(AGeoReferencingSystem *System, FVector EastingNorthingHeight);
};
