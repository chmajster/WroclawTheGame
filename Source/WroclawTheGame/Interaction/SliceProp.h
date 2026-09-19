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
    // Do not add reflected fields here without regenerating Przebudzenie_Source.
    // Launch On uses unversioned cooked properties, so the native schema must stay
    // byte-for-byte compatible with the map authored before InteractionBounds existed.
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FString ActionId;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UDoorComponent> Door;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UBasePuzzleComponent> Puzzle;
    virtual void BeginPlay() override;
    void Configure(const FString &Id, const FVector &Size);
    virtual FText Prompt(ASliceCharacter *Player) const override;
    virtual void Interact(ASliceCharacter *Player) override;
    virtual void Tick(float DeltaSeconds) override;

  private:
    bool bOpened = false;
    FVector ClosedPosition;
};
