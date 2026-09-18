#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SliceGameMode.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceGameMode : public AGameModeBase
{
    GENERATED_BODY()
  public:
    ASliceGameMode();
    virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Relocate(const FVector &Location);
    bool HasThreat() const;
    FString ThreatText() const;
    bool IsReady() const
    {
        return bReady;
    }

  private:
    bool bReady = false;
    double Started = 0;
};
