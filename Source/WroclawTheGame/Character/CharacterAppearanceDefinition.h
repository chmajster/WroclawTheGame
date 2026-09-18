#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterAppearanceDefinition.generated.h"

UENUM(BlueprintType)
enum class EAppearanceCategory : uint8 { Character, Face, Eyes, Nose, Mouth, Jaw, Ears, Skin, Hair, Beard, Body, Clothing, Voice };
UENUM(BlueprintType)
enum class EAppearanceSex : uint8 { Male, Female };
UENUM(BlueprintType)
enum class EClothingSlot : uint8 { Outerwear, Top, Bottom, Shoes, Head, Accessory, Back };

// Only stable IDs and values are serialized. Asset references belong to the catalog.
USTRUCT(BlueprintType)
struct FCharacterAppearanceDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EAppearanceSex Sex = EAppearanceSex::Male;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName PresetID = TEXT("Preset01");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FString Name = TEXT("Alex");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 RandomSeed = 12345;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float Height = 180;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float VisualAge = 27;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName BodyPreset = TEXT("Male.Standard");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName BodyBuild = TEXT("Average");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName FacePreset = TEXT("Face01");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<FName, float> FaceMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 SkinTone = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float SkinRoughness = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float Freckles = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) float SkinImperfections = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 EyeColor = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName EyeShape = TEXT("Natural");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName HairStyle = TEXT("Short");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 HairColor = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName BeardStyle = TEXT("Stubble");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 BeardColor = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName EyebrowStyle = TEXT("Natural");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 EyebrowColor = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<EClothingSlot, FName> Clothing;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TMap<EClothingSlot, int32> ClothingVariants;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName VoiceProfileID = TEXT("Voice01");
};

USTRUCT(BlueprintType)
struct FCharacterCustomizationSaveData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 CharacterAppearanceVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FCharacterAppearanceDefinition PlayerAppearanceData;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) TArray<FName> OwnedClothing;
};

USTRUCT(BlueprintType)
struct FFaceMorphDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MorphID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MorphTargetName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinValue = -.35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MaxValue = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DefaultValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EAppearanceCategory Category = EAppearanceCategory::Face;
};

USTRUCT(BlueprintType)
struct FClothingVariantDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor Color = FLinearColor(.18f,.22f,.28f);
};

// Used by HairStyleDefinition, BeardStyleDefinition and ClothingDefinition arrays.
USTRUCT(BlueprintType)
struct FAppearancePartDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> ClothingCompatibilityTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UStaticMesh> StaticMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UMaterialInterface> Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Socket = TEXT("head");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform Offset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 LODProfile = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool ColorSupport = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPlaceholder = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EClothingSlot Slot = EClothingSlot::Top;
    // Material slot names on the base body. Use prepared segmented bodies for section hiding.
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> HiddenBodyRegions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FClothingVariantDefinition> Variants;
};

USTRUCT(BlueprintType)
struct FBodyPresetDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EAppearanceSex Sex = EAppearanceSex::Male;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName CompatibilityTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class USkeletalMesh> FaceMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<class UAnimInstance> AnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<class UAnimInstance> FaceAnimationClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UAnimationAsset> CreatorIdle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UAnimationAsset> Walk;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UAnimationAsset> Jog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UAnimationAsset> Crouch;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class UMaterialInterface> SkinMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float ReferenceHeight = 180;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator MeshRotation = FRotator(0,-90,0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bPlaceholder = true;
};

USTRUCT(BlueprintType)
struct FVoiceProfileDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName ID;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<class USoundBase> Preview;
};

UCLASS(BlueprintType)
class WROCLAWTHEGAME_API UCharacterAppearanceCatalog : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FBodyPresetDefinition> Bodies;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FFaceMorphDefinition> Morphs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FAppearancePartDefinition> HairStyleDefinitions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FAppearancePartDefinition> BeardStyleDefinitions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FAppearancePartDefinition> EyebrowStyleDefinitions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FAppearancePartDefinition> ClothingDefinitions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FCharacterAppearanceDefinition> Presets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FVoiceProfileDefinition> VoiceProfiles;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> FacePresets;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> EyeShapes;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FLinearColor> SkinPalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FLinearColor> HairPalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FLinearColor> EyePalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FCharacterAppearanceDefinition FallbackDefinition;
    UFUNCTION(BlueprintCallable) void BuildFallbackCatalog();
    UFUNCTION(BlueprintCallable) void ApplyModernHeroProfile();
    const FBodyPresetDefinition* Body(FName ID) const;
    const FAppearancePartDefinition* Part(const TArray<FAppearancePartDefinition>& Parts, FName ID) const;
    bool Compatible(const FAppearancePartDefinition& Part, const FCharacterAppearanceDefinition& Appearance) const;
};
