#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SliceController.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceController : public APlayerController
{
    GENERATED_BODY()
  public:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;
    void OpenCCTV(class UTextureRenderTarget2D *Feed);
    void ContextAction();
    UPROPERTY() TObjectPtr<class UTextureRenderTarget2D> CCTVFeed;
    bool bCCTV = false;
    void OpenKeypad(const FString &Id);
    void ShowMessage(const FString &Text);
    void OpenPhone();
    void Peek();
    void Escape();
    void Confirm();
    void NewGame();
    void LoadGame();
    void Quit();
    void Inventory();
    void Backspace();
    void QuestLog();
    void Investigation();
    void Hint();
    void Achievements();
    void ScrollUp();
    void ScrollDown();
    bool GameplayBlocked() const
    {
        return bCCTV || bKeypad || bPeek || bInventory || bPhone || bInvestigation || !Message.IsEmpty();
    }
    bool bKeypad = false, bInventory = false, bPhone = false, bInvestigation = false, bPeek = false;
    FString Code, Message, PuzzleId;
    int32 Page = 0, Scroll = 0;
    double PanelOpenedAt = 0;
    UPROPERTY() TObjectPtr<class ACameraActor> PeekCamera;

  private:
    void Digit(int32 N);
    void CloseModal();
    void Suspend(bool bPause);
    void Reload();
    void Digit0();
    void Digit1();
    void Digit2();
    void Digit3();
    void Digit4();
    void Digit5();
    void Digit6();
    void Digit7();
    void Digit8();
    void Digit9();
    void LetterA();
    void LetterB();
    void LetterC();
    void LetterD();
    void LetterE();
    void LetterF();
    void LetterG();
    void LetterH();
    void LetterI();
    void LetterJ();
    void LetterK();
    void LetterL();
    void LetterM();
    void LetterN();
    void LetterO();
    void LetterP();
    void LetterQ();
    void LetterR();
    void LetterS();
    void LetterT();
    void LetterU();
    void LetterV();
    void LetterW();
    void LetterX();
    void LetterY();
    void LetterZ();
};
