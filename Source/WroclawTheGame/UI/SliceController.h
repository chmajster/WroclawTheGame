#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SliceController.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceController : public APlayerController {
 GENERATED_BODY()
public:
 virtual void SetupInputComponent() override;
 virtual void BeginPlay() override;
 void OpenKeypad(class ASliceProp* Target);
 void ShowMessage(const FString& Text);
 void Escape(); void Confirm(); void NewGame(); void LoadGame(); void Quit(); void Inventory(); void Backspace();
 bool bKeypad=false, bInventory=false;
 FString Code, Message;
 UPROPERTY() TObjectPtr<class ASliceProp> CodeTarget;
private:
 void Digit(int32 N);
 void Digit0(); void Digit1(); void Digit2(); void Digit3(); void Digit4();
 void Digit5(); void Digit6(); void Digit7(); void Digit8(); void Digit9();
 void Reload();
};
