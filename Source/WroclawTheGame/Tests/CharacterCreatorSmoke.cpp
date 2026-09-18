#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "ImageUtils.h"
#include "UI/SliceController.h"
#include "UI/CharacterCreatorWidget.h"
#include "Character/CharacterCreator.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Mission/SliceMission.h"

// Explicit opt-in smoke harness; never starts a campaign or touches a player save.
static FAutoConsoleCommandWithWorld GCreatorSmoke(
    TEXT("WTG.CreatorSmoke"),TEXT("Open creator, exercise editing and capture UI without writing saves."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
    {
        if (!World || !World->IsGameWorld()) return;
        TWeakObjectPtr<UWorld> WeakWorld(World); float Elapsed=0; bool Opened=false, Captured=false;
        FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld,Elapsed,Opened,Captured](float Dt) mutable
        {
            auto* W=WeakWorld.Get(); if (!W) return false; Elapsed+=Dt;
            auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(W,0));
            if (!Opened && PC && Elapsed>2)
            {
                auto* M=W->GetGameInstance()->GetSubsystem<USliceMission>(); M->bShowMenu=true;
                PC->NewGame(); Opened=true;
                auto* C=W->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>(); C->Randomize(12345); C->Undo(); C->Redo();
                if (PC->CharacterCreatorWidget) PC->CharacterCreatorWidget->Refresh();
                UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_OPEN widget=%d seed=%d"),PC->CharacterCreatorWidget!=nullptr,C->Draft.RandomSeed);
            }
            if (Opened && !Captured && Elapsed>8 && PC && PC->CharacterCreatorWidget)
            {
                auto* UI=PC->CharacterCreatorWidget.Get(); auto* C=W->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>();
                const bool Matches=FCharacterAppearanceDefinition::StaticStruct()->CompareScriptStruct(&UI->Studio->Appearance->Appearance,&C->Draft,0);
                UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_RENDER matches=%d loading=%d viewport=%d"),Matches,UI->Studio->Appearance->bLoading,GEngine->GameViewport!=nullptr);
                UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_CAMERA %s / %s"),*UI->Studio->Capture->GetComponentLocation().ToString(),*UI->Studio->Capture->GetComponentRotation().ToString());
                TArray<UPrimitiveComponent*> Parts; UI->Studio->GetComponents(Parts);
                for (auto* P:Parts) if (P->IsVisible()) { UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_PART %s at %s"),*P->GetName(),*P->GetComponentLocation().ToString()); break; }
                TArray<FColor> Pixels; UI->Studio->RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
                int32 Lit=0,Opaque=0; for (const auto& P:Pixels) { if (P.R>5 || P.G>5 || P.B>5) ++Lit; if (P.A>0) ++Opaque; }
                UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_PIXELS lit=%d alpha=%d total=%d"),Lit,Opaque,Pixels.Num());
                TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(720,1000,Pixels,Png); FFileHelper::SaveArrayToFile(Png,*(FPaths::ProjectSavedDir()/TEXT("Tests/CharacterCreatorCapture.png")));
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/CharacterCreatorPreview.png"),true,false);
                Captured=true;
            }
            if (Elapsed>12)
            {
                if (PC && PC->CharacterCreatorWidget) PC->CancelCharacterCreator();
                UE_LOG(LogTemp,Display,TEXT("WTG_CREATOR_SMOKE_DONE")); FPlatformMisc::RequestExit(false); return false;
            }
            return true;
        }));
    }));
#endif
