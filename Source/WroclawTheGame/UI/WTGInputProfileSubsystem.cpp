#include "UI/WTGInputProfileSubsystem.h"
void UWTGInputProfileSubsystem::ResetToDefaults(){ Profile=FWTGInputProfile(); }
bool UWTGInputProfileSubsystem::Rebind(TMap<FName,FKey>& Map,FName ActionId,FKey Key){
 if(ActionId.IsNone()||!Key.IsValid())return false;
 for(const TPair<FName,FKey>& Pair:Map)if(Pair.Key!=ActionId&&Pair.Value==Key)return false;
 Map.Add(ActionId,Key);return true;
}
bool UWTGInputProfileSubsystem::RebindKeyboard(FName ActionId,FKey Key){return Rebind(Profile.KeyboardOverrides,ActionId,Key);}
bool UWTGInputProfileSubsystem::RebindGamepad(FName ActionId,FKey Key){return Rebind(Profile.GamepadOverrides,ActionId,Key);}
void UWTGInputProfileSubsystem::SetMouseSensitivity(float Value){Profile.MouseSensitivity=FMath::Clamp(Value,0.1f,5.0f);}
void UWTGInputProfileSubsystem::SetGamepadSensitivity(float Value){Profile.GamepadSensitivity=FMath::Clamp(Value,0.1f,5.0f);}
void UWTGInputProfileSubsystem::SetGamepadDeadzone(float Value){Profile.GamepadDeadzone=FMath::Clamp(Value,0.0f,0.5f);}
void UWTGInputProfileSubsystem::SetUIScale(float Value){Profile.UIScale=FMath::Clamp(Value,0.75f,1.5f);}
