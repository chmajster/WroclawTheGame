#include "Input/ControlSettings.h"

UWTGControlSettings* UWTGControlSettings::Get()
{
    return GetMutableDefault<UWTGControlSettings>();
}

void UWTGControlSettings::SetMouseSensitivity(float Sensitivity)
{
    MouseSensitivity = FMath::Clamp(Sensitivity, 0.25f, 3.0f);
    SaveConfig();
}

void UWTGControlSettings::SetInvertY(bool bEnabled)
{
    bInvertY = bEnabled;
    SaveConfig();
}

void UWTGControlSettings::ResetToDefaults()
{
    MouseSensitivity = 1.0f;
    bInvertY = false;
    SaveConfig();
}
