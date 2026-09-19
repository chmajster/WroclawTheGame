#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "CharacterCreatorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAppearanceChanged);
UCLASS()
class WROCLAWTHEGAME_API UCharacterCreatorValidator : public UObject
{
    GENERATED_BODY()
public:
    static FCharacterAppearanceDefinition Normalize(const UCharacterAppearanceCatalog& C, FCharacterAppearanceDefinition A);
    UFUNCTION(BlueprintCallable) static TArray<FString> ValidateCatalog(UCharacterAppearanceCatalog* Catalog, bool CheckAssets = true);
};

USTRUCT()
struct FAppearanceChange
{
    GENERATED_BODY()
    UPROPERTY() FCharacterAppearanceDefinition Before;
    UPROPERTY() FCharacterAppearanceDefinition After;
};

UCLASS()
class WROCLAWTHEGAME_API UCharacterCreatorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UCharacterAppearanceCatalog> Catalog;
    UPROPERTY(BlueprintReadOnly) FCharacterAppearanceDefinition Draft;
    UPROPERTY(BlueprintReadOnly) FCharacterCustomizationSaveData Committed;
    UPROPERTY(BlueprintReadOnly) bool bEditing = false;
    UPROPERTY(BlueprintReadOnly) bool bSummary = false;
    UPROPERTY(BlueprintAssignable) FOnAppearanceChanged OnChanged;
    UFUNCTION(BlueprintCallable) void BeginCreation();
    UFUNCTION(BlueprintCallable) void FinishCreation();
    UFUNCTION(BlueprintCallable) void Cancel();
    UFUNCTION(BlueprintCallable) void Change(const FCharacterAppearanceDefinition& Appearance);
    UFUNCTION(BlueprintCallable) void Undo();
    UFUNCTION(BlueprintCallable) void Redo();
    UFUNCTION(BlueprintCallable) void ResetCategory(EAppearanceCategory Category);
    UFUNCTION(BlueprintCallable) void ResetAll();
    UFUNCTION(BlueprintCallable) void SelectPreset(int32 Index);
    UFUNCTION(BlueprintCallable) void Randomize(int32 Seed);
    UFUNCTION(BlueprintCallable) FCharacterAppearanceDefinition GenerateRandomNPCAppearance(int32 Seed) const;
    UFUNCTION(BlueprintCallable) void Restore(const FCharacterCustomizationSaveData& Data);
    UFUNCTION(BlueprintCallable) FCharacterCustomizationSaveData MakeSaveData() const;
    UFUNCTION(BlueprintPure) FString DebugText() const;
    bool CanUndo() const { return Cursor>0; }
    bool CanRedo() const { return Cursor<History.Num(); }
private:
    UPROPERTY() TArray<FAppearanceChange> History;
    int32 Cursor = 0;
};
