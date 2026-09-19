#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "SliceProp.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceProp : public AActor, public IInteractable
{
    GENERATED_BODY()
  public:
    ASliceProp();
    // Keep the legacy serialized property schema stable. Przebudzenie_Source was
    // authored before Collider existed and Launch On cooks with unversioned property
    // serialization. Collider must stay reflected for GC/editor visibility, but it
    // must never participate in the persisted actor-property stream.
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ActionId;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UDoorComponent> Door;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBasePuzzleComponent> Puzzle;
    UPROPERTY(VisibleAnywhere, SkipSerialization) TObjectPtr<class UBoxComponent> Collider;
    virtual void BeginPlay() override;
    void Configure(const FString &Id, const FVector &Size);
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
    virtual void Tick(float DeltaSeconds) override;

  private:
    bool bOpened = false;
    FVector ClosedPosition;
};
