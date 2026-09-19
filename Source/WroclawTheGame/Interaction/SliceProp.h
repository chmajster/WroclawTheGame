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
    // Keep the legacy serialized property order stable. Przebudzenie_Source and
    // other cooked assets predate Collider, and unversioned property serialization
    // relies on matching field order between the editor cooker and runtime build.
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ActionId;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UDoorComponent> Door;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBasePuzzleComponent> Puzzle;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Collider;
    virtual void BeginPlay() override;
    void Configure(const FString &Id, const FVector &Size);
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
    virtual void Tick(float DeltaSeconds) override;

  private:
    bool bOpened = false;
    FVector ClosedPosition;
};
