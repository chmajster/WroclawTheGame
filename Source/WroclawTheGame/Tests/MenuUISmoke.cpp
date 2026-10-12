#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "UI/Menu/MenuShell.h"
#include "UI/Menu/MenuMap.h"
#include "UI/SliceController.h"
#include "UI/WTGInputProfileSubsystem.h"
#include "Save/DistrictSaveSubsystem.h"
#include "Save/DistrictSaveGame.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Layout/WidgetPath.h"

namespace WTGMenuQA
{
TSharedPtr<SWidget> Find(TSharedRef<SWidget> Root,TFunction<bool(const TSharedRef<SWidget>&)> Match)
{
    if(Match(Root))return Root;
    FChildren* Children=Root->GetChildren();for(int32 I=0;I<Children->Num();++I)if(auto Found=Find(Children->GetChildAt(I),Match))return Found;return nullptr;
}
bool HasText(TSharedRef<SWidget> Root,const FString& Label)
{
    return Find(Root,[&](const TSharedRef<SWidget>& W){return W->GetTypeAsString()==TEXT("STextBlock") && StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Label;}).IsValid();
}
TSharedPtr<SButton> Button(UMenuFlowSubsystem* Menu,const FString& Label)
{
    if(!Menu->SlateMenu())return nullptr;
    auto Found=Find(Menu->SlateMenu().ToSharedRef(),[&](const TSharedRef<SWidget>& W){return W->GetTypeAsString()==TEXT("SButton") && W->IsEnabled() && HasText(W,Label);});
    return Found?StaticCastSharedPtr<SButton>(Found):nullptr;
}
void Key(FKey K)
{
    FKeyEvent E(K,FModifierKeysState(),0,false,0,0);FSlateApplication::Get().ProcessKeyDownEvent(E);FSlateApplication::Get().ProcessKeyUpEvent(E);
}
bool Click(UMenuFlowSubsystem* Menu,const FString& Label,bool Mouse=false,FKey Accept=EKeys::Enter)
{
    auto B=Button(Menu,Label);if(!B)return false;FSlateApplication::Get().SetKeyboardFocus(B);
    if(!Mouse){Key(Accept);return true;}
    const auto G=B->GetCachedGeometry();const auto P=G.LocalToAbsolute(G.GetLocalSize()*.5);
    FPointerEvent Down(0,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState());
    FPointerEvent Up(0,P,P,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
    FWidgetPath Path;FSlateApplication::Get().GeneratePathToWidgetUnchecked(B.ToSharedRef(),Path);
    FSlateApplication::Get().ProcessReply(Path,B->OnMouseButtonDown(G,Down),&Path,&Down);
    FSlateApplication::Get().ProcessReply(Path,B->OnMouseButtonUp(G,Up),&Path,&Up);return true;
}
void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Tests")/Name,true,false);}
}

// Real Slate controls/key routing/pointer handlers and game backend. This probe
// only owns two explicitly named QA saves and requires an isolated settings INI.
static FAutoConsoleCommandWithWorld GMenuUISmoke(TEXT("WTG.MenuUISmoke"),TEXT("Menu interaction/video revert; isolated MenuQA INI required."),
 FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W){
    if(!W || !W->IsGameWorld())return;
    const FString Ini=FConfigCacheIni::GetDestIniFilename(TEXT("GameUserSettings"),nullptr,*FPaths::GeneratedConfigDir());
    if(!FPaths::ConvertRelativePathToFull(Ini).StartsWith(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("MenuQA/")))){UE_LOG(LogTemp,Error,TEXT("WTG_MENU_UI_DONE result=FAIL isolated_ini_required"));FPlatformMisc::RequestExit(false);return;}
    const FString Slot=TEXT("WroclawDistrict_MenuQA"),Broken=TEXT("WroclawDistrict_MenuQACorrupt");
    if(UGameplayStatics::DoesSaveGameExist(Slot,0) || UGameplayStatics::DoesSaveGameExist(Broken,0)){UE_LOG(LogTemp,Error,TEXT("WTG_MENU_UI_DONE result=FAIL existing_qa_slot_preserved"));FPlatformMisc::RequestExit(false);return;}
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak=TWeakObjectPtr<UWorld>(W),Slot,Broken,Stage=0,Age=0.f,Total=0.f,OK=true,Frozen=0.,FrozenStats=0.,SavedTime=0.,PreviousSlot=FString(),Position=FVector::ZeroVector,OriginalSize=FIntPoint::ZeroValue](float Dt)mutable{
        UWorld* World=Weak.Get();if(!World)return false;Age+=Dt;Total+=Dt;
        auto* Menu=World->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();auto* D=World->GetSubsystem<UDistrictSaveSubsystem>();auto* S=GEngine->GetGameUserSettings();
        auto Check=[&](bool V,const TCHAR* Name){OK&=V;UE_LOG(LogTemp,Display,TEXT("WTG_MENU_UI_CHECK stage=%d %s=%s"),Stage,Name,V?TEXT("PASS"):TEXT("FAIL"));};
        auto Click=[&](const FString& Label,bool Mouse=false,FKey Accept=EKeys::Enter){const bool V=WTGMenuQA::Click(Menu,Label,Mouse,Accept);Check(V,*Label);};
        if(Stage==0){if(Menu->Screen!=EWTGMenuScreen::Main || Total<4){if(Total<50)return true;Check(false,TEXT("startup_timeout"));}PreviousSlot=D->Slot;D->Slot=Slot;
            World->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>()->ResetToDefaults();S->SetScreenResolution(FIntPoint(1600,900));S->SetFullscreenMode(EWindowMode::Windowed);S->ApplyResolutionSettings(false);S->ConfirmVideoMode();OriginalSize=S->GetScreenResolution();}
        else if(Age<.7f || Menu->IsTransitioning())return true;
        switch(Stage){
        case 0:Check(Menu->Screen==EWTGMenuScreen::Main && World->IsPaused() && Menu->Controller()->IsMoveInputIgnored() && Menu->Controller()->IsLookInputIgnored(),TEXT("startup_input_lock"));WTGMenuQA::Shot(TEXT("MenuQA_Main.png"));break;
        case 1:Click(TEXT("Nowa gra"));Check(WTGMenuQA::Button(Menu,TEXT("Potwierdź")).IsValid(),TEXT("new_confirmation"));break;
        case 2:WTGMenuQA::Key(EKeys::Escape);Check(!UGameplayStatics::DoesSaveGameExist(Slot,0) && Menu->Screen==EWTGMenuScreen::Main,TEXT("cancel_preserves_save"));break;
        case 3:Click(TEXT("Nowa gra"),true);break;
        case 4:Click(TEXT("Potwierdź"));break;
        case 5:Check(Menu->Screen==EWTGMenuScreen::Playing && !World->IsPaused() && !Menu->Controller()->IsMoveInputIgnored() && !Menu->Controller()->IsLookInputIgnored() && D->HasSave(),TEXT("new_actual_session"));Menu->Controller()->Escape();Frozen=World->GetTimeSeconds();FrozenStats=Menu->PlayedSeconds;Position=Menu->Controller()->GetPawn()->GetActorLocation();break;
        case 6:Check(Menu->Screen==EWTGMenuScreen::Pause && World->IsPaused() && FMath::IsNearlyEqual(World->GetTimeSeconds(),Frozen,.01) && FMath::IsNearlyEqual(Menu->PlayedSeconds,FrozenStats,.01) && Menu->Controller()->GetPawn()->GetActorLocation().Equals(Position,.1),TEXT("pause_frozen"));WTGMenuQA::Shot(TEXT("MenuQA_Map.png"));break;
        case 7:{auto Found=WTGMenuQA::Find(Menu->SlateMenu().ToSharedRef(),[](const TSharedRef<SWidget>& X){return X->GetTypeAsString()==TEXT("SMenuMap");});Check(Found.IsValid(),TEXT("map_widget"));
            if(Found){auto Map=StaticCastSharedPtr<SMenuMap>(Found);const auto Center=Map->ViewCenter();const float Zoom=Map->ViewZoom();FSlateApplication::Get().SetKeyboardFocus(Map);WTGMenuQA::Key(EKeys::Up);WTGMenuQA::Key(EKeys::Add);
                Check(Map->ViewCenter()!=Center && Map->ViewZoom()>Zoom,TEXT("map_keyboard_pan_zoom"));const auto G=Map->GetCachedGeometry();const auto P=G.LocalToAbsolute(G.GetLocalSize()*.5);
                FPointerEvent Down(0,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()),Up(0,P,P,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState());
                Map->OnMouseButtonDown(G,Down);Map->OnMouseButtonUp(G,Up);Check(Menu->MapWaypoint.IsSet(),TEXT("map_mouse_waypoint"));}break;}
        case 8:Click(TEXT("GRA"),true);Check(WTGMenuQA::HasText(Menu->SlateMenu().ToSharedRef(),TEXT("Twoja rozgrywka")),TEXT("mouse_tab"));break;
        case 9:Click(TEXT("Zapisz grę"));SavedTime=Menu->PlayedSeconds;break;
        case 10:{auto* Data=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));Check(Data && FMath::IsNearlyEqual(Data->PlayedSeconds,SavedTime,.01),TEXT("save_real_stats"));Menu->PlayedSeconds=0;Click(TEXT("Wczytaj zapis"));break;}
        case 11:Click(TEXT("Wczytaj"));break;
        case 12:Click(TEXT("Potwierdź"));break;
        case 13:Check(Menu->Screen==EWTGMenuScreen::Playing && Menu->PlayedSeconds>=SavedTime && D->Slot==Slot,TEXT("load_restores_stats_session"));Menu->Controller()->Escape();break;
        case 14:{const TCHAR* Labels[]={TEXT("GRA"),TEXT("POSTAĆ"),TEXT("STATYSTYKI"),TEXT("USTAWIENIA"),TEXT("STEROWANIE"),TEXT("MAPA")};const int32 Pages[]={0,11,12,2,3,10};
            for(int32 Pass=0;Pass<3;++Pass)for(int32 I=0;I<6;++I){Click(Labels[I]);Check(Menu->SlateMenu()->ActivePage()==Pages[I],TEXT("tab_content"));}Click(TEXT("USTAWIENIA"));break;}
        case 15:WTGMenuQA::Shot(TEXT("MenuQA_Settings.png"));Menu->SlateMenu()->PreviewVideoMode(FIntPoint(1280,720),2);break;
        case 16:Check(S->GetScreenResolution()==FIntPoint(1280,720) && Menu->SlateMenu()->IsVideoConfirmationPending(),TEXT("video_preview"));WTGMenuQA::Key(EKeys::Escape);break;
        case 17:Check(S->GetScreenResolution()==OriginalSize && !Menu->SlateMenu()->IsVideoConfirmationPending(),TEXT("video_cancel_reverts"));Menu->SlateMenu()->PreviewVideoMode(FIntPoint(1280,720),2);break;
        case 18:if(Age<16)return true;Check(S->GetScreenResolution()==OriginalSize && !Menu->SlateMenu()->IsVideoConfirmationPending() && !WTGMenuQA::Button(Menu,TEXT("Potwierdź")),TEXT("video_timeout_reverts"));Menu->SlateMenu()->PreviewVideoMode(FIntPoint(1280,720),2);break;
        case 19:Click(TEXT("Potwierdź"));Check(S->GetLastConfirmedScreenResolution()==FIntPoint(1280,720),TEXT("video_confirm_saved"));Click(TEXT("STEROWANIE"));break;
        case 20:WTGMenuQA::Shot(TEXT("MenuQA_Controls.png"));Click(TEXT("SPACJA"));break;
        case 21:WTGMenuQA::Key(EKeys::E);Check(World->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>()->Key("Jump")==EKeys::SpaceBar,TEXT("ui_rebind_conflict"));WTGMenuQA::Key(EKeys::O);Check(World->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>()->Key("Jump")==EKeys::O,TEXT("ui_rebind_saved"));break;
        case 22:Click(TEXT("O"));WTGMenuQA::Key(EKeys::Escape);Check(World->GetGameInstance()->GetSubsystem<UWTGInputProfileSubsystem>()->Key("Jump")==EKeys::O,TEXT("capture_cancel"));
            {auto Before=FSlateApplication::Get().GetKeyboardFocusedWidget();WTGMenuQA::Key(EKeys::Gamepad_DPad_Down);Check(Before!=FSlateApplication::Get().GetKeyboardFocusedWidget(),TEXT("pad_focus"));}break;
        case 23:Click(TEXT("MAPA"),false,EKeys::Gamepad_FaceButton_Bottom);WTGMenuQA::Key(EKeys::Escape);break;
        case 24:Check(Menu->Screen==EWTGMenuScreen::Playing && !World->IsPaused() && GEngine->GetMaxFPS()>30,TEXT("resume_restores_frame_budget"));Menu->ApplicationActivationChanged(false);Check(Menu->Screen==EWTGMenuScreen::Pause && World->IsPaused(),TEXT("focus_loss_pause"));Click(TEXT("GRA"));break;
        case 25:Click(TEXT("Menu główne"));break;
        case 26:Click(TEXT("Potwierdź"));break;
        case 27:Check(Menu->Screen==EWTGMenuScreen::Main && World->IsPaused(),TEXT("return_main"));FFileHelper::SaveStringToFile(TEXT("corrupt QA file"),*(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Broken+TEXT(".sav"))));Menu->SlateMenu()->Refresh();Click(TEXT("Kontynuuj"));break;
        case 28:Click(TEXT("Potwierdź"));break;
        case 29:Check(Menu->Screen==EWTGMenuScreen::Playing && Menu->PlayedSeconds>=SavedTime,TEXT("continue_skips_corrupt"));Menu->Controller()->Escape();Click(TEXT("GRA"));break;
        case 30:Click(TEXT("POSTAĆ"));break;
        case 31:WTGMenuQA::Shot(TEXT("MenuQA_Character.png"));Click(TEXT("GRA"));Click(TEXT("Wyjdź z gry"));break;
        case 32:UGameplayStatics::DeleteGameInSlot(Slot,0);UGameplayStatics::DeleteGameInSlot(Broken,0);D->Slot=PreviousSlot;
            UE_LOG(LogTemp,Display,TEXT("WTG_MENU_UI_DONE result=%s elapsed=%.2f"),OK?TEXT("PASS"):TEXT("FAIL"),Total);Click(TEXT("Potwierdź"));return false;
        }
        if(!OK){UGameplayStatics::DeleteGameInSlot(Slot,0);UGameplayStatics::DeleteGameInSlot(Broken,0);D->Slot=PreviousSlot;
            UE_LOG(LogTemp,Error,TEXT("WTG_MENU_UI_DONE result=FAIL stage=%d"),Stage);FPlatformMisc::RequestExit(false);return false;}
        ++Stage;Age=0;return true;
    }));
 }));
#endif
