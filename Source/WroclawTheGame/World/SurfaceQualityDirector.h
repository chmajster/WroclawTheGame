#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SurfaceQualityDirector.generated.h"

// Per-world material migration, lighting and weather. Never writes player saves or authored maps.
UCLASS()
class WROCLAWTHEGAME_API ASurfaceQualityDirector : public AActor
{
    GENERATED_BODY()
  public:
    ASurfaceQualityDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintCallable) void SetSurfaceWetness(float Value);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ReplacedSlots = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) float Wetness = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool FollowWeather = true;

  private:
    void ApplyActor(AActor *Actor);
    void ActorSpawned(AActor *Actor);
    void LevelAdded(class ULevel *Level, UWorld *World);
    void AddRoadPatch(class UMeshComponent *Mesh);
    UPROPERTY() TMap<FName, TObjectPtr<class UMaterialInstanceDynamic>> Surfaces;
    UPROPERTY() TArray<TObjectPtr<class UMaterialInterface>> PatchMaterials;
    UPROPERTY() TObjectPtr<class APostProcessVolume> Exposure;
    TArray<TWeakObjectPtr<class UDecalComponent>> Decals;
    FDelegateHandle SpawnHandle;
    FDelegateHandle LevelHandle;
};
