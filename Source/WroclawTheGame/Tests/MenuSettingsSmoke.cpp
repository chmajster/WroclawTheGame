#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/WTGInputProfileSubsystem.h"
#include "UI/PerformanceSettings.h"
#include "Audio/WTGAudioSettings.h"
#include "Character/SliceCharacter.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

static FAutoConsoleCommandWithWorld GMenuSettingsSmoke(TEXT("WTG.MenuSettingsSmoke"),TEXT("Settings/mappings check; requires isolated MenuQA settings INI."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W){
    if(!W || !W->IsGameWorld())return;
    const FString ConfigPath=FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir());
    if(!FPaths::ConvertRelativePathToFull(ConfigPath).StartsWith(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("MenuQA/")))){UE_LOG(LogTemp,Error,TEXT("WTG_MENU_SETTINGS_DONE result=FAIL isolated_ini_required"));FPlatformMisc::RequestExit(false);return;}
    const bool Replay=FParse::Param(FCommandLine::Get(),TEXT("WTGMenuSettingsReplay"));
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(W),Replay,Elapsed=0.f](float Dt)mutable{
        auto* World=Weak.Get();if(!World)return false;Elapsed+=Dt;
        auto* Menu=World->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();
        if(Menu->Screen!=EWTGMenuScreen::Main && Elapsed<45)return true;
        bool OK=Menu->Screen==EWTGMenuScreen::Main;
        auto Check=[&](bool V,const TCHAR* Name){OK&=V;UE_LOG(LogTemp,Display,TEXT("WTG_MENU_SETTINGS_CHECK %s=%s"),Name,V?TEXT("PASS"):TEXT("FAIL"));};
        auto* I=World->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>();auto* S=GEngine->GetGameUserSettings();auto* A=UWTGAudioSettings::Get();
        if(!Replay){
            Check(I->RebindKeyboard("Jump",EKeys::P),TEXT("rebind"));
            Check(!I->RebindKeyboard("Interact",EKeys::P),TEXT("context_conflict"));
            Check(!I->RebindKeyboard("Jump",EKeys::Escape),TEXT("reserved_escape"));
            Check(!I->RebindKeyboard("unknown",EKeys::O),TEXT("unknown_action"));
            Check(I->RebindGamepad("CarHorn",EKeys::Gamepad_Special_Left),TEXT("vehicle_pad"));
            I->SetMouseSensitivity(1.7f);I->SetGamepadSensitivity(1.5f);I->SetGamepadDeadzone(.3f);
            I->Profile.bInvertMouseY=true;I->SaveProfile();A->SetMasterVolume(.4f);A->SetAmbientVolume(.25f);
            S->SetTextureQuality(2);S->SetShadowQuality(1);S->SetViewDistanceQuality(2);S->SetVSyncEnabled(false);S->ApplyNonResolutionSettings();S->SaveSettings();
        }
        Check(I->Key("Jump")==EKeys::P && I->Key("CarHorn",true)==EKeys::Gamepad_Special_Left,TEXT("bindings_persist"));
        Check(FMath::IsNearlyEqual(I->Profile.MouseSensitivity,1.7f) && I->Profile.bInvertMouseY && FMath::IsNearlyEqual(I->Profile.GamepadSensitivity,1.5f) && FMath::IsNearlyEqual(I->Profile.GamepadDeadzone,.3f),TEXT("axes_persist"));
        Check(FMath::IsNearlyEqual(A->MasterVolume,.4f) && FMath::IsNearlyEqual(A->AmbientVolume,.25f),TEXT("audio_persist"));
        Check(S->GetTextureQuality()==2 && S->GetShadowQuality()==1 && S->GetViewDistanceQuality()==2 && !S->IsVSyncEnabled(),TEXT("graphics_persist"));
        auto* P=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(World,0));bool Found=false;
        if(P && P->GetInputMapping())for(const auto& M:P->GetInputMapping()->GetMappings())if(M.Action && M.Action->GetFName()==TEXT("Jump") && M.Key==EKeys::P)Found=true;
        Check(Found,TEXT("live_character_mapping"));
        UE_LOG(LogTemp,Display,TEXT("WTG_MENU_SETTINGS_DONE result=%s replay=%d"),OK?TEXT("PASS"):TEXT("FAIL"),Replay);
        FPlatformMisc::RequestExit(false);return false;
    }));
 }));
#endif
