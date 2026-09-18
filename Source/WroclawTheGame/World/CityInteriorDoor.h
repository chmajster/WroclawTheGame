#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "CityInteriorDoor.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ACityInteriorDoor : public AActor, public IInteractable
{
    GENERATED_BODY()
  public:
    ACityInteriorDoor();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Collider;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere) FString BuildingId;
    UPROPERTY(EditAnywhere) FString Label;
    UPROPERTY(EditAnywhere) FVector Destination;
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
};
