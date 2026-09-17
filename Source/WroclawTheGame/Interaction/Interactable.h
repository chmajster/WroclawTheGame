#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"
class ASliceCharacter;
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
    GENERATED_BODY()
};
class WROCLAWTHEGAME_API IInteractable
{
    GENERATED_BODY()
  public:
    virtual FText Prompt(ASliceCharacter *Player) const = 0;
    virtual void Interact(ASliceCharacter *Player) = 0;
};
