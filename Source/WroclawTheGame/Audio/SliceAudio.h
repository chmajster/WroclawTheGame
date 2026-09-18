#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SliceAudio.generated.h"
UCLASS()
class WROCLAWTHEGAME_API USliceAudio : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
  public:
    static void Play(const UObject *Context, const FString &Name, const FVector &Location,
                     float Volume = 0.7f);
    static void PlayUI(const UObject *Context, const FString &Name, float Volume = 0.35f);
};
