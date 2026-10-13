#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LocomotionAssetLibrary.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ULocomotionAssetLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable) static class USkeleton* CreateCivilianSkeleton(class USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable) static class UPhysicsAsset* CreateCivilianPhysics(class USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable) static bool ConfigureCivilianLodSettings(class USkeletalMesh* Mesh,class USkeleton* Skeleton);
    UFUNCTION(BlueprintCallable) static bool RetargetCivilianAnimation(class UAnimationAsset* Animation,class USkeletalMesh* Mesh,class UAnimationAsset* Source);
    // Editor-only measurement of the in-place stance foot's backward velocity.
    UFUNCTION(BlueprintCallable) static float MeasureCivilianStride(class UAnimSequence* Animation,class USkeletalMesh* Mesh);
    UFUNCTION(BlueprintCallable) static bool LinkCompatibleRig(class USkeletalMesh* Mesh,class UAnimSequence* Animation);
    UFUNCTION(BlueprintCallable) static class UBlendSpace1D* BakeLocomotion(const FString& Path,class USkeletalMesh* Mesh,class UAnimSequence* Idle,class UAnimSequence* Walk,class UAnimSequence* Jog,class UAnimSequence* Sprint);
};
