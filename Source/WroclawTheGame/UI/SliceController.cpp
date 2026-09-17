#include "UI/SliceController.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "Interaction/SliceProp.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"
void ASliceController::BeginPlay() { Super::BeginPlay(); SetInputMode(FInputModeGameOnly()); bShowMouseCursor=false; }
void ASliceController::SetupInputComponent() {
 Super::SetupInputComponent();
 auto Bind=[&](FKey Key,void(ASliceController::*Function)()) {InputComponent->BindKey(Key,IE_Pressed,this,Function).bExecuteWhenPaused=true;};
 Bind(EKeys::Escape,&ASliceController::Escape); Bind(EKeys::Enter,&ASliceController::Confirm);
 Bind(EKeys::N,&ASliceController::NewGame); Bind(EKeys::L,&ASliceController::LoadGame); Bind(EKeys::Q,&ASliceController::Quit);
 Bind(EKeys::I,&ASliceController::Inventory); Bind(EKeys::BackSpace,&ASliceController::Backspace);
 const FKey Keys[]={EKeys::Zero,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
 const FKey Pad[]={EKeys::NumPadZero,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight,EKeys::NumPadNine};
 void(ASliceController::*Functions[])()={&ASliceController::Digit0,&ASliceController::Digit1,&ASliceController::Digit2,&ASliceController::Digit3,&ASliceController::Digit4,&ASliceController::Digit5,&ASliceController::Digit6,&ASliceController::Digit7,&ASliceController::Digit8,&ASliceController::Digit9};
 for(int I=0;I<10;++I){Bind(Keys[I],Functions[I]);Bind(Pad[I],Functions[I]);}
}
void ASliceController::OpenKeypad(ASliceProp* Target) {CodeTarget=Target; Code.Empty(); bKeypad=true; SetPause(true);}
void ASliceController::ShowMessage(const FString& Text) {Message=Text; SetPause(true);}
void ASliceController::Escape() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(M->bDead || M->State.Finished()) return;
 if(bKeypad || bInventory || !Message.IsEmpty()){bKeypad=false; bInventory=false; Message.Empty(); CodeTarget=nullptr; SetPause(false); return;}
 if(!M->bInGame) return;
 M->bShowMenu=!M->bShowMenu; SetPause(M->bShowMenu);
 if(auto* P=Cast<ASliceCharacter>(GetPawn())) {P->SetBlock(false); P->SetSprint(false);}
}
void ASliceController::Confirm() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(M->bDead) {LoadGame();return;}
 if(bKeypad) {
  if(Code.Len()!=4){M->Notify(TEXT("Wprowadź cztery cyfry."));return;}
  auto* P=CastChecked<ASliceCharacter>(GetPawn());
  if(CodeTarget) CodeTarget->SubmitCode(Code,P);
  else if(Code==TEXT("0417") && M->Apply(Wroclaw::Event::UnlockPhone)) M->Notify(TEXT("Telefon uruchomiony. Przeczytaj wiadomość [T]."));
  else M->Notify(TEXT("Nieprawidłowy PIN lub brakuje kartki z PIN-em / powerbanku."));
  bKeypad=false; CodeTarget=nullptr; SetPause(false); return;
 }
 if(!Message.IsEmpty() || bInventory) {Message.Empty(); bInventory=false; SetPause(false);return;}
 if(M->bShowMenu && M->bInGame) {M->bShowMenu=false;SetPause(false);}
}
void ASliceController::Reload() {
 bKeypad=false;bInventory=false;Message.Empty();CodeTarget=nullptr;SetPause(false);
 UGameplayStatics::OpenLevel(this,FName(TEXT("Przebudzenie")));
}
void ASliceController::NewGame() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(!M->bShowMenu && !M->bDead && !M->State.Finished()) return;
 M->NewGame(); Reload();
}
void ASliceController::LoadGame() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(!M->bShowMenu && !M->bDead) return;
 if(M->ContinueGame()) Reload();
}
void ASliceController::Quit() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(M->bShowMenu || M->bDead || M->State.Finished()) UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
}
void ASliceController::Inventory() {
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(!M->bInGame || M->bDead || M->bShowMenu || M->State.Finished() || bKeypad || !Message.IsEmpty()) return;
 bInventory=!bInventory; SetPause(bInventory);
}
void ASliceController::Digit(int32 N){if(bKeypad && Code.Len()<4) Code+=FString::FromInt(N);}
void ASliceController::Backspace(){if(bKeypad && !Code.IsEmpty()) Code.LeftChopInline(1);}
#define DIGIT(N) void ASliceController::Digit##N(){Digit(N);}
DIGIT(0) DIGIT(1) DIGIT(2) DIGIT(3) DIGIT(4) DIGIT(5) DIGIT(6) DIGIT(7) DIGIT(8) DIGIT(9)
#undef DIGIT
