#include "Audio/AudioSettings.h"

UWTGAudioSettings* UWTGAudioSettings::Get()
{
    return GetMutableDefault<UWTGAudioSettings>();
}

void UWTGAudioSettings::SetSFXVolume(float Volume)
{
    SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::SetUIVolume(float Volume)
{
    UIVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::ResetToDefaults()
{
    SFXVolume = 1.0f;
    UIVolume = 1.0f;
    SaveConfig();
}
