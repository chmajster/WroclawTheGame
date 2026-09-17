#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "ResidentNPC.generated.h"
UCLASS()
class WROCLAWTHEGAME_API AResidentNPC : public ACharacter, public IInteractable
{
    GENERATED_BODY()
  public:
    AResidentNPC();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName DefinitionId;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void BeginPlay() override;
    virtual FText Prompt(class ASliceCharacter *Player) const override;
    virtual void Interact(class ASliceCharacter *Player) override;
};
