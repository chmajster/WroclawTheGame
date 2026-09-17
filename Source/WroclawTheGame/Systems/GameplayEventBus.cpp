#include "Systems/GameplayEventBus.h"
void UGameplayEventBus::Emit(const TCHAR *Type, FName Subject, float Value, FVector Location, AActor *Source)
{
    FWTGGameplayEvent E;
    E.Type = FGameplayTag::RequestGameplayTag(FName(Type), false);
    E.Subject = Subject;
    E.Value = Value;
    E.Location = Location;
    E.Source = Source;
    OnEvent.Broadcast(E);
}
