#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SliceGameMode.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 ASliceGameMode();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UPROPERTY() TObjectPtr<class ASliceEnemy> Enemy;
};
