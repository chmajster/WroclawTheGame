#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "CampaignMigrationDefinition.generated.h"

USTRUCT(BlueprintType)
struct FCampaignMigrationZone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SourceMinX = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SourceMaxX = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector SourceAnchor = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector TargetAnchor = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float TargetYaw = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString BuildingId;
};

UCLASS(BlueprintType)
class WROCLAWTHEGAME_API UCampaignMigrationDefinition : public UDataAsset
{
    GENERATED_BODY()

  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString SourceSpace = TEXT("BlockoutV1");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString TargetSpace = TEXT("WroclawGISV1");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FCampaignMigrationZone> Zones;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="Wroclaw|Campaign Migration")
    bool ImportJson(const FString &JsonText);

    bool TransformLegacyPosition(const FVector &Source, FVector &Target) const;
    bool TransformLegacyTransform(const FTransform &Source, FTransform &Target) const;
};

UCLASS()
class WROCLAWTHEGAME_API ACampaignMigrationRegistry : public AActor
{
    GENERATED_BODY()
  public:
    ACampaignMigrationRegistry()
    {
        PrimaryActorTick.bCanEverTick = false;
    }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wroclaw|Campaign Migration")
    TObjectPtr<UCampaignMigrationDefinition> Definition;
};
