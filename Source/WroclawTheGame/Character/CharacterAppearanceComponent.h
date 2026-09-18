#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Character/CharacterAppearanceDefinition.h"
#include "CharacterAppearanceComponent.generated.h"

UCLASS(ClassGroup=(WTG), meta=(BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UCharacterAppearanceComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCharacterAppearanceComponent();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* Function) override;
    UPROPERTY(BlueprintReadOnly) FCharacterAppearanceDefinition Appearance;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> BodyMesh;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<class USkeletalMeshComponent> FaceMesh;
    UPROPERTY(BlueprintReadOnly) bool bLoading = false;
    UPROPERTY(BlueprintReadOnly) bool bUsingPlaceholder = true;
    UPROPERTY(BlueprintReadOnly) float HeightRatio = 1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bPreview = false;
    UPROPERTY(BlueprintReadWrite) FName PreviewMovement = TEXT("Idle");
    UFUNCTION(BlueprintCallable) void ApplyAppearance(const FCharacterAppearanceDefinition& Data);
    UFUNCTION(BlueprintCallable) void ReloadAppearance();
    // A rig-specific AnimBP can consume HeightRatio for stride/IK retargeting.
    UFUNCTION(BlueprintImplementableEvent) void OnAppearanceApplied(const FCharacterAppearanceDefinition& Data);
private:
    UPROPERTY() TObjectPtr<class USceneComponent> VisualRoot;
    UPROPERTY() TMap<FName,TObjectPtr<class UStaticMeshComponent>> Primitives;
    UPROPERTY() TMap<FName,TObjectPtr<class UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TArray<TObjectPtr<class UMeshComponent>> ModularParts;
    UPROPERTY() TMap<FName,TObjectPtr<class UMeshComponent>> CachedParts;
    UPROPERTY() TObjectPtr<class UCharacterAppearanceCatalog> Catalog;
    TSharedPtr<struct FStreamableHandle> LoadHandle;
    TArray<FSoftObjectPath> LoadedPaths;
    int32 RequestSerial=0;
    float Time=0;
    void Render();
    void BuildPlaceholder();
    void Primitive(FName ID,FVector Location,FVector Scale,FLinearColor Color,bool Visible=true);
};

UCLASS(ClassGroup=(WTG), meta=(BlueprintSpawnableComponent))
class WROCLAWTHEGAME_API UWardrobeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) bool Equip(EClothingSlot Slot,FName ClothingID,int32 Variant=0);
    UFUNCTION(BlueprintCallable) void GrantClothing(FName ClothingID);
    UFUNCTION(BlueprintPure) TArray<FName> OwnedClothing() const;
};
