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
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(W),Elapsed=0.f,Shot=false,ResumeRequested=false,PauseShot=false,LoadingShot=false](float Dt)mutable{
        auto* World=Weak.Get();if(!World)return false;Elapsed+=Dt;
        auto* Menu=World->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();
        if(Elapsed>25 && !Shot && Menu->Screen==EWTGMenuScreen::Main){
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuMain.png"),true,false);Shot=true;
        }
        if(Shot && Elapsed>26 && !ResumeRequested){Menu->bHasSession=true;Menu->Resume();ResumeRequested=true;}
        if(ResumeRequested && Elapsed>27 && !PauseShot && Menu->Screen==EWTGMenuScreen::Playing){
            Menu->ShowPause();
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuPause.png"),true,false);PauseShot=true;
        }
        if(PauseShot && Elapsed>29 && !LoadingShot){World->GetGameViewport()->AddViewportWidgetContent(WTGMenu::LoadingScreen(),900);
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests/MenuLoading.png"),true,false);LoadingShot=true;}
        if((LoadingShot && Elapsed>32) || Elapsed>60){
            UE_LOG(LogTemp,Display,TEXT("WTG_MENU_VIEW_DONE result=%s paused=%d"),Shot && PauseShot && LoadingShot?TEXT("PASS"):TEXT("FAIL"),World->IsPaused());
            FPlatformMisc::RequestExit(false);return false;
        }
        return true;
    }));
 }));
#endif
