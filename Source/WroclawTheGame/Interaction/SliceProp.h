#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SliceProp.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceProp : public AActor, public IInteractable {
 GENERATED_BODY()
public:
 ASliceProp();
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY() FString ActionId;
 void Configure(const FString& Id,const FVector& Size);
 virtual FText Prompt(ASliceCharacter* Player) const override;
 virtual void Interact(ASliceCharacter* Player) override;
 virtual void Tick(float DeltaSeconds) override;
private:
 bool bOpened=false;
 FVector ClosedPosition;
};
