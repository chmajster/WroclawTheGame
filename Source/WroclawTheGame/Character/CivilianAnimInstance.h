#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "CivilianAnimInstance.generated.h"

// Keeps the project's existing clips/blendspace and modular leader pose. All
// world queries run on the game thread; the proxy evaluates immutable inputs.
UCLASS(Transient, Blueprintable)
class WROCLAWTHEGAME_API UCivilianAnimInstance : public UAnimSingleNodeInstance
{
    GENERATED_BODY()
public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    UPROPERTY(BlueprintReadOnly) float GroundSpeed=0;
    UPROPERTY(BlueprintReadOnly) FName MovementState=TEXT("Idle");
    UPROPERTY(BlueprintReadOnly) bool bGrounded=false;
    UPROPERTY(BlueprintReadOnly) FVector LeftGround=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector RightGround=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector LeftNormal=FVector::UpVector;
    UPROPERTY(BlueprintReadOnly) FVector RightNormal=FVector::UpVector;
    UPROPERTY(BlueprintReadOnly) bool bLeftContact=false;
    UPROPERTY(BlueprintReadOnly) bool bRightContact=false;
    UPROPERTY(BlueprintReadOnly) FVector2D FootOffsets=FVector2D::ZeroVector;
    float FloorZ=0;
    int32 TransitionSerial=0;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};
