#include "UI/WTGInputProfileSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "Kismet/GameplayStatics.h"
#include "Character/SliceCharacter.h"
#include "Vehicles/DriveableVehicle.h"
#include "Engine/GameInstance.h"
#include "UI/SliceController.h"

static bool WTGReservedInputKey(FKey K)
{
    return K==EKeys::Escape || K==EKeys::F5 || K==EKeys::F9 || K==EKeys::Enter || K==EKeys::N || K==EKeys::L || K==EKeys::Q || K==EKeys::Gamepad_Special_Right;
}
FString UWTGInputProfileSubsystem::DisplayKey(FKey K)
{
    static const TMap<FKey,FString> Names={
        {EKeys::Gamepad_FaceButton_Bottom,TEXT("PAD A")},{EKeys::Gamepad_FaceButton_Right,TEXT("PAD B")},
        {EKeys::Gamepad_FaceButton_Left,TEXT("PAD X")},{EKeys::Gamepad_FaceButton_Top,TEXT("PAD Y")},
        {EKeys::Gamepad_LeftShoulder,TEXT("PAD LB")},{EKeys::Gamepad_RightShoulder,TEXT("PAD RB")},
        {EKeys::Gamepad_LeftThumbstick,TEXT("PAD L3")},{EKeys::Gamepad_RightThumbstick,TEXT("PAD R3")},
        {EKeys::Gamepad_DPad_Up,TEXT("D-pad ↑")},{EKeys::Gamepad_DPad_Down,TEXT("D-pad ↓")},
        {EKeys::Gamepad_DPad_Left,TEXT("D-pad ←")},{EKeys::Gamepad_DPad_Right,TEXT("D-pad →")},
        {EKeys::Gamepad_Special_Left,TEXT("PAD WSTECZ")},{EKeys::SpaceBar,TEXT("SPACJA")},
        {EKeys::LeftMouseButton,TEXT("LPM")},{EKeys::RightMouseButton,TEXT("PPM")},{EKeys::MiddleMouseButton,TEXT("ŚPM")},
        {EKeys::LeftShift,TEXT("LEWY SHIFT")},{EKeys::LeftControl,TEXT("LEWY CTRL")},{EKeys::LeftAlt,TEXT("LEWY ALT")},
        {EKeys::NumPadZero,TEXT("NUM 0")},{EKeys::Add,TEXT("NUM +")},{EKeys::Multiply,TEXT("NUM *")}};
    if(const auto* Name=Names.Find(K))return *Name;return K.GetDisplayName().ToString();
}

const TArray<FWTGInputBinding>& UWTGInputProfileSubsystem::Bindings()
{
    static const TArray<FWTGInputBinding> Values={
        {"Forward",TEXT("Ruch do przodu"),EKeys::W,FKey(),"foot"},
        {"Backward",TEXT("Ruch do ty\u0142u"),EKeys::S,FKey(),"foot"},
        {"Left",TEXT("Ruch w lewo"),EKeys::A,FKey(),"foot"},
        {"Right",TEXT("Ruch w prawo"),EKeys::D,FKey(),"foot"},
        {"Interact",TEXT("Interakcja"),EKeys::E,EKeys::Gamepad_FaceButton_Bottom,"foot"},
        {"Jump",TEXT("Skok"),EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Left,"foot"},
        {"Sprint",TEXT("Sprint"),EKeys::LeftShift,EKeys::Gamepad_LeftThumbstick,"foot"},
        {"Crouch",TEXT("Kucanie"),EKeys::LeftControl,EKeys::Gamepad_RightThumbstick,"foot"},
        {"Phone",TEXT("Telefon"),EKeys::T,EKeys::Gamepad_DPad_Up,"foot"},
        {"Attack",TEXT("Atak"),EKeys::LeftMouseButton,EKeys::Gamepad_RightShoulder,"foot"},
        {"HeavyAttack",TEXT("Silny atak"),EKeys::MiddleMouseButton,EKeys::Gamepad_FaceButton_Top,"foot"},
        {"Block",TEXT("Blok"),EKeys::RightMouseButton,EKeys::Gamepad_LeftShoulder,"foot"},
        {"Dodge",TEXT("Unik"),EKeys::LeftAlt,EKeys::Gamepad_FaceButton_Right,"foot"},
        {"Throw",TEXT("Rzut"),EKeys::G,EKeys::Gamepad_DPad_Down,"foot"},
        {"Heal",TEXT("Leczenie"),EKeys::V,EKeys::Gamepad_DPad_Right,"foot"},
        {"Flashlight",TEXT("Latarka"),EKeys::F,EKeys::Gamepad_DPad_Left,"foot"},
        {"CarForward",TEXT("Samoch\u00f3d: gaz"),EKeys::W,FKey(),"vehicle"},
        {"CarBackward",TEXT("Samoch\u00f3d: hamowanie / cofanie"),EKeys::S,FKey(),"vehicle"},
        {"CarLeft",TEXT("Samoch\u00f3d: skr\u0119t w lewo"),EKeys::A,FKey(),"vehicle"},
        {"CarRight",TEXT("Samoch\u00f3d: skr\u0119t w prawo"),EKeys::D,FKey(),"vehicle"},
        {"CarExit",TEXT("Samoch\u00f3d: wysiadanie"),EKeys::E,EKeys::Gamepad_FaceButton_Bottom,"vehicle"},
        {"CarHandbrake",TEXT("Samoch\u00f3d: hamulec r\u0119czny"),EKeys::SpaceBar,EKeys::Gamepad_FaceButton_Left,"vehicle"},
        {"CarEngine",TEXT("Samoch\u00f3d: silnik"),EKeys::NumPadZero,EKeys::Gamepad_FaceButton_Top,"vehicle"},
        {"CarLights",TEXT("Samoch\u00f3d: \u015bwiat\u0142a"),EKeys::Add,EKeys::Gamepad_DPad_Up,"vehicle"},
        {"CarHorn",TEXT("Samoch\u00f3d: klakson"),EKeys::Multiply,EKeys::Gamepad_RightThumbstick,"vehicle"},
        {"Retry",TEXT("Ponów dostawę / odzyskaj postać"),EKeys::R,FKey(),"global"},
        {"QuestLog",TEXT("Dziennik zadań"),EKeys::B,FKey(),"global"},
        {"Inventory",TEXT("Ekwipunek"),EKeys::I,FKey(),"global"},
        {"Investigation",TEXT("Śledztwo"),EKeys::J,FKey(),"global"},
        {"Hint",TEXT("Wskazówka"),EKeys::H,FKey(),"global"},
        {"Achievements",TEXT("Osiągnięcia"),EKeys::K,FKey(),"global"},
        {"Context",TEXT("Akcja kontekstowa kampanii"),EKeys::C,FKey(),"global"}
    };
    return Values;
}
FKey UWTGInputProfileSubsystem::Key(FName Id,bool Gamepad) const
{
    const auto& Overrides=Gamepad?Profile.GamepadOverrides:Profile.KeyboardOverrides;
    if(const auto* V=Overrides.Find(Id))return *V;
    for(const auto& B:Bindings())if(B.Id==Id)return Gamepad?B.Gamepad:B.Keyboard;return FKey();
}
void UWTGInputProfileSubsystem::Initialize(FSubsystemCollectionBase& C)
{
    Super::Initialize(C);
    auto Read=[&](const TCHAR* K,float Default,float Min,float Max){float V=Default;GConfig->GetFloat(TEXT("WTG.Input"),K,V,GGameUserSettingsIni);return FMath::IsFinite(V)?FMath::Clamp(V,Min,Max):Default;};
    Profile.MouseSensitivity=Read(TEXT("MouseSensitivity"),1,.1f,5);
    Profile.GamepadSensitivity=Read(TEXT("GamepadSensitivity"),1,.1f,5);
    Profile.GamepadDeadzone=Read(TEXT("GamepadDeadzone"),.15f,0,.5f);
    Profile.UIScale=Read(TEXT("UIScale"),1,.75f,1.5f);
    GConfig->GetBool(TEXT("WTG.Input"),TEXT("InvertMouseY"),Profile.bInvertMouseY,GGameUserSettingsIni);
    for(const auto& B:Bindings())for(bool Pad:{false,true}){
        FString Value;const TCHAR* Section=Pad?TEXT("WTG.Input.Gamepad"):TEXT("WTG.Input.Keyboard");
        if(GConfig->GetString(Section,*B.Id.ToString(),Value,GGameUserSettingsIni)){
            const FKey K{FName(*Value)};
            if(K.IsValid() && !K.IsAnalog() && K.IsGamepadKey()==Pad && !WTGReservedInputKey(K) && (!Pad || B.Gamepad.IsValid()))
                (Pad?Profile.GamepadOverrides:Profile.KeyboardOverrides).Add(B.Id,K);
        }
    }
    // Reject hand-edited conflicts; valid profiles written by the UI already satisfy this.
    for(bool Pad:{false,true})for(const auto& B:Bindings()){
        auto& Overrides=Pad?Profile.GamepadOverrides:Profile.KeyboardOverrides;if(!Overrides.Contains(B.Id))continue;
        for(const auto& Other:Bindings())if(B.Id!=Other.Id && (B.Context==Other.Context || B.Context=="global" || Other.Context=="global") && Key(B.Id,Pad)==Key(Other.Id,Pad)){Overrides.Remove(B.Id);break;}
    }
}
void UWTGInputProfileSubsystem::SaveProfile()
{
    GConfig->SetFloat(TEXT("WTG.Input"),TEXT("MouseSensitivity"),Profile.MouseSensitivity,GGameUserSettingsIni);
    GConfig->SetBool(TEXT("WTG.Input"),TEXT("InvertMouseY"),Profile.bInvertMouseY,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("WTG.Input"),TEXT("GamepadSensitivity"),Profile.GamepadSensitivity,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("WTG.Input"),TEXT("GamepadDeadzone"),Profile.GamepadDeadzone,GGameUserSettingsIni);
    GConfig->SetFloat(TEXT("WTG.Input"),TEXT("UIScale"),Profile.UIScale,GGameUserSettingsIni);
    for(bool Pad:{false,true}){const TCHAR* S=Pad?TEXT("WTG.Input.Gamepad"):TEXT("WTG.Input.Keyboard");GConfig->EmptySection(S,GGameUserSettingsIni);
        for(const auto& B:(Pad?Profile.GamepadOverrides:Profile.KeyboardOverrides))GConfig->SetString(S,*B.Key.ToString(),*B.Value.GetFName().ToString(),GGameUserSettingsIni);}
    GConfig->Flush(false,GGameUserSettingsIni);
}
void UWTGInputProfileSubsystem::SaveMouseProfile(){SaveProfile();}
void UWTGInputProfileSubsystem::ApplyToPlayer()
{
    auto* Pawn=UGameplayStatics::GetPlayerPawn(GetGameInstance(),0);auto* P=Cast<ASliceCharacter>(Pawn);
    if(auto* Car=Cast<ADriveableVehicle>(Pawn))P=Car->Driver;if(P)P->ApplyInputProfile();
    if(auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(GetGameInstance(),0)))PC->ApplyShortcutProfile();
}
void UWTGInputProfileSubsystem::ToggleInvertMouse(){Profile.bInvertMouseY=!Profile.bInvertMouseY;SaveProfile();}
void UWTGInputProfileSubsystem::ResetToDefaults(){Profile=FWTGInputProfile();SaveProfile();ApplyToPlayer();}
bool UWTGInputProfileSubsystem::Rebind(TMap<FName,FKey>& Map,FName Id,FKey K)
{
    const bool Pad=&Map==&Profile.GamepadOverrides;
    const auto* Binding=Bindings().FindByPredicate([Id](const FWTGInputBinding& B){return B.Id==Id;});
    if(!Binding || !K.IsValid() || K.IsGamepadKey()!=Pad || K.IsAnalog() || WTGReservedInputKey(K))return false;
    if(Pad && !Binding->Gamepad.IsValid())return false;
    for(const auto& B:Bindings())if(B.Id!=Id && (B.Context==Binding->Context || B.Context=="global" || Binding->Context=="global") && Key(B.Id,Pad)==K)return false;
    Map.Add(Id,K);SaveProfile();ApplyToPlayer();return true;
}
bool UWTGInputProfileSubsystem::RebindKeyboard(FName Id,FKey K){return Rebind(Profile.KeyboardOverrides,Id,K);}
bool UWTGInputProfileSubsystem::RebindGamepad(FName Id,FKey K){return Rebind(Profile.GamepadOverrides,Id,K);}
void UWTGInputProfileSubsystem::SetMouseSensitivity(float V){Profile.MouseSensitivity=FMath::IsFinite(V)?FMath::Clamp(V,.1f,5.f):1;SaveProfile();}
void UWTGInputProfileSubsystem::SetGamepadSensitivity(float V){Profile.GamepadSensitivity=FMath::IsFinite(V)?FMath::Clamp(V,.1f,5.f):1;SaveProfile();}
void UWTGInputProfileSubsystem::SetGamepadDeadzone(float V){Profile.GamepadDeadzone=FMath::IsFinite(V)?FMath::Clamp(V,0.f,.5f):.15f;SaveProfile();ApplyToPlayer();}
void UWTGInputProfileSubsystem::SetUIScale(float V){Profile.UIScale=FMath::IsFinite(V)?FMath::Clamp(V,.75f,1.5f):1;SaveProfile();}
