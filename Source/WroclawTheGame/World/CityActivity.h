#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "CityActivity.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ACityActivity : public AActor, public IInteractable
{
    GENERATED_BODY()
  public:
    ACityActivity();
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Collider;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere) FString ActionId;
    UPROPERTY(EditAnywhere) FString BuildingId;
    virtual void BeginPlay() override;
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
};
