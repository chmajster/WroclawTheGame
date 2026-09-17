#include "Audio/SliceAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Mission/SliceMission.h"
#include "Engine/GameInstance.h"
void USliceAudio::Play(const UObject* Context,const FString& Name,const FVector& Location,float Volume) {
 auto* Instance=UGameplayStatics::GetGameInstance(Context); if(!Instance) return;
 auto* M=Instance->GetSubsystem<USliceMission>();
 auto& Sound=M->Sounds.FindOrAdd(Name);
 if(!Sound) {
  const FString Path=FString::Printf(TEXT("/Game/Generated/Audio/%s.%s"),*Name,*Name);
  Sound=LoadObject<USoundBase>(nullptr,*Path);
 }
 if(Sound) UGameplayStatics::PlaySoundAtLocation(Context,Sound,Location,Volume);
}
