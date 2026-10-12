#include "Mission/DistrictDeliverySubsystem.h"
#include "Mission/DistrictDeliveryPoint.h"
#include "Geography/DistrictRegistry.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Systems/WTGEconomySubsystem.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Vehicles/DriveableVehicle.h"
#include "Components/BoxComponent.h"
#include "Components/GameplayComponents.h"
#include "World/CityPopulation.h"
#include "UI/SliceController.h"
#include "Mission/SliceMission.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
const FString UDistrictDeliverySubsystem::RewardId=TEXT("odra.delivery.completed.v1");
void UDistrictDeliverySubsystem::CreatePoints()
{
    auto* Catalog=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Catalog.Get();
    auto Spawn=[&](bool IsReceiver,const FVector& Position,FName BodyId)
    {
        FHitResult Floor;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(DeliveryPointFloor),false);
        const bool Found=GetWorld()->LineTraceSingleByChannel(Floor,Position+FVector(0,0,250),Position-FVector(0,0,400),ECC_Visibility,Params);
        if (!Found) return static_cast<ADistrictDeliveryPoint*>(nullptr);
        auto* Point=GetWorld()->SpawnActor<ADistrictDeliveryPoint>(Floor.ImpactPoint+FVector(0,0,97),FRotator(0,Registry->VehicleYaw,0));
        const auto* Body=Catalog->Body(BodyId);
        if (Point && Body) Point->Initialize(IsReceiver,Body->Mesh.LoadSynchronous(),Body->CreatorIdle.LoadSynchronous());
        return Point;
    };
    Giver=Spawn(false,Registry->MeetingPoint,TEXT("Female.Standard"));
    Receiver=Spawn(true,Registry->DeliveryPoint,TEXT("Male.Standard"));
    UE_LOG(LogTemp,Display,TEXT("WTG_DELIVERY_POINTS_READY giver=%d receiver=%d reward=%lld"),IsValid(Giver),IsValid(Receiver),Registry->DeliveryRewardCents);
}
void UDistrictDeliverySubsystem::Tick(float Dt)
{
    auto* Gameplay=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();
    if (!Registry)
    {
        Registry=Gameplay->GetRegistry();
        if (!Registry) return;
        CreatePoints();
        if (GetWorld()->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>()->HasAppliedTransaction(RewardId)) State.Stage=Wroclaw::DeliveryStage::Complete;
    }
    if (UGameplayStatics::IsGamePaused(this)) return;
    auto* Car=Registry->DeliveryVehicle.Get();
    auto* M=GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (IsValid(Car) && Car->Driver) State.EnterCar();
    const bool Lost=!IsValid(Car) || Car->Health<=0 || Car->GetActorLocation().Z< -500 || Gameplay->RecoverySerial!=RecoveryAtStart;
    const bool Arrived=IsValid(Car) && FVector::Dist2D(Car->GetActorLocation(),Registry->DeliveryPoint)<900;
    const auto Before=State.Stage;
    State.Tick(Dt,M->bDead,Lost,Arrived,IsValid(Car) && FMath::Abs(Car->Speed)<100);
    if (Before!=State.Stage)
    {
        if (State.Stage==Wroclaw::DeliveryStage::Failed)
        {
            FailureReason=M->bDead?TEXT("Postać straciła przytomność"):(Lost?TEXT("Samochód lub trasa zostały utracone"):TEXT("Minął czas dostawy"));
            M->Notify(FailureReason+TEXT(". R — ponów dostawę."));
        }
        else M->Notify(Objective());
    }
}
void UDistrictDeliverySubsystem::Interact(bool IsReceiver,ASliceCharacter* Player)
{
    if (!Registry || !Player) return;
    auto* M=GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (State.Stage==Wroclaw::DeliveryStage::Complete) {M->Notify(TEXT("Olek: Nagłośnienie znów działa. Dzięki za pomoc!"));return;}
    if (!IsReceiver)
    {
        if (FVector::Dist2D(Player->GetActorLocation(),Registry->MeetingPoint)>240) {M->Notify(TEXT("Podejdź do Mai przy pracowni."));return;}
        if (State.Stage==Wroclaw::DeliveryStage::Failed) {Restart();return;}
        if (State.Start(Registry->DeliveryTimeLimit))
        {
            RecoveryAtStart=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->RecoverySerial;
            M->Notify(TEXT("Maja: Zawieź moduł nagłośnienia Olkowi na bulwar. Kluczyki są w naszym aucie."));
        }
        else M->Notify(Objective());
        return;
    }
    auto* Car=Registry->DeliveryVehicle.Get();
    if (!State.CanDeliver(UGameplayStatics::GetPlayerPawn(this,0)==Player,FVector::Dist2D(Player->GetActorLocation(),Registry->DeliveryPoint)<240,IsValid(Car) && FVector::Dist2D(Car->GetActorLocation(),Registry->DeliveryPoint)<900,IsValid(Car) && FMath::Abs(Car->Speed)<100,GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->WantedLevel()))
    {M->Notify(TEXT("Przywieź auto, zatrzymaj je przy bulwarze i podejdź do Olka. Zgub ewentualny pościg."));return;}
    auto* Economy=GetWorld()->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>();
    const bool Applied=Economy->HasAppliedTransaction(RewardId) || Economy->ApplyTransaction(RewardId,Registry->DeliveryRewardCents,TEXT("Przesyłka znad Odry"));
    if (State.Complete(Applied)) M->Notify(FString::Printf(TEXT("Olek: Uratowałeś sąsiedzki koncert. Dostawa ukończona! +%.2f zł"),Registry->DeliveryRewardCents/100.0));
    else M->Notify(TEXT("Nie udało się rozliczyć dostawy. Spróbuj ponownie."));
}
bool UDistrictDeliverySubsystem::Restart()
{
    if (!Registry) return false;
    auto* Gameplay=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();
    auto* M=GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>();
    if (M->bDead && State.Active()) State.Tick(0,true,false,false,false);
    if (State.Stage!=Wroclaw::DeliveryStage::Failed) return false;
    Gameplay->RecoverPlayer();
    auto* Car=Registry->DeliveryVehicle.Get();
    if (!IsValid(Car))
    {
        Car=GetWorld()->SpawnActor<ADriveableVehicle>(Registry->VehicleSpawn,FRotator(0,Registry->VehicleYaw,0));
        Registry->DeliveryVehicle=Car;
        if (Car && Registry->Population) Car->SetVisualMeshes(Registry->Population->TrafficBodyMesh,Registry->Population->TrafficWheelMesh);
    }
    if (!Car) return false;
    if (Car->Driver && !Car->Exit()) return false;
    Car->Chassis->SetSimulatePhysics(false);
    Car->SetActorLocationAndRotation(Registry->VehicleSpawn,FRotator(0,Registry->VehicleYaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Car->Health=100;Car->Speed=0;Car->bEngine=false;
    FailureReason.Empty();State.Start(Registry->DeliveryTimeLimit);RecoveryAtStart=Gameplay->RecoverySerial;
    if (auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(this,0))) PC->ResumeGame();
    M->Notify(TEXT("Nowa próba dostawy. Auto czeka przy pracowni."));return true;
}
FVector UDistrictDeliverySubsystem::Target() const
{
    if (!Registry) return FVector::ZeroVector;
    if (State.Stage==Wroclaw::DeliveryStage::GetVehicle || (State.Stage==Wroclaw::DeliveryStage::Drive && !Cast<ADriveableVehicle>(UGameplayStatics::GetPlayerPawn(this,0))))
        return IsValid(Registry->DeliveryVehicle)?Registry->DeliveryVehicle->GetActorLocation():Registry->VehicleSpawn;
    if (State.Stage==Wroclaw::DeliveryStage::Drive || State.Stage==Wroclaw::DeliveryStage::Deliver) return Registry->DeliveryPoint;
    return Registry->MeetingPoint;
}
FString UDistrictDeliverySubsystem::Objective() const
{
    switch(State.Stage)
    {
        case Wroclaw::DeliveryStage::Meet:return TEXT("Przesyłka znad Odry — porozmawiaj z Mają przy pracowni");
        case Wroclaw::DeliveryStage::GetVehicle:return TEXT("Dotrzyj do samochodu pracowni i wsiądź (E)");
        case Wroclaw::DeliveryStage::Drive:return Cast<ADriveableVehicle>(UGameplayStatics::GetPlayerPawn(this,0))?TEXT("Zawieź moduł nagłośnienia na bulwar"):TEXT("Wróć do auta; przesyłka nadal czeka w bagażniku");
        case Wroclaw::DeliveryStage::Deliver:return TEXT("Zatrzymaj auto, wysiądź (E) i doręcz paczkę Olkowi (E)");
        case Wroclaw::DeliveryStage::Complete:return TEXT("Przesyłka znad Odry — dostawa ukończona");
        case Wroclaw::DeliveryStage::Failed:return FailureReason+TEXT(" · R — ponów dostawę");
    }
    return FString();
}
