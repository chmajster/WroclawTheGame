#include "Audio/WTGAudioSettings.h"

UWTGAudioSettings* UWTGAudioSettings::Get()
{
    auto* Settings = GetMutableDefault<UWTGAudioSettings>();
    Settings->MasterVolume = FMath::Clamp(Settings->MasterVolume, 0.0f, 1.0f);
    Settings->SFXVolume = FMath::Clamp(Settings->SFXVolume, 0.0f, 1.0f);
    Settings->MusicVolume = FMath::Clamp(Settings->MusicVolume, 0.0f, 1.0f);
    Settings->UIVolume = FMath::Clamp(Settings->UIVolume, 0.0f, 1.0f);
    return Settings;
}

void UWTGAudioSettings::SetMasterVolume(float Volume)
{
    MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::SetSFXVolume(float Volume)
{
    SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::SetMusicVolume(float Volume)
{
    MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::SetUIVolume(float Volume)
{
    UIVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
    SaveConfig();
}

void UWTGAudioSettings::ResetToDefaults()
{
    MasterVolume = 1.0f;
    SFXVolume = 1.0f;
    MusicVolume = 1.0f;
    UIVolume = 1.0f;
    SaveConfig();
}
