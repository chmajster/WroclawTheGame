#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SliceHUD.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceHUD : public AHUD
{
    GENERATED_BODY()
  public:
    virtual void DrawHUD() override;

  private:
    void Text(const FString &Value, float X, float Y, float Scale = 1,
              const FLinearColor &Color = FLinearColor::White);
    void DrawCityCoverage();
    void DrawFPSCounter();
    void Panel(const FString &Title, const FString &Body);
    float SmoothedFPS = 0.0f;
};
