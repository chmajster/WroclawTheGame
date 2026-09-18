#include "UI/SliceHUD.h"
#include "Systems/WroclawMapSubsystem.h"
#include "Systems/CityCoverageSubsystem.h"
#include "Systems/CityGameplaySubsystem.h"
#include "Vehicles/RaceSession.h"
#include "Vehicles/DriveableVehicle.h"
#include "Geography/GeoPreviewGameMode.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/GameplayComponents.h"
#include "UI/SliceController.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Character/SliceCharacter.h"
#include "Mission/SliceMission.h"
#include "Interaction/Interactable.h"
#include "Core/SliceGameMode.h"
#include "Character/CharacterCreatorSubsystem.h"
void ASliceHUD::Text(const FString &Value, float X, float Y, float Scale, const FLinearColor &Color)
{
    DrawText(Value, Color, X, Y, GEngine->GetMediumFont(), Scale, false);
}
void ASliceHUD::Panel(const FString &Title, const FString &Body)
{
    DrawRect(FLinearColor(.015, .025, .035, .96), 0, 0, Canvas->SizeX, Canvas->SizeY);
    const float X = Canvas->SizeX * .06f, Y = Canvas->SizeY * .09f, MaxWidth = Canvas->SizeX - X * 2;
    Text(Title, X, Y, 1.5, FLinearColor(.9, .72, .38));
    TArray<FString> Lines, Paragraphs;
    Body.ParseIntoArrayLines(Paragraphs, false);
    for (const auto &Para : Paragraphs)
    {
        TArray<FString> Words;
        Para.ParseIntoArray(Words, TEXT(" "), true);
        FString Line;
        for (const auto &Word : Words)
        {
            const FString Candidate = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
            float W, H;
            GetTextSize(Candidate, W, H, GEngine->GetMediumFont(), 1);
            if (W > MaxWidth && !Line.IsEmpty())
            {
                Lines.Add(Line);
                Line = Word;
            }
            else
                Line = Candidate;
        }
        Lines.Add(Line);
    }
    auto *PC = CastChecked<ASliceController>(PlayerOwner);
    const int Visible = FMath::Max(1, FMath::FloorToInt((Canvas->SizeY - Y - 130) / 27));
    PC->Scroll = FMath::Clamp(PC->Scroll, 0, FMath::Max(0, Lines.Num() - Visible));
    for (int I = PC->Scroll; I < Lines.Num() && I < PC->Scroll + Visible; ++I)
        Text(Lines[I], X, Y + 65 + (I - PC->Scroll) * 27);
    if (Lines.Num() > Visible)
        Text(TEXT("↑ / ↓ — przewijaj"), X, Canvas->SizeY - 55, .8);
}
void ASliceHUD::DrawHUD()
{
    Super::DrawHUD();
    if (GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->bEditing) return;
    if (!Canvas)
        return;
    auto *PC = Cast<ASliceController>(PlayerOwner);
    if (!PC)
        return;
    auto *M = GetGameInstance()->GetSubsystem<USliceMission>();
    auto *CityGameplay = GetWorld()->GetSubsystem<UCityGameplaySubsystem>();
    if (Cast<AGeoPreviewGameMode>(GetWorld()->GetAuthGameMode()))
        Text(TEXT("Mapa: © OpenStreetMap contributors (ODbL) | teren: Copernicus EU-DEM / USGS, via Mapzen"),
             20, Canvas->SizeY - 22, .65);
#if !UE_BUILD_SHIPPING
    if (GetWorld()->GetSubsystem<UCityCoverageSubsystem>()->bShowOverlay)
    {
        DrawCityCoverage();
        return;
    }
#endif
    auto *P = Cast<ASliceCharacter>(PC->GetPawn());
    if (!P)
    {
        if (auto *Vehicle = Cast<ADriveableVehicle>(PC->GetPawn()))
        {
            if (PC->bPhone)
                Panel(TEXT("TELEFON"),M->PhoneText(PC->Page)+TEXT("\nESC — zamknij"));
            else if (!PC->Message.IsEmpty())
                Panel(TEXT("DZIENNIK"),PC->Message+TEXT("\nENTER / ESC — zamknij"));
            else if (M->bShowMenu)
                Panel(TEXT("PAUZA"), CityGameplay->IsActive() ? TEXT("ESC — wznów | N — nowy zapis miasta | L — wczytaj miasto | Q — wyjście") : TEXT("ESC — wznów | N — nowa kampania | L — checkpoint | Q — wyjście"));
            else
            {
                Text(Vehicle->Status(), 30, Canvas->SizeY - 120, .9);
                Text(CityGameplay->IsActive() ? CityGameplay->NearbyObjective() : TEXT("NADODRZE — TEST POJAZDU"), 30, 30, 1.0);
                if (auto *Race = Vehicle->FindComponentByClass<URaceSession>())
                    Text(Race->StatusText(), 30, 85, .9);
            }
        }
        if (FPlatformTime::Seconds()<M->NotificationUntil)
            Text(M->Notification.Left(135),30,Canvas->SizeY-160,.8,FLinearColor(1,.82,.4));
        return;
    }
    if (M->bDead)
        Panel(TEXT("NIE UDAŁO SIĘ UCIEC"), TEXT("ENTER / L — ostatni checkpoint\nN — nowa gra\nQ — wyjście"));
    else if (M->State.Finished())
        Panel(TEXT("ROZDZIAŁ 1 — PRZEBUDZENIE"),
              TEXT("Warsztat daje chwilę bezpieczeństwa. Mapa ujawnia blokady wyjazdowe, dworzec i "
                   "lotnisko.\nNie możesz jeszcze opuścić Wrocławia. Wymaga to ukończenia "
                   "kampanii.\n\nROZDZIAŁ 2 ODBLOKOWANY — zawartość poza zakresem tego etapu.\n\n") +
                  M->AchievementsText() + TEXT("\nN — nowa gra | Q — wyjście"));
    else if (M->bShowMenu && CityGameplay->IsActive())
        Panel(TEXT("WROCŁAW — PAUZA"), TEXT("ENTER / ESC — wznów\nL — wczytaj ostatni zapis miasta\nN — rozpocznij od nowa (usuwa postęp miasta)\nQ — wyjdź\n\nB — dziennik dzielnic | T — telefon\nKampania Przebudzenie posiada oddzielny zapis."));
    else if (M->bShowMenu)
        Panel(
            TEXT("WROCLAW THE GAME"),
            TEXT("PRZEBUDZENIE\n\nN — nowa gra\n") +
                FString(M->HasSave() ? TEXT("L — wczytaj checkpoint\n")
                                     : TEXT("Brak zgodnego zapisu v3 / v2\n")) +
                (M->bInGame ? TEXT("ENTER / ESC — wznów\n") : TEXT("")) +
                TEXT("Q — wyjście\n\nWASD ruch | mysz kamera | SHIFT sprint | CTRL kucanie | SPACJA skok\nE "
                     "interakcja | T telefon | I ekwipunek | J śledztwo | H podpowiedź\nLPM atak | PPM blok "
                     "| ALT unik | G rzut przedmiotu | V opatrunek\nF latarka | B questy | K osiągnięcia | "
                     "ESC pauza\nPodczas wpisywania kodu świat działa dalej. ESC pozwala przerwać i uciec."));
    else if (PC->bKeypad)
    {
        const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*PC->PuzzleId));
        if (!A)
            return;
        FString Help =
            A->kind == "choice" ? UTF8_TO_TCHAR(A->body.c_str()) : TEXT("Wpisz rozwiązanie: ") + PC->Code;
        if (A->kind == "symbols")
            Help += TEXT("\n1 słońce | 2 księżyc | 3 fala | 4 gwiazda");
        if (A->kind == "wires")
            Help += TEXT("\n1 czerwony | 2 biały | 3 niebieski");
        if (A->kind == "password")
            Help += TEXT("\nHasło wpisz wielkimi literami, zgodnie ze wskazówkami.");
        if (A->kind == "lights")
            Help += TEXT(
                "\nObserwuj diody przez 6 sekund, potem odtwórz kolejność. Ponowne E odtwarza sekwencję.");
        auto It = M->State.locks.find(A->id);
        if (It != M->State.locks.end() && It->second.until > M->State.elapsed)
            Help += FString::Printf(TEXT("\nBLOKADA: %.0f s"),
                                    FMath::CeilToDouble(It->second.until - M->State.elapsed));
        Panel(UTF8_TO_TCHAR(A->label.c_str()),
              Help + TEXT("\n\nENTER — zatwierdź | BACKSPACE — popraw | ESC — przerwij"));
        if (A->kind == "lights")
        {
            const double T = M->State.elapsed - PC->PanelOpenedAt;
            const auto Sequence = M->State.Answer(*A);
            const int Index = FMath::FloorToInt(T);
            const int Lit = T >= 0 && T < 4 && FMath::Frac(T) < .65 ? Sequence[Index] - '1' : -1;
            for (int I = 0; I < 4; ++I)
            {
                const float X = Canvas->SizeX * .25f + I * 100;
                DrawRect(I == Lit ? FLinearColor(1, .75, .15) : FLinearColor(.12, .15, .18), X,
                         Canvas->SizeY * .65, 65, 55);
                Text(FString::FromInt(I + 1), X + 25, Canvas->SizeY * .65 + 17);
            }
        }
    }
    else if (PC->bCCTV)
    {
        Panel(TEXT("MONITORING — ESC, aby zamknąć"), TEXT(""));
        if (PC->CCTVFeed)
            DrawTexture(PC->CCTVFeed, Canvas->SizeX * .1, Canvas->SizeY * .2, Canvas->SizeX * .8,
                        Canvas->SizeY * .65, 0, 0, 1, 1);
    }
    else if (PC->bPeek)
    {
        Text(TEXT("WIZJER — obcy na klatce. ESC — odsuń się"), Canvas->SizeX * .25, Canvas->SizeY * .8);
    }
    else if (PC->bInventory)
        Panel(TEXT("EKWIPUNEK"), M->InventoryText() + TEXT("\nI / ESC — zamknij"));
    else if (PC->bPhone)
        Panel(TEXT("TELEFON"), M->PhoneText(PC->Page) + TEXT("\nESC — zamknij"));
    else if (PC->bInvestigation)
        Panel(TEXT("ŚLEDZTWO"), M->InvestigationText(PC->Page) + TEXT("\nESC — zamknij"));
    else if (!PC->Message.IsEmpty())
        Panel(TEXT("WSKAZÓWKA"), PC->Message + TEXT("\n\nENTER / ESC — zamknij"));
    else
    {
        DrawRect(FLinearColor(0, 0, 0, .7), 20, 20, Canvas->SizeX - 40, 75);
        Text(CityGameplay->IsActive() ? CityGameplay->NearbyObjective() : M->ObjectiveText(), 36, 35);
        const float Y = Canvas->SizeY - 95;
        DrawRect(FLinearColor(.08, .08, .08), 30, Y, 220, 16);
        DrawRect(FLinearColor(.7, .15, .15), 30, Y, 220 * P->HealthState->Value / 100, 16);
        DrawRect(FLinearColor(.08, .08, .08), 30, Y + 28, 220, 12);
        DrawRect(FLinearColor(.1, .65, .55), 30, Y + 28, 220 * P->StaminaState->Value / 100, 12);
        if (P->Hiding.IsValid())
            Text(TEXT("UKRYCIE — E, aby wyjść"), 35, Canvas->SizeY - 200);
        DrawRect(FLinearColor::White, Canvas->SizeX / 2.f - 2, Canvas->SizeY / 2.f - 2, 4, 4);
        if (auto *Target = Cast<IInteractable>(P->Focus))
            Text(TEXT("[E] ") + Target->Prompt(P).ToString(), Canvas->SizeX * .25, Canvas->SizeY * .72);
        Text(FString::Printf(TEXT("HEAT %d / 5"), M->WorldState.HeatLevel()), 35, Canvas->SizeY - 220, .8);
        Text(TEXT("J śledztwo | H podpowiedź | T telefon | I ekwipunek"), 300, Canvas->SizeY - 40, .75);
    }
    if (PC->bKeypad && !M->bDead)
    {
        DrawRect(FLinearColor(.1, .1, .1), 30, Canvas->SizeY - 75, 220, 16);
        DrawRect(FLinearColor(.7, .15, .15), 30, Canvas->SizeY - 75, 220 * P->HealthState->Value / 100, 16);
    }
    if (auto *GM = Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode()))
        Text(GM->ThreatText(), Canvas->SizeX - 360, Canvas->SizeY - 100, .85, FLinearColor(1, .4, .2));
    if (!M->bLastSaveSucceeded)
        Text(TEXT("BŁĄD ZAPISU — bieżący postęp niezapisany"), 35, Canvas->SizeY - 170, .85,
             FLinearColor::Red);
    if (FPlatformTime::Seconds() < M->NotificationUntil)
        Text(M->Notification.Left(135), 35, Canvas->SizeY - 140, .8, FLinearColor(1, .82, .4));
}

void ASliceHUD::DrawCityCoverage()
{
    const auto *Coverage = GetWorld()->GetSubsystem<UCityCoverageSubsystem>();
    const auto *City = GetWorld()->GetSubsystem<UWroclawMapSubsystem>()->GetCity();
    DrawRect(FLinearColor(.015, .025, .035, .96), 0, 0, Canvas->SizeX, Canvas->SizeY);
    Text(TEXT("CITY COVERAGE — granice produkcyjne"), 25, 20, 1.2);
    Text(Coverage->ReportText(), 25, 55, .9);
    if (!City || City->Sectors.IsEmpty()) return;
    FVector2D Min(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
    FVector2D Max(-TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
    for (const auto &Sector : City->Sectors)
        for (const auto &Point : Sector.Boundary)
        {
            Min.X = FMath::Min(Min.X, Point.X); Min.Y = FMath::Min(Min.Y, Point.Y);
            Max.X = FMath::Max(Max.X, Point.X); Max.Y = FMath::Max(Max.Y, Point.Y);
        }
    const double Scale = FMath::Min((Canvas->SizeX - 80.) / FMath::Max(1., Max.X - Min.X),
                                    (Canvas->SizeY - 180.) / FMath::Max(1., Max.Y - Min.Y));
    const auto Screen = [&](const FVector2D &Point) { return FVector2D(40, 100) + (Point - Min) * Scale; };
    for (const auto &Sector : City->Sectors)
    {
        if (Sector.Boundary.Num() < 3) continue;
        const FLinearColor Color = Coverage->Color(Sector.Status);
        FVector2D Centre = FVector2D::ZeroVector;
        for (int32 I = 0; I < Sector.Boundary.Num(); ++I)
        {
            const FVector2D A = Screen(Sector.Boundary[I]);
            const FVector2D B = Screen(Sector.Boundary[(I + 1) % Sector.Boundary.Num()]);
            DrawLine(A.X, A.Y, B.X, B.Y, Color, 2);
            Centre += A;
        }
        Centre /= Sector.Boundary.Num();
        Text(Sector.DisplayName, Centre.X - 65, Centre.Y - 17, .7, Color);
        Text(Coverage->Label(Sector.Status), Centre.X - 65, Centre.Y + 3, .7, Color);
    }
    if (const APawn *Pawn = PlayerOwner->GetPawn())
    {
        const FVector Position = Pawn->GetActorLocation();
        const FVector2D Point = Screen(FVector2D(Position.X, Position.Y));
        DrawRect(FLinearColor::White, Point.X - 3, Point.Y - 3, 6, 6);
    }
    Text(TEXT("Szary Missing | niebieski GIS | pomarańczowy Blockout | żółty Playable | zielony Detailed | fioletowy Final"),
         25, Canvas->SizeY - 45, .65);
}
