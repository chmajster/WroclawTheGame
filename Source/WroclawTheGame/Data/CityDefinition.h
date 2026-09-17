#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Info.h"
#include "CityDefinition.generated.h"

UENUM(BlueprintType)
enum class ECityCoverageStatus : uint8
{
    Missing, GISOnly, Blockout, Playable, Detailed, Final
};

USTRUCT(BlueprintType)
struct FCitySectorDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DistrictId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ArchitectureProfile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ContentDensity;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FVector2D> Boundary;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ECityCoverageStatus Status = ECityCoverageStatus::Missing;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FString> Blockers;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 BuildingCount = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RoadCount = 0;
};

UCLASS(BlueprintType)
class WROCLAWTHEGAME_API UCityDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
  public:
    UFUNCTION(CallInEditor, BlueprintCallable) bool ImportCatalog(const FString &Json);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SourceFingerprint;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCitySectorDefinition> Sectors;
};

// Hard reference keeps the catalogue in the cooked map; the registry never streams out.
UCLASS()
class WROCLAWTHEGAME_API ACityRegistry : public AInfo
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCityDefinition> Definition;
    virtual void BeginPlay() override;
};
