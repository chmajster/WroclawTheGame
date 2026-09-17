#include "UI/SliceHUD.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UI/SliceController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "Interaction/Interactable.h"
#include "Core/SliceGameMode.h"
#include "AI/SliceEnemy.h"
void ASliceHUD::Text(const FString& Value,float X,float Y,float Scale,const FLinearColor& Color) {
 DrawText(Value,Color,X,Y,GEngine->GetMediumFont(),Scale,false);
}
void ASliceHUD::Panel(const FString& Title,const FString& Body) {
 DrawRect(FLinearColor(0.015f,0.025f,0.035f,0.96f),0,0,Canvas->SizeX,Canvas->SizeY);
 const float X=Canvas->SizeX*0.08f, Y=Canvas->SizeY*0.16f;
 Text(Title,X,Y,1.65f,FLinearColor(0.9f,0.72f,0.38f));
 TArray<FString> Lines; Body.ParseIntoArrayLines(Lines,false);
 float Row=Y+70;
 for(const FString& Line:Lines) {Text(Line,X,Row,1.0f);Row+=30;}
}
void ASliceHUD::DrawHUD() {
 Super::DrawHUD(); if(!Canvas) return;
 auto* PC=Cast<ASliceController>(PlayerOwner); if(!PC) return;
 auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 auto* P=Cast<ASliceCharacter>(PC->GetPawn()); if(!P) return;
 if(M->bDead) Panel(TEXT("NIE UDAŁO SIĘ UCIEC"),TEXT("ENTER / L — wróć do ostatniego checkpointu\nN — nowa gra\nQ — wyjście"));
 else if(M->State.Finished()) Panel(TEXT("PRZEBUDZENIE — KONIEC ROZDZIAŁU"),TEXT("Drzwi zamykają się za tobą. Tym razem udało się zgubić pościg.\nTo dopiero początek. Nadal jesteś we Wrocławiu.\n\nUkończono rozdział.\nN — nowa gra\nQ — wyjście"));
 else if(M->bShowMenu) Panel(TEXT("WROCLAW THE GAME"),FString(TEXT("PRZEBUDZENIE\n\nN — nowa gra\n"))+(M->HasSave()?TEXT("L — wczytaj checkpoint\n"):TEXT("Brak zapisanego checkpointu\n"))+(M->bInGame?TEXT("ENTER / ESC — wznów\n"):TEXT(""))+TEXT("Q — wyjście\n\nWASD — ruch | mysz — kamera | SHIFT — sprint | CTRL — kucanie\nSPACJA — skok | E — interakcja | T — telefon | I — ekwipunek\nLPM — lekki atak | PPM — blok | ESC — pauza\nWalka kosztuje staminę. Narożniki i cichy ruch pomagają zgubić napastnika."));
 else if(PC->bKeypad) Panel(PC->CodeTarget?TEXT("ZAMEK SZAFKI"):TEXT("TELEFON — PIN"),TEXT("Wprowadź 4 cyfry: ")+PC->Code+TEXT("\n\nCyfry 0–9 — wpisz | BACKSPACE — usuń\nENTER — zatwierdź | ESC — anuluj"));
 else if(PC->bInventory) Panel(TEXT("EKWIPUNEK"),M->InventoryText()+TEXT("\nI / ESC — zamknij"));
 else if(!PC->Message.IsEmpty()) Panel(TEXT("WSKAZÓWKA"),PC->Message+TEXT("\n\nENTER / ESC — zamknij"));
 else {
  DrawRect(FLinearColor(0,0,0,0.7),20,20,Canvas->SizeX-40,90);
  Text(TEXT("PRZEBUDZENIE"),36,28,0.85f,FLinearColor(0.9f,0.72f,0.38f)); Text(M->ObjectiveText(),36,56);
  const float Y=Canvas->SizeY-100;
  DrawRect(FLinearColor(0.08f,0.08f,0.08f),30,Y,220,16); DrawRect(FLinearColor(0.7f,0.15f,0.15f),30,Y,220*P->Health/100,16);
  DrawRect(FLinearColor(0.08f,0.08f,0.08f),30,Y+28,220,12); DrawRect(FLinearColor(0.1f,0.65f,0.55f),30,Y+28,220*P->Stamina/100,12);
  Text(TEXT("ZDROWIE / STAMINA"),30,Y-25,0.7f);
  DrawRect(FLinearColor::White,Canvas->SizeX/2.f-2,Canvas->SizeY/2.f-2,4,4);
  if(auto* Target=Cast<IInteractable>(P->Focus)) Text(TEXT("[E] ")+Target->Prompt(P).ToString(),Canvas->SizeX*0.3f,Canvas->SizeY*0.73f);
  if(auto* GM=Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode())) if(GM->Enemy) Text(GM->Enemy->StatusText(),Canvas->SizeX-330,Y,0.85f,FLinearColor(1,0.55f,0.25f));
  Text(TEXT("E interakcja  |  T telefon  |  I ekwipunek  |  ESC pauza"),300,Canvas->SizeY-40,0.7f);
 }
 if(!M->bLastSaveSucceeded) Text(TEXT("BŁĄD ZAPISU: bieżący postęp nie został zapisany."),35,Canvas->SizeY-170,0.85f,FLinearColor::Red);
 if(FPlatformTime::Seconds()<M->NotificationUntil) Text(M->Notification,35,Canvas->SizeY-140,0.85f,FLinearColor(1,0.82f,0.4f));
}
