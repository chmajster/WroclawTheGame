#include "UI/WTGInputProfileSubsystem.h"
#include "Misc/ConfigCacheIni.h"
void UWTGInputProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
 Super::Initialize(Collection);
 float Value=1;GConfig->GetFloat(TEXT("WTG.Input"),TEXT("MouseSensitivity"),Value,GGameUserSettingsIni);
 Profile.MouseSensitivity=FMath::IsFinite(Value)?FMath::Clamp(Value,.1f,5.f):1.f;
 GConfig->GetBool(TEXT("WTG.Input"),TEXT("InvertMouseY"),Profile.bInvertMouseY,GGameUserSettingsIni);
}
void UWTGInputProfileSubsystem::SaveMouseProfile()
{
 GConfig->SetFloat(TEXT("WTG.Input"),TEXT("MouseSensitivity"),Profile.MouseSensitivity,GGameUserSettingsIni);
 GConfig->SetBool(TEXT("WTG.Input"),TEXT("InvertMouseY"),Profile.bInvertMouseY,GGameUserSettingsIni);GConfig->Flush(false,GGameUserSettingsIni);
}
void UWTGInputProfileSubsystem::ToggleInvertMouse(){Profile.bInvertMouseY=!Profile.bInvertMouseY;SaveMouseProfile();}
void UWTGInputProfileSubsystem::ResetToDefaults(){ Profile=FWTGInputProfile();SaveMouseProfile(); }
bool UWTGInputProfileSubsystem::Rebind(TMap<FName,FKey>& Map,FName ActionId,FKey Key){
 if(ActionId.IsNone()||!Key.IsValid())return false;
 for(const TPair<FName,FKey>& Pair:Map)if(Pair.Key!=ActionId&&Pair.Value==Key)return false;
 Map.Add(ActionId,Key);return true;
}
bool UWTGInputProfileSubsystem::RebindKeyboard(FName ActionId,FKey Key){return Rebind(Profile.KeyboardOverrides,ActionId,Key);}
bool UWTGInputProfileSubsystem::RebindGamepad(FName ActionId,FKey Key){return Rebind(Profile.GamepadOverrides,ActionId,Key);}
void UWTGInputProfileSubsystem::SetMouseSensitivity(float Value){Profile.MouseSensitivity=FMath::IsFinite(Value)?FMath::Clamp(Value,0.1f,5.0f):1.f;SaveMouseProfile();}
void UWTGInputProfileSubsystem::SetGamepadSensitivity(float Value){Profile.GamepadSensitivity=FMath::Clamp(Value,0.1f,5.0f);}
void UWTGInputProfileSubsystem::SetGamepadDeadzone(float Value){Profile.GamepadDeadzone=FMath::Clamp(Value,0.0f,0.5f);}
void UWTGInputProfileSubsystem::SetUIScale(float Value){Profile.UIScale=FMath::Clamp(Value,0.75f,1.5f);}
