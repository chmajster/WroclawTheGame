#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"
#include "UI/Menu/MenuLoadingScreen.h"

static FAutoConsoleCommandWithWorld GMenuViewSmoke(TEXT("WTG.MenuViewSmoke"),TEXT("Capture real frontend without changing player saves."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W){
    if(!W || !W->IsGameWorld())return;
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(W),Elapsed=0.f,Shot=false,ResumeRequested=false,PauseOpened=false,PauseShot=false,LoadingShot=false,FrameMs=TArray<float>()](float Dt)mutable{
        auto* World=Weak.Get();if(!World)return false;Elapsed+=Dt;
        auto* Menu=World->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();
        if(Menu->Screen==EWTGMenuScreen::Main && Elapsed>5 && Elapsed<25)FrameMs.Add(Dt*1000);
        if(Elapsed>25 && !Shot && Menu->Screen==EWTGMenuScreen::Main){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuMain.png"),true,false);Shot=true;
        }
        if(Shot && Elapsed>26 && !ResumeRequested){Menu->bHasSession=true;Menu->Resume();ResumeRequested=true;}
        if(ResumeRequested && Elapsed>27 && !PauseOpened && Menu->Screen==EWTGMenuScreen::Playing){Menu->ShowPause();PauseOpened=true;}
        if(PauseOpened && Elapsed>28 && !PauseShot){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuPause.png"),true,false);PauseShot=true;
        }
        if(PauseShot && Elapsed>29 && !LoadingShot){World->GetGameViewport()->AddViewportWidgetContent(WTGMenu::LoadingScreen(),900);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuLoading.png"),true,false);LoadingShot=true;}
        if((LoadingShot && Elapsed>32) || Elapsed>60){
            if(!FrameMs.IsEmpty()){double Sum=0;for(float V:FrameMs)Sum+=V;FrameMs.Sort();
                UE_LOG(LogTemp,Display,TEXT("WTG_MENU_FRAME_BUDGET frames=%d mean_ms=%.2f p95_ms=%.2f viewport=%dx%d"),FrameMs.Num(),Sum/FrameMs.Num(),FrameMs[FMath::Min(FrameMs.Num()-1,int32(FrameMs.Num()*.95))],World->GetGameViewport()->Viewport->GetSizeXY().X,World->GetGameViewport()->Viewport->GetSizeXY().Y);}
            UE_LOG(LogTemp,Display,TEXT("WTG_MENU_VIEW_DONE result=%s paused=%d"),Shot && PauseShot && LoadingShot?TEXT("PASS"):TEXT("FAIL"),World->IsPaused());
            FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }));
 }));
#endif
