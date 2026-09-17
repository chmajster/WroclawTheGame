#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ChapterDefinition.generated.h"
USTRUCT(BlueprintType)
struct FWTGActionRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText Body;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Answer;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Prerequisites;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Items;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Rewards;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Clues;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> Consumes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer Tags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer RequiredTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer GrantedTags;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool SafeOnly = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool Checkpoint = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Heat = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FText> Hints;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float HintDelay = 90;
};
UCLASS(BlueprintType)
class WROCLAWTHEGAME_API UChapterDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 SchemaVersion = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FWTGActionRecord> Actions;
    UFUNCTION(CallInEditor, BlueprintCallable) void ImportGeneratedCatalog();
    bool Apply() const;
};
