#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "WorldInteraction.generated.h"
UCLASS()
class WROCLAWTHEGAME_API AWorldInteraction : public AActor, public IInteractable
{
    GENERATED_BODY()
  public:
    AWorldInteraction();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DefinitionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Kind;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UHideableComponent> Hideable;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Collider;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    virtual void BeginPlay() override;
    virtual FText Prompt(class ASliceCharacter *Player) const override;
    virtual void Interact(class ASliceCharacter *Player) override;
};
