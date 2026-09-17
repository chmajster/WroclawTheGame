#include "UI/SliceController.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraActor.h"
#include "Audio/SliceAudio.h"
void ASliceController::BeginPlay(){Super::BeginPlay();SetInputMode(FInputModeGameOnly());bShowMouseCursor=false;}
void ASliceController::SetupInputComponent(){
 Super::SetupInputComponent();
 auto Bind=[&](FKey Key,void(ASliceController::*Function)()){auto& B=InputComponent->BindKey(Key,IE_Pressed,this,Function);B.bExecuteWhenPaused=true;B.bConsumeInput=false;};
 Bind(EKeys::Escape,&ASliceController::Escape);Bind(EKeys::Enter,&ASliceController::Confirm);
 Bind(EKeys::N,&ASliceController::NewGame);Bind(EKeys::L,&ASliceController::LoadGame);Bind(EKeys::Q,&ASliceController::Quit);
 Bind(EKeys::B,&ASliceController::QuestLog);Bind(EKeys::I,&ASliceController::Inventory);Bind(EKeys::J,&ASliceController::Investigation);Bind(EKeys::H,&ASliceController::Hint);Bind(EKeys::K,&ASliceController::Achievements);
 Bind(EKeys::BackSpace,&ASliceController::Backspace);Bind(EKeys::Up,&ASliceController::ScrollUp);Bind(EKeys::Down,&ASliceController::ScrollDown);
 Bind(EKeys::A,&ASliceController::LetterA);
 Bind(EKeys::B,&ASliceController::LetterB);
 Bind(EKeys::C,&ASliceController::LetterC);
 Bind(EKeys::D,&ASliceController::LetterD);
 Bind(EKeys::E,&ASliceController::LetterE);
 Bind(EKeys::F,&ASliceController::LetterF);
 Bind(EKeys::G,&ASliceController::LetterG);
 Bind(EKeys::H,&ASliceController::LetterH);
 Bind(EKeys::I,&ASliceController::LetterI);
 Bind(EKeys::J,&ASliceController::LetterJ);
 Bind(EKeys::K,&ASliceController::LetterK);
 Bind(EKeys::L,&ASliceController::LetterL);
 Bind(EKeys::M,&ASliceController::LetterM);
 Bind(EKeys::N,&ASliceController::LetterN);
 Bind(EKeys::O,&ASliceController::LetterO);
 Bind(EKeys::P,&ASliceController::LetterP);
 Bind(EKeys::Q,&ASliceController::LetterQ);
 Bind(EKeys::R,&ASliceController::LetterR);
 Bind(EKeys::S,&ASliceController::LetterS);
 Bind(EKeys::T,&ASliceController::LetterT);
 Bind(EKeys::U,&ASliceController::LetterU);
 Bind(EKeys::V,&ASliceController::LetterV);
 Bind(EKeys::W,&ASliceController::LetterW);
 Bind(EKeys::X,&ASliceController::LetterX);
 Bind(EKeys::Y,&ASliceController::LetterY);
 Bind(EKeys::Z,&ASliceController::LetterZ);
 const FKey Keys[]={EKeys::Zero,EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
 const FKey Pad[]={EKeys::NumPadZero,EKeys::NumPadOne,EKeys::NumPadTwo,EKeys::NumPadThree,EKeys::NumPadFour,EKeys::NumPadFive,EKeys::NumPadSix,EKeys::NumPadSeven,EKeys::NumPadEight,EKeys::NumPadNine};
 void(ASliceController::*Functions[])()={&ASliceController::Digit0,&ASliceController::Digit1,&ASliceController::Digit2,&ASliceController::Digit3,&ASliceController::Digit4,&ASliceController::Digit5,&ASliceController::Digit6,&ASliceController::Digit7,&ASliceController::Digit8,&ASliceController::Digit9};
 for(int I=0;I<10;++I){Bind(Keys[I],Functions[I]);Bind(Pad[I],Functions[I]);}
}
void ASliceController::Suspend(bool bPause){
 SetPause(bPause);ResetIgnoreMoveInput();ResetIgnoreLookInput();SetIgnoreMoveInput(true);SetIgnoreLookInput(true);
 if(auto* P=Cast<ASliceCharacter>(GetPawn())){P->SetSprint(false);P->SetBlock(false);P->GetCharacterMovement()->StopMovementImmediately();}
}
void ASliceController::CloseModal(){
 bKeypad=false;bInventory=false;bPhone=false;bInvestigation=false;bPeek=false;Message.Empty();PuzzleId.Empty();Scroll=0;
 if(PeekCamera){SetViewTarget(GetPawn());PeekCamera->Destroy();PeekCamera=nullptr;}
 ResetIgnoreMoveInput();ResetIgnoreLookInput();SetPause(false);
}
void ASliceController::OpenKeypad(const FString& Id){
 const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*Id));if(!A)return;
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(!M->State.CanDo(*A)){M->Notify(TEXT("Brakuje wcześniejszego etapu lub przedmiotu."));return;}
 CloseModal();PuzzleId=Id;Code.Empty();bKeypad=true;PanelOpenedAt=M->State.elapsed;Suspend(false);
}
void ASliceController::ShowMessage(const FString& Text){CloseModal();Message=Text;Suspend(true);}
void ASliceController::OpenPhone(){
 CloseModal();bPhone=true;Page=0;GetGameInstance()->GetSubsystem<USliceMission>()->PhoneText(Page);Suspend(true);
}
void ASliceController::Peek(){
 CloseModal();bPeek=true;Suspend(false);
 PeekCamera=GetWorld()->SpawnActor<ACameraActor>(FVector(1260,400,535),FRotator(-10,0,0));if(PeekCamera) SetViewTargetWithBlend(PeekCamera,0.2);
}
void ASliceController::Escape(){
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bDead || M->State.Finished())return;
 if(GameplayBlocked()){CloseModal();return;}if(!M->bInGame)return;
 M->bShowMenu=!M->bShowMenu;SetPause(M->bShowMenu);
 if(auto* P=Cast<ASliceCharacter>(GetPawn())){P->SetSprint(false);P->SetBlock(false);}
}
void ASliceController::Confirm(){
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bDead){LoadGame();return;}
 if(bKeypad){
  const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*PuzzleId));if(!A)return;
  if(A->kind=="lights" && M->State.elapsed-PanelOpenedAt<6){M->Notify(TEXT("Najpierw obejrzyj całą sekwencję."));return;}
  Wroclaw::Result R;
  if(A->kind=="choice"){
   if(Code!=TEXT("1") && Code!=TEXT("2")){M->Notify(TEXT("Wybierz 1 lub 2."));return;}
   R=M->Act(Code==TEXT("1")?PuzzleId:UTF8_TO_TCHAR(A->alternate.c_str()));
  }else R=M->Submit(PuzzleId,Code);
  if(R==Wroclaw::Result::Applied || R==Wroclaw::Result::AlreadyDone){CloseModal();USliceAudio::Play(this,TEXT("Switch"),GetPawn()->GetActorLocation());}
  else{Code.Empty();if(R==Wroclaw::Result::Wrong || R==Wroclaw::Result::Cooldown) USliceAudio::Play(this,TEXT("Alarm"),GetPawn()->GetActorLocation());}
  return;
 }
 if(GameplayBlocked()){CloseModal();return;}
 if(M->bShowMenu && M->bInGame){M->bShowMenu=false;SetPause(false);}
}
void ASliceController::Reload(){CloseModal();UGameplayStatics::OpenLevel(this,FName(TEXT("Przebudzenie")));}
void ASliceController::NewGame(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(!M->bShowMenu && !M->bDead && !M->State.Finished())return;M->NewGame();Reload();}
void ASliceController::LoadGame(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(!M->bShowMenu && !M->bDead)return;if(M->ContinueGame())Reload();}
void ASliceController::Quit(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bShowMenu || M->bDead || M->State.Finished())UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);}
void ASliceController::Inventory(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(!M->bInGame || M->bDead || M->bShowMenu || M->State.Finished())return;if(bInventory){CloseModal();return;}if(GameplayBlocked())return;bInventory=true;Suspend(true);}
void ASliceController::Investigation(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(!M->bInGame || M->bDead || M->bShowMenu || M->State.Finished())return;if(bInvestigation){CloseModal();return;}if(GameplayBlocked())return;bInvestigation=true;Page=0;Scroll=0;Suspend(true);}
void ASliceController::Hint(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bInGame && !M->bDead && !M->bShowMenu && !GameplayBlocked())ShowMessage(M->HintText());}
void ASliceController::Achievements(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bInGame && !M->bDead && !M->bShowMenu && !GameplayBlocked())ShowMessage(M->AchievementsText());}
void ASliceController::ScrollUp(){Scroll=FMath::Max(0,Scroll-2);}void ASliceController::ScrollDown(){Scroll=FMath::Min(500,Scroll+2);}
void ASliceController::Digit(int32 N){
 if(bKeypad && Code.Len()<8){
  auto* M=GetGameInstance()->GetSubsystem<USliceMission>();const auto* A=Wroclaw::Progress::Find(TCHAR_TO_UTF8(*PuzzleId));
  if(A && A->kind=="lights" && M->State.elapsed-PanelOpenedAt<6)return;
  Code+=FString::FromInt(N);
 }
 else if(bPhone && N>=1 && N<=5){Page=N-1;Scroll=0;GetGameInstance()->GetSubsystem<USliceMission>()->PhoneText(Page);}
 else if(bInvestigation && N>=1 && N<=4){Page=N-1;Scroll=0;}
}
void ASliceController::Backspace(){if(bKeypad && !Code.IsEmpty())Code.LeftChopInline(1);}
#define DIGIT(N) void ASliceController::Digit##N(){Digit(N);}
DIGIT(0) DIGIT(1) DIGIT(2) DIGIT(3) DIGIT(4) DIGIT(5) DIGIT(6) DIGIT(7) DIGIT(8) DIGIT(9)
#undef DIGIT
#define LETTER(C) void ASliceController::Letter##C(){if(bKeypad && Code.Len()<8)Code+=TEXT(#C);}
LETTER(A) LETTER(B) LETTER(C) LETTER(D) LETTER(E) LETTER(F) LETTER(G) LETTER(H) LETTER(I) LETTER(J) LETTER(K) LETTER(L) LETTER(M) LETTER(N) LETTER(O) LETTER(P) LETTER(Q) LETTER(R) LETTER(S) LETTER(T) LETTER(U) LETTER(V) LETTER(W) LETTER(X) LETTER(Y) LETTER(Z)
#undef LETTER

void ASliceController::QuestLog(){auto* M=GetGameInstance()->GetSubsystem<USliceMission>();if(M->bInGame && !M->bDead && !M->bShowMenu && !GameplayBlocked())ShowMessage(M->QuestLogText());}
