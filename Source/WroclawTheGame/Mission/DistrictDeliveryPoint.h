#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "DistrictDeliveryPoint.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ADistrictDeliveryPoint : public AActor,public IInteractable
{
    GENERATED_BODY()
public:
    ADistrictDeliveryPoint();
    virtual FText Prompt(class ASliceCharacter* Player) const override;
    virtual void Interact(class ASliceCharacter* Player) override;
    void Initialize(bool IsReceiver,class USkeletalMesh* Mesh,class UAnimationAsset* Idle);
private:
    UPROPERTY() TObjectPtr<class UCapsuleComponent> Capsule;
    UPROPERTY() TObjectPtr<class USkeletalMeshComponent> Visual;
    UPROPERTY() TObjectPtr<class UTextRenderComponent> Name;
    bool bReceiver=false;
};
