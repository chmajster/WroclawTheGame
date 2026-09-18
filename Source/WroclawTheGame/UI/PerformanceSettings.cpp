#include "UI/PerformanceSettings.h"

#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

UWTGPerformanceSettings* UWTGPerformanceSettings::Get()
{
    return GetMutableDefault<UWTGPerformanceSettings>();
}

void UWTGPerformanceSettings::SetShowFPS(bool bEnabled)
{
    bShowFPS = bEnabled;
    SaveConfig();
}

void UWTGPerformanceSettings::SetFPSLimit(int32 Limit)
{
    FPSLimit = Limit <= 0 ? 0 : FMath::Clamp(Limit, 30, 1000);
    SaveConfig();
    Apply();
}

void UWTGPerformanceSettings::SetReduceUIMotion(bool bEnabled)
{
    bReduceUIMotion = bEnabled;
    SaveConfig();
}

void UWTGPerformanceSettings::SetMenuBackgroundBlur(bool bEnabled)
{
    bMenuBackgroundBlur = bEnabled;
    SaveConfig();
}

void UWTGPerformanceSettings::SetUISounds(bool bEnabled)
{
    bUISounds = bEnabled;
    SaveConfig();
}

void UWTGPerformanceSettings::Apply() const
{
    if (!GEngine)
        return;

    if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
    {
        UserSettings->SetFrameRateLimit(FPSLimit > 0 ? static_cast<float>(FPSLimit) : 0.0f);
        UserSettings->SaveSettings();
    }
}

void UWTGPerformanceSettings::ResetToDefaults()
{
    bShowFPS = false;
    FPSLimit = 60;
    bReduceUIMotion = false;
    bMenuBackgroundBlur = true;
    bUISounds = true;
    SaveConfig();
    Apply();
}
