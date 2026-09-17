#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SliceGameMode.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 ASliceGameMode();
 virtual void BeginPlay() override;virtual void Tick(float DeltaSeconds) override;
 bool HasThreat() const;FString ThreatText() const;
 UPROPERTY() TObjectPtr<class ASliceEnemy> Enemy;
 UPROPERTY() TArray<TObjectPtr<class ASliceEnemy>> Guards;
private:
 void SpawnGuard(const FString& Id,const FVector& Position,const TArray<FVector>& Patrol);
};
