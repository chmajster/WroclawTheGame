#include "Mission/DistrictDeliveryPoint.h"
#include "Mission/DistrictDeliverySubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
ADistrictDeliveryPoint::ADistrictDeliveryPoint()
{
    Capsule=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));SetRootComponent(Capsule);
    Capsule->InitCapsuleSize(34,95);Capsule->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Visual=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Visual"));Visual->SetupAttachment(Capsule);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);Visual->SetRelativeLocation(FVector(0,0,-95));
    Visual->SetRelativeRotation(FRotator(0,-90,0));
    Name=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name"));Name->SetupAttachment(Capsule);
    Name->SetRelativeLocation(FVector(0,0,115));Name->SetWorldSize(20);Name->SetHorizontalAlignment(EHTA_Center);
}
void ADistrictDeliveryPoint::Initialize(bool IsReceiver,USkeletalMesh* Mesh,UAnimationAsset* Idle)
{
    bReceiver=IsReceiver;Visual->SetSkeletalMesh(Mesh);
    if (Idle) Visual->PlayAnimation(Idle,true);
    Name->SetText(FText::FromString(IsReceiver?TEXT("Olek · bulwar"):TEXT("Maja · sąsiedzka pracownia")));
}
FText ADistrictDeliveryPoint::Prompt(ASliceCharacter*) const
{
    auto* Session=GetWorld()->GetSubsystem<UDistrictDeliverySubsystem>();
    if (Session->State.Stage==Wroclaw::DeliveryStage::Complete) return FText::FromString(TEXT("Przesyłka doręczona. Dzięki!"));
    return FText::FromString(bReceiver?TEXT("E — doręcz przesyłkę"):TEXT("E — porozmawiaj z Mają"));
}
void ADistrictDeliveryPoint::Interact(ASliceCharacter* Player) { GetWorld()->GetSubsystem<UDistrictDeliverySubsystem>()->Interact(bReceiver,Player); }
