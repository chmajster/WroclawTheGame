#include "Components/GameplayComponents.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PrimitiveComponent.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "Interaction/Interactable.h"
#include "Framework/GameplayFramework.h"
#include "Systems/GameplayEventBus.h"
#include "Systems/NoiseSystem.h"
namespace
{
USliceMission *Mission(const UActorComponent *C)
{
    return C->GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
}
} // namespace
bool UCombatComponent::Attack(bool Heavy)
{
    auto *Pawn = Cast<APawn>(GetOwner());
    auto *S = GetOwner()->FindComponentByClass<UStaminaComponent>();
    if (!Pawn || !S)
        return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastAttack < (Heavy ? 1.1 : .65) || !S->Spend(Heavy ? HeavyCost : LightCost))
        return false;
    LastAttack = Now;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WTGMelee), false, Pawn);
    FHitResult Hit;
    const FVector Start = Pawn->GetActorLocation();
    if (GetWorld()->SweepSingleByChannel(Hit, Start, Start + Pawn->GetActorForwardVector() * Range,
                                         FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(32), Params))
        UGameplayStatics::ApplyDamage(Hit.GetActor(), Heavy ? HeavyDamage : LightDamage,
                                      Pawn->GetController(), Pawn, nullptr);
    GetWorld()->GetSubsystem<UNoiseSystem>()->Report(TEXT("Combat"), Start, Pawn);
    return true;
}
bool UCombatComponent::Dodge(const FVector &Direction)
{
    auto *C = Cast<ACharacter>(GetOwner());
    auto *S = GetOwner()->FindComponentByClass<UStaminaComponent>();
    if (!C || !S || C->GetCharacterMovement()->IsFalling() || !S->Spend(25))
        return false;
    InvulnerableUntil = GetWorld()->GetTimeSeconds() + .3;
    C->LaunchCharacter(Direction.GetSafeNormal() * 620 + FVector(0, 0, 60), true, true);
    return true;
}
float UCombatComponent::Receive(float Damage, AActor *Causer)
{
    if (GetWorld()->GetTimeSeconds() < InvulnerableUntil)
        return 0;
    auto *H = GetOwner()->FindComponentByClass<UHealthComponent>();
    auto *S = GetOwner()->FindComponentByClass<UStaminaComponent>();
    const bool Facing =
        Causer && FVector::DotProduct(
                      GetOwner()->GetActorForwardVector(),
                      (Causer->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal()) > .2;
    if (bBlocking && Facing && S && S->Spend(20))
        Damage *= .2f;
    return H ? H->Damage(Damage) : 0;
}
bool UHideableComponent::Enter(ASliceCharacter *P)
{
    if (!P || Occupant.IsValid() || P->Hiding.IsValid())
        return false;
    Occupant = P;
    P->Hiding = this;
    P->GetCharacterMovement()->StopMovementImmediately();
    P->GetCharacterMovement()->DisableMovement();
    P->SetActorHiddenInGame(true);
    P->SetActorEnableCollision(false);
    GetWorld()->GetSubsystem<UGameplayEventBus>()->Emit(TEXT("State.Player.Hidden"), HideId, 1,
                                                        P->GetActorLocation(), P);
    return true;
}
bool UHideableComponent::Leave()
{
    auto *P = Occupant.Get();
    if (!P)
        return false;
    P->SetActorEnableCollision(true);
    if (!P->TeleportTo(ExitLocation, P->GetActorRotation(), false, false))
    {
        P->SetActorEnableCollision(false);
        return false;
    }
    P->SetActorHiddenInGame(false);
    P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    P->Hiding.Reset();
    Occupant.Reset();
    return true;
}
bool UHideableComponent::Inspect(AActor *Observer, const FVector &LastKnown, float Suspicion)
{
    if (!Observer || !Occupant.IsValid() || Suspicion < .75f ||
        FVector::Dist(LastKnown, GetOwner()->GetActorLocation()) > 250 ||
        FVector::Dist(Observer->GetActorLocation(), GetOwner()->GetActorLocation()) > 180)
        return false;
    return Leave();
}
bool UPowerConsumerComponent::Powered() const
{
    auto *M = Mission(this);
    return (!RequiredPower.IsValid() || M->State.Tagged(TCHAR_TO_UTF8(*RequiredPower.ToString()))) &&
           (!DisabledBy.IsValid() || !M->State.Tagged(TCHAR_TO_UTF8(*DisabledBy.ToString())));
}
bool UDoorComponent::SetOpen(bool Open, bool Authorized)
{
    if (Open && !Authorized && State != EWTGDoorState::Unlocked && State != EWTGDoorState::Broken)
        return false;
    bOpen = Open;
    if (auto *Mesh = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
        Mesh->SetCollisionEnabled(Open ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    return true;
}
bool UBasePuzzleComponent::Submit(const FString &Answer)
{
    return Mission(this)->Submit(DefinitionId.ToString(), Answer) == Wroclaw::Result::Applied;
}
FString UBasePuzzleComponent::Hint() const
{
    auto *M = Mission(this);
    const auto *A = Wroclaw::Progress::Find(TCHAR_TO_UTF8(*DefinitionId.ToString()));
    return A ? UTF8_TO_TCHAR(Wroclaw::PuzzleFramework::Hint(*A, M->State).c_str()) : TEXT("");
}
bool UInventoryComponent::Has(FName Item) const
{
    if (const auto* Wardrobe=GetOwner()->FindComponentByClass<UWardrobeComponent>())
        if (Wardrobe->OwnedClothing().Contains(Item)) return true;
    return Mission(this)->State.Has(TCHAR_TO_UTF8(*Item.ToString()));
}
bool UInventoryComponent::Use(FName Item)
{
    if (!Mission(this)->State.Use(TCHAR_TO_UTF8(*Item.ToString())))
        return false;
    GetWorld()->GetSubsystem<UGameplayEventBus>()->Emit(TEXT("Event.Inventory.Used"), Item, 1,
                                                        GetOwner()->GetActorLocation(), GetOwner());
    return true;
}
bool UInventoryComponent::HasTag(FGameplayTag Tag) const
{
    for (const auto &I : Wroclaw::Items())
        if (Mission(this)->State.Has(I.id))
            for (const auto &T : I.tags)
                if (FGameplayTag::RequestGameplayTag(FName(UTF8_TO_TCHAR(T.c_str())), false).MatchesTag(Tag))
                    return true;
    return false;
}
AActor *UInteractionComponent::Find(const FVector &Start, const FVector &Direction) const
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(WTGInteraction), false, GetOwner());
    GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * 600, ECC_Visibility, Params);
    return Hit.GetActor() && Hit.GetActor()->Implements<UInteractable>() &&
                   FVector::Dist(GetOwner()->GetActorLocation(), Hit.ImpactPoint) < Range
               ? Hit.GetActor()
               : nullptr;
}
bool UInteractionComponent::Interact(AActor *Target)
{
    if (!Target || FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation()) > Range + 150)
        return false;
    auto *P = Cast<ASliceCharacter>(GetOwner());
    auto *I = Cast<IInteractable>(Target);
    if (!P || !I)
        return false;
    I->Interact(P);
    return true;
}
FString UFactionComponent::RelationTo(const UFactionComponent *Other) const
{
    if (!Other)
        return TEXT("Neutral");
    const std::string A = TCHAR_TO_UTF8(*FactionId.ToString()),
                      B = TCHAR_TO_UTF8(*Other->FactionId.ToString());
    if (A == B)
        return TEXT("Friendly");
    for (const auto &F : Wroclaw::Factions())
        if ((F.a == A && F.b == B) || (F.a == B && F.b == A))
            return UTF8_TO_TCHAR(F.relation.c_str());
    return TEXT("Neutral");
}
