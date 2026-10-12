#include "Save/DistrictSaveSubsystem.h"
#include "UI/Menu/MenuFlowSubsystem.h"
#include "Save/DistrictSaveGame.h"
#include "Systems/DistrictGameplaySubsystem.h"
#include "Mission/DistrictDeliverySubsystem.h"
#include "Geography/DistrictRegistry.h"
#include "Character/SliceCharacter.h"
#include "Character/CharacterCreatorSubsystem.h"
#include "Character/CharacterAppearanceComponent.h"
#include "Vehicles/DriveableVehicle.h"
#include "Vehicles/VehiclePursuitSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GameplayComponents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Mission/SliceMission.h"
#include "UI/SliceController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "World/CityPopulation.h"
#include "HAL/FileManager.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

static ADriveableVehicle* RestoreDistrictCar(AWTGDistrictRegistry* R)
{
    if(IsValid(R->DeliveryVehicle))return R->DeliveryVehicle;
    auto* Car=R->GetWorld()->SpawnActor<ADriveableVehicle>(R->VehicleSpawn,FRotator(0,R->VehicleYaw,0));
    R->DeliveryVehicle=Car;if(Car && R->Population)Car->SetVisualMeshes(R->Population->TrafficBodyMesh,R->Population->TrafficWheelMesh);return Car;
}

bool UDistrictSaveGame::Validate(const FVector& Min,const FVector& Max,const FString& Id) const
{
    auto Position=[&](const FVector& P){return !P.ContainsNaN() && P.X>=Min.X && P.X<=Max.X && P.Y>=Min.Y && P.Y<=Max.Y && P.Z>=-300 && P.Z<=3000;};
    if(Version!=1 || DistrictId!=Id || !Position(PlayerPosition) || !Position(CarPosition) || !FMath::IsFinite(PlayerYaw) || !FMath::IsFinite(CarYaw) ||
       !FMath::IsFinite(PlayerHealth) || PlayerHealth<=0 || PlayerHealth>100 || !FMath::IsFinite(CarHealth) || CarHealth<0 || CarHealth>100 || (bInVehicle && CarHealth<=0) ||
       DeliveryStage>5 || !FMath::IsFinite(Remaining) || Remaining<0 || Remaining>420 || (DeliveryStage>=1 && DeliveryStage<=3 && Remaining<=0) || Attempts>1000000 ||
       !FMath::IsFinite(WantedHeat) || WantedHeat<0 || WantedHeat>90 || Economy.Ledger.Num()>1024 || FailureReason.Len()>512 ||
       Appearance.CharacterAppearanceVersion!=1 || !FMath::IsFinite(Appearance.PlayerAppearanceData.Height) || Appearance.PlayerAppearanceData.Height<140 || Appearance.PlayerAppearanceData.Height>220) return false;
    if(!FMath::IsFinite(PlayedSeconds) || !FMath::IsFinite(WalkedCm) || !FMath::IsFinite(DrivenCm) || PlayedSeconds<0 || WalkedCm<0 || DrivenCm<0 || PlayedSeconds>1e12 || WalkedCm>1e15 || DrivenCm>1e15)return false;
    int64 Balance=25000;TSet<FString> IDs;bool Completed=false;
    for(const auto& T:Economy.Ledger)
    {
        if(T.TransactionId.IsEmpty() || T.TransactionId.Len()>256 || IDs.Contains(T.TransactionId) || T.AmountCents>10000000 || T.AmountCents< -10000000) return false;
        Balance+=T.AmountCents;if(Balance< -200000 || Balance!=T.BalanceAfterCents) return false;
        IDs.Add(T.TransactionId);
        if(T.TransactionId==UDistrictDeliverySubsystem::RewardId){if(T.AmountCents!=18000)return false;Completed=true;}
    }
    return Balance==Economy.BalanceCents && Completed==(DeliveryStage==4);
}
bool UDistrictSaveSubsystem::HasSave() const {return UGameplayStatics::DoesSaveGameExist(Slot,0);}
bool UDistrictSaveSubsystem::HasReadableHeader(const FString& SlotName)
{
    if(FPaths::GetCleanFilename(SlotName)!=SlotName)return false;
    const FString Path=FPaths::ProjectSavedDir()/TEXT("SaveGames")/(SlotName+TEXT(".sav"));
    const int64 Size=IFileManager::Get().FileSize(*Path);
    if(Size<128 || Size>16*1024*1024)return false;
    TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return false;
    int32 Offset=0;
    auto Int=[&](int32& V){if(Offset+4>Bytes.Num())return false;FMemory::Memcpy(&V,Bytes.GetData()+Offset,4);Offset+=4;return true;};
    auto SkipString=[&](int32 MaxChars){int32 N;if(!Int(N) || N< -MaxChars || N>MaxChars)return false;
        const int32 Count=N<0?-N*2:N;if(Offset+Count>Bytes.Num())return false;
        if(Count && (Bytes[Offset+Count-1]!=0 || (N<0 && Bytes[Offset+Count-2]!=0)))return false;Offset+=Count;return true;};
    int32 Tag=0,Version=0;if(!Int(Tag) || Tag!=0x53415647 || !Int(Version) || Version<2 || Version>3)return false;
    // GVAS v2/v3: package version, engine version/branch, optimized GUID versions.
    Offset+=(Version==3?8:4)+10;if(!SkipString(1024))return false;
    int32 Format=0,Count=0;if(!Int(Format) || Format!=3 || !Int(Count) || Count<0 || Count>4096 || Offset+Count*20>Bytes.Num())return false;
    Offset+=Count*20;int32 Chars=0;if(!Int(Chars) || Chars< -512 || Chars>512 || Chars==0)return false;
    const FString Expected=TEXT("/Script/WroclawTheGame.DistrictSaveGame");
    if(FMath::Abs(Chars)!=Expected.Len()+1)return false;
    const int32 Width=Chars<0?2:1;if(Offset+FMath::Abs(Chars)*Width>=Bytes.Num())return false;
    for(int32 I=0;I<=Expected.Len();++I){const uint16 Character=Bytes[Offset+I*Width]+(Width==2?uint16(Bytes[Offset+I*Width+1])<<8:0);
        if(Character!=(I==Expected.Len()?0:uint16(Expected[I])))return false;}
    return true;
}
void UDistrictSaveSubsystem::Report(const FString& Message)
{
    Status=Message;GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->Notify(Message);
}
UDistrictSaveGame* UDistrictSaveSubsystem::Capture() const
{
    auto* G=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();auto* R=G->GetRegistry();
    auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0);auto* Car=R?R->DeliveryVehicle.Get():nullptr;
    auto* Player=Cast<ASliceCharacter>(Pawn);if(auto* V=Cast<ADriveableVehicle>(Pawn))Player=V->Driver;
    if(!R || !Player || !IsValid(Car) || GetWorld()->GetGameInstance()->GetSubsystem<USliceMission>()->bDead)return nullptr;
    auto* D=NewObject<UDistrictSaveGame>();D->DistrictId=R->DistrictId;
    D->bInVehicle=Pawn==Car;D->PlayerPosition=D->bInVehicle?R->PlayerStart+FVector(0,0,104):Player->GetActorLocation();
    D->PlayerYaw=Player->GetActorRotation().Yaw;D->PlayerHealth=Player->HealthState->Value;
    D->CarPosition=Car->GetActorLocation();D->CarYaw=Car->GetActorRotation().Yaw;D->CarHealth=Car->Health;
    D->Economy=GetWorld()->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>()->ExportSaveState();
    auto* Delivery=GetWorld()->GetSubsystem<UDistrictDeliverySubsystem>();D->DeliveryStage=uint8(Delivery->State.Stage);
    D->Remaining=Delivery->State.Remaining;D->Attempts=Delivery->State.Attempts;D->FailureReason=Delivery->FailureReason;
    D->WantedHeat=G->Wanted.Heat;D->Appearance=GetWorld()->GetGameInstance()->GetSubsystem<UCharacterCreatorSubsystem>()->Committed;
    auto* Stats=GetWorld()->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();D->PlayedSeconds=Stats->PlayedSeconds;D->WalkedCm=Stats->WalkedCm;D->DrivenCm=Stats->DrivenCm;
    return D->Validate(R->BoundsMin,R->BoundsMax,R->DistrictId)?D:nullptr;
}
bool UDistrictSaveSubsystem::Save(bool Notify)
{
    auto* Data=bWriteBlocked?nullptr:Capture();
    bool OK=false;
    // Serialize to a sibling first. Windows replaces the slot in one operation,
    // preserving the previous save if serialization/write/rename fails.
    TArray<uint8> Bytes;
    if(Data && Slot.StartsWith(TEXT("WroclawDistrict")) && !Slot.Contains(TEXT("/")) && !Slot.Contains(TEXT("\\")) && !Slot.Contains(TEXT("..")) && UGameplayStatics::SaveGameToMemory(Data,Bytes))
    {
        const FString Target=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("SaveGames")/(Slot+TEXT(".sav")));
        const FString Temp=Target+TEXT(".pending");IFileManager::Get().MakeDirectory(*FPaths::GetPath(Target),true);
        if(FFileHelper::SaveArrayToFile(Bytes,*Temp))
        {
#if PLATFORM_WINDOWS
            OK=::MoveFileExW(*Temp,*Target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
            OK=UGameplayStatics::SaveGameToSlot(Data,Slot,0);
#endif
        }
        if(IFileManager::Get().FileExists(*Temp))IFileManager::Get().Delete(*Temp);
    }
    Status=OK?TEXT("Postęp zapisany"):TEXT("Nie można zapisać postępu");
    if(Notify)Report(Status);
    if(OK)AutoSaveSeconds=0;
    return OK;
}
bool UDistrictSaveSubsystem::Apply(const UDistrictSaveGame* D)
{
    auto* G=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();auto* R=G->GetRegistry();
    if(!R || !D || !D->Validate(R->BoundsMin,R->BoundsMax,R->DistrictId))return false;
    auto* Car=R->DeliveryVehicle.Get();auto* PC=Cast<ASliceController>(UGameplayStatics::GetPlayerController(this,0));
    auto* Player=PC?Cast<ASliceCharacter>(PC->GetPawn()):nullptr;
    if(auto* Current=PC?Cast<ADriveableVehicle>(PC->GetPawn()):nullptr)
    { Current->Speed=0;Current->Chassis->SetPhysicsLinearVelocity(FVector::ZeroVector);Current->Chassis->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
      if(!Current->Exit())return false;Player=Cast<ASliceCharacter>(PC->GetPawn()); }
    if(!Player)return false;
    if(!IsValid(Car))Car=RestoreDistrictCar(R);
    if(!Car)return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DistrictLoadFloor),false);Params.AddIgnoredActor(Player);Params.AddIgnoredActor(Car);
    FHitResult Floor;
    if(!GetWorld()->LineTraceSingleByChannel(Floor,D->PlayerPosition+FVector(0,0,150),D->PlayerPosition-FVector(0,0,450),ECC_Visibility,Params) || Floor.ImpactNormal.Z<.7)return false;
    const FVector Safe=Floor.ImpactPoint+FVector(0,0,Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4);
    if(GetWorld()->OverlapBlockingTestByChannel(Safe,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(),Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Params))return false;
    FHitResult CarFloor;
    if(!GetWorld()->LineTraceSingleByChannel(CarFloor,D->CarPosition+FVector(0,0,150),D->CarPosition-FVector(0,0,450),ECC_Visibility,Params) || CarFloor.ImpactNormal.Z<.7)return false;
    if(GetWorld()->OverlapBlockingTestByChannel(D->CarPosition,FQuat(FRotator(0,D->CarYaw,0)),ECC_WorldStatic,FCollisionShape::MakeBox(Car->Chassis->GetScaledBoxExtent()*.9f),Params)){
        TArray<FOverlapResult> Hits;
        GetWorld()->OverlapMultiByChannel(Hits,D->CarPosition,FQuat(FRotator(0,D->CarYaw,0)),ECC_WorldStatic,FCollisionShape::MakeBox(Car->Chassis->GetScaledBoxExtent()*.9f),Params);
        for(const auto& Hit:Hits)UE_LOG(LogTemp,Warning,TEXT("WTG_DISTRICT_LOAD_BLOCKER actor=%s component=%s type=%d blocking=%d"),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),Hit.GetComponent()?int(Hit.GetComponent()->GetCollisionObjectType()):-1,Hit.bBlockingHit);
        UE_LOG(LogTemp,Warning,TEXT("WTG_DISTRICT_LOAD_BLOCKED car=%s yaw=%.1f"),*D->CarPosition.ToString(),D->CarYaw);return false;
    }
    Car->SetAIControl(false);Car->Chassis->SetSimulatePhysics(false);Car->SetActorLocationAndRotation(D->CarPosition,FRotator(0,D->CarYaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Car->Health=D->CarHealth;Car->Speed=0;Car->bEngine=false;
    Player->SetActorLocationAndRotation(Safe,FRotator(0,D->PlayerYaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetCharacterMovement()->StopMovementImmediately();Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Player->HealthState->Value=D->PlayerHealth;
    auto* GI=GetWorld()->GetGameInstance();GI->GetSubsystem<UWTGEconomySubsystem>()->ImportSaveState(D->Economy);
    auto* Creator=GI->GetSubsystem<UCharacterCreatorSubsystem>();Creator->Restore(D->Appearance);Player->Appearance->ApplyAppearance(Creator->Committed.PlayerAppearanceData);
    auto* Delivery=GetWorld()->GetSubsystem<UDistrictDeliverySubsystem>();Delivery->State.Stage=Wroclaw::DeliveryStage(D->DeliveryStage);Delivery->State.Remaining=D->Remaining;Delivery->State.Attempts=D->Attempts;
    Delivery->FailureReason=D->FailureReason;Delivery->SynchronizeRecovery();
    GetWorld()->GetSubsystem<UVehiclePursuitSubsystem>()->StopPursuit();
    G->Wanted.Heat=D->WantedHeat;G->Wanted.QuietSeconds=0;
    GI->GetSubsystem<USliceMission>()->bDead=false;PC->ResumeGame();PC->SetControlRotation(FRotator(0,D->PlayerYaw,0));
    auto* Stats=GetWorld()->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();Stats->PlayedSeconds=D->PlayedSeconds;Stats->WalkedCm=D->WalkedCm;Stats->DrivenCm=D->DrivenCm;Stats->ResetTracking();
    if(D->bInVehicle)Car->Interact(Player);
    return !D->bInVehicle || Car->Driver==Player;
}
bool UDistrictSaveSubsystem::LoadSaved()
{
    auto* R=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>()->GetRegistry();
    if(!R || !HasSave()){Report(TEXT("Brak zapisu tej dzielnicy"));return false;}
    if(!HasReadableHeader(Slot)){bWriteBlocked=true;Report(TEXT("Niepoprawny format zapisu. Zachowano plik."));return false;}
    auto* Data=Cast<UDistrictSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot,0));
    if(!Data || !Data->Validate(R->BoundsMin,R->BoundsMax,R->DistrictId))
    {bWriteBlocked=true;Report(TEXT("Zapis jest uszkodzony lub nowszy. Zachowano plik; nowa gra utworzy nowy zapis."));return false;}
    if(!Apply(Data)){Report(TEXT("Wczytanie wymaga bezpiecznego miejsca dla postaci i auta"));return false;}
    bTriedAutoLoad=true;bWriteBlocked=false;AutoSaveSeconds=0;Report(TEXT("Postęp wczytany"));return true;
}
bool UDistrictSaveSubsystem::NewRun()
{
    auto* G=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();auto* R=G->GetRegistry();if(!R)return false;
    auto* Old=Capture();const bool Blocked=bWriteBlocked;
    G->RecoverPlayer();auto* Car=RestoreDistrictCar(R);if(!Car)return false;
    Car->Chassis->SetSimulatePhysics(false);Car->SetActorLocationAndRotation(R->VehicleSpawn,FRotator(0,R->VehicleYaw,0),false,nullptr,ETeleportType::TeleportPhysics);
    Car->Health=100;Car->Speed=0;Car->bEngine=false;G->Wanted={};
    GetWorld()->GetGameInstance()->GetSubsystem<UWTGEconomySubsystem>()->ResetEconomy();
    auto* Delivery=GetWorld()->GetSubsystem<UDistrictDeliverySubsystem>();Delivery->State={};Delivery->FailureReason.Empty();Delivery->SynchronizeRecovery();
    auto* Stats=GetWorld()->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>();Stats->PlayedSeconds=Stats->WalkedCm=Stats->DrivenCm=0;Stats->ResetTracking();
    bWriteBlocked=false;const bool OK=Save();if(OK)bTriedAutoLoad=true;if(!OK){if(Old)Apply(Old);bWriteBlocked=Blocked;}return OK;
}
void UDistrictSaveSubsystem::Tick(float Dt)
{
    if(GetWorld()->GetGameInstance()->GetSubsystem<UMenuFlowSubsystem>()->IsFrontEnd())return;
    auto* G=GetWorld()->GetSubsystem<UDistrictGameplaySubsystem>();if(!G->IsActive())return;
    if(!bTriedAutoLoad)
    {
        auto* Player=Cast<ASliceCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
        if(GetWorld()->GetTimeSeconds()<2 || !Player || Player->GetCharacterMovement()->MovementMode!=MOVE_Walking)return;
        bTriedAutoLoad=true;if(!FParse::Param(FCommandLine::Get(),TEXT("WTGNoDistrictAutoLoad")) && HasSave())LoadSaved();
    }
    if(UGameplayStatics::IsGamePaused(this) || FParse::Param(FCommandLine::Get(),TEXT("WTGNoDistrictAutoSave")))return;
    AutoSaveSeconds+=Dt;if(AutoSaveSeconds>=30){AutoSaveSeconds=0;Save(false);}
}
