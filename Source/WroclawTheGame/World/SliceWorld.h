#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SliceWorld.generated.h"
UCLASS()
class WROCLAWTHEGAME_API ASliceWorld : public AActor {
 GENERATED_BODY()
public:
 ASliceWorld();virtual void BeginPlay() override;virtual void Tick(float DeltaSeconds) override;
private:
 UPROPERTY() TMap<FName,TObjectPtr<class UInstancedStaticMeshComponent>> Batches;
 UPROPERTY() TObjectPtr<class UPointLightComponent> PuzzleLight;
 UPROPERTY() TObjectPtr<class UAudioComponent> RoomAudio;
 UPROPERTY() TObjectPtr<class UAudioComponent> StreetAudio;
 UPROPERTY() TObjectPtr<class UAudioComponent> ChaseAudio;
 void Box(FVector Position,FVector Size,FName Material,FRotator Rotation=FRotator::ZeroRotator);
 void Sign(FVector Position,FRotator Rotation,const FString& Text,float Size=24);
 void Light(FVector Position,float Intensity,float Radius,FLinearColor Color);
 void Apartment();void Routes();void Outdoors();void Interactions();
 void AudioLoop(TObjectPtr<UAudioComponent>& Component,const TCHAR* Name,float Volume);
};
