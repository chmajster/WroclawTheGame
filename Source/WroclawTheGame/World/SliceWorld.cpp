#include "World/SliceWorld.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Mission/SliceMission.h"
#include "Character/SliceCharacter.h"
#include "Core/SliceGameMode.h"
#include "AI/SliceEnemy.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
ASliceWorld::ASliceWorld() {
 PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=0.25;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 RootComponent->SetMobility(EComponentMobility::Static);
}
void ASliceWorld::Box(FVector Position,FVector Size,FName Material,FRotator Rotation) {
 auto& Batch=Batches.FindOrAdd(Material);
 if(!Batch) {
  Batch=NewObject<UInstancedStaticMeshComponent>(this,Material);Batch->SetupAttachment(RootComponent);AddInstanceComponent(Batch);
  Batch->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
  const FString Path=FString::Printf(TEXT("/Game/Generated/M_%s.M_%s"),*Material.ToString(),*Material.ToString());
  if(auto* M=LoadObject<UMaterialInterface>(nullptr,*Path)) Batch->SetMaterial(0,M);
  Batch->SetCollisionProfileName(TEXT("BlockAll"));Batch->SetMobility(EComponentMobility::Static);Batch->RegisterComponent();
 }
 Batch->AddInstance(FTransform(Rotation,Position,Size/100),true);
}
void ASliceWorld::Sign(FVector Position,FRotator Rotation,const FString& Text,float Size) {
 auto* Label=NewObject<UTextRenderComponent>(this);Label->SetupAttachment(RootComponent);AddInstanceComponent(Label);
 Label->SetText(FText::FromString(Text));Label->SetWorldSize(Size);Label->SetTextRenderColor(FColor(215,228,235));
 Label->SetHorizontalAlignment(EHTA_Center);Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Label->RegisterComponent();Label->SetWorldLocationAndRotation(Position,Rotation);
}
ASliceProp* ASliceWorld::Prop(EPropKind Kind,FVector Position,FVector Size) {
 auto* P=GetWorld()->SpawnActor<ASliceProp>(Position,FRotator::ZeroRotator);P->Configure(Kind,Size);return P;
}
void ASliceWorld::Light(FVector Position,float Intensity,float Radius,FLinearColor Color) {
 auto* L=NewObject<UPointLightComponent>(this);L->SetupAttachment(RootComponent);AddInstanceComponent(L);
 L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetLightColor(Color);L->SetCastShadows(false);L->RegisterComponent();L->SetWorldLocation(Position);
}
void ASliceWorld::Apartment() {
 Box({600,450,350},{1200,900,20},TEXT("Wood"));
 Box({600,450,680},{1220,920,20},TEXT("Plaster"));
 Box({0,450,510},{20,900,300},TEXT("Plaster"));
 Box({600,0,510},{1200,20,300},TEXT("Plaster"));
 Box({600,900,510},{1200,20,300},TEXT("Plaster"));
 Box({1200,150,510},{20,300,300},TEXT("Plaster"));
 Box({1200,700,510},{20,400,300},TEXT("Plaster"));
 Box({1200,400,650},{20,200,60},TEXT("Plaster"));
 // A sleeping alcove and an open kitchen; doorways remain wider than the capsule.
 Box({420,610,500},{20,580,280},TEXT("Plaster"));
 Box({250,180,395},{180,210,70},TEXT("Wood"));Box({250,180,439},{175,205,18},TEXT("Fabric"));
 Box({250,120,456},{95,55,20},TEXT("Paper"));
 Prop(EPropKind::Intro,{250,240,452},{35,25,3});
 Box({600,130,426},{180,100,12},TEXT("Wood"));
 Box({540,130,390},{14,75,60},TEXT("Wood"));Box({670,130,390},{14,75,60},TEXT("Wood"));
 Prop(EPropKind::PhoneDrawer,{590,180,415},{95,35,22});
 Prop(EPropKind::Lamp,{665,130,458},{22,22,45});
 Box({1060,120,400},{230,110,80},TEXT("Fabric"));Box({1060,65,447},{230,20,110},TEXT("Fabric"));
 Prop(EPropKind::Charger,{1040,160,450},{22,13,7});
 Prop(EPropKind::PinNote,{80,790,449},{32,25,3});
 Box({80,800,402},{100,110,80},TEXT("Wood"));
 Prop(EPropKind::FuseCupboard,{760,835,415},{150,100,110});
 Box({550,835,420},{210,100,120},TEXT("Wood"));
 Box({550,835,486},{210,105,12},TEXT("Metal"));
 Prop(EPropKind::FuseBox,{1155,600,510},{30,70,80});
 Prop(EPropKind::Cabinet,{1040,830,435},{150,100,150});
 Prop(EPropKind::ExitDoor,{1200,400,490},{20,190,260});
 Sign({1134,600,550},{0,180,0},TEXT("ROZDZIELNIA"),12);
 Sign({1040,774,525},{0,-90,0},TEXT("ZAMEK 4 CYFRY"),12);
 // Dim emergency light keeps the apartment navigable before restoring power.
 Light({650,420,625},1900,1150,FLinearColor(0.7,0.8,1));
 PuzzleLight=NewObject<UPointLightComponent>(this,TEXT("DeskLight"));PuzzleLight->SetupAttachment(RootComponent);AddInstanceComponent(PuzzleLight);
 PuzzleLight->SetIntensity(0);PuzzleLight->SetAttenuationRadius(500);PuzzleLight->SetLightColor(FLinearColor(1,0.75,0.4));PuzzleLight->RegisterComponent();PuzzleLight->SetWorldLocation({665,130,520});
 Box({1260,400,350},{120,220,20},TEXT("Concrete"));
 // 18 real steps, each 20 cm high and 40 cm deep, from z=360 to ground.
 for(int I=0;I<18;++I) {const float Top=340-I*20;Box({1340.f+I*40,400,Top/2-5},{40,220,Top+10},TEXT("Concrete"));}
 Box({1620,290,370},{840,20,760},TEXT("Plaster"));Box({1620,520,370},{840,20,760},TEXT("Plaster"));
 Box({2220,600,-10},{360,600,20},TEXT("Concrete"));
 Box({2220,300,150},{360,20,300},TEXT("Plaster"));Box({2400,600,150},{20,600,300},TEXT("Plaster"));
 Box({2040,710,150},{20,380,300},TEXT("Plaster"));
 Box({2100,900,150},{120,20,300},TEXT("Plaster"));Box({2370,900,150},{60,20,300},TEXT("Plaster"));
 Prop(EPropKind::BuildingDoor,{2250,900,130},{180,20,260});
 Sign({2390,680,235},{0,180,0},TEXT("WYJŚCIE"),25);Light({2220,640,265},2200,900,FLinearColor(1,0.75,0.5));
}
void ASliceWorld::Outdoors() {
 Box({2600,1500,-10},{1600,1200,20},TEXT("Cobble"));
 Box({3500,2650,-15},{3400,1100,30},TEXT("Asphalt"));
 Box({4900,3850,-10},{600,1300,20},TEXT("Cobble"));
 Box({4600,4900,-10},{1200,800,20},TEXT("Concrete"));
 // Boundary buildings: high walls prevent shortcuts across locked progression.
 Box({1790,2050,450},{30,2300,900},TEXT("Brick"));
 Box({3300,1490,450},{200,1200,900},TEXT("Plaster"));
 Box({1960,890,450},{320,30,900},TEXT("Brick"));Box({2900,890,450},{1000,30,900},TEXT("Brick"));
 Box({4300,2090,600},{1800,30,1200},TEXT("Plaster"));
 Box({5220,3700,500},{40,3200,1000},TEXT("Brick"));
 Box({3200,3220,500},{2800,40,1000},TEXT("Plaster"));
 Box({4580,3850,400},{40,1300,800},TEXT("Brick"));
 // Dogleg occluders: sight is lost around masonry; search uses the last seen location.
 Box({4830,3690,125},{280,260,250},TEXT("Metal"));
 Box({5070,4150,170},{260,35,340},TEXT("Brick"));
 // Shelter room with a physical locked door.
 Box({4000,4900,165},{25,800,330},TEXT("Plaster"));Box({4600,5300,165},{1200,25,330},TEXT("Brick"));
 Box({4425,4500,165},{850,25,330},TEXT("Brick"));Box({5115,4500,165},{170,25,330},TEXT("Brick"));
 Box({4900,4500,300},{180,25,60},TEXT("Brick"));
 Prop(EPropKind::SafeDoor,{4900,4500,135},{175,25,270});
 Box({4600,4900,340},{1200,800,20},TEXT("Plaster"));
 Box({4270,4950,42},{180,80,84},TEXT("Wood"));Light({4600,4900,280},3200,1100,FLinearColor(0.6,0.8,1));
 Sign({4930,4480,310},{0,-90,0},TEXT("AZYL / LOKAL 7"),23);
 // Wroclaw cues: tenement elevations, cornices, bilingual-free Polish signage, tram cables.
 for(int I=0;I<8;++I) {
  const float X=1950+I*390;
  Box({X,3200,650},{310,18,45},TEXT("Stone"));
  for(int Floor=0;Floor<3;++Floor) {
   Box({X,3190,350.f+Floor*250},{120,12,165},TEXT("Glass"));
   Box({X,3178,265.f+Floor*250},{150,35,15},TEXT("Stone"));
  }
 }
 for(int I=0;I<4;++I) {Box({3600.f+I*400,2107,500},{110,15,170},TEXT("Glass"));Box({3600.f+I*400,2107,820},{110,15,170},TEXT("Glass"));}
 Box({3500,3150,8},{3400,90,16},TEXT("Stone"));Box({4200,2140,8},{1600,80,16},TEXT("Stone"));
 Box({3500,2600,570},{3400,3,3},TEXT("Metal"));Box({3500,2800,570},{3400,3,3},TEXT("Metal"));
 for(int I=0;I<4;++I) {const float X=2700+I*650;Box({X,3080,260},{10,10,520},TEXT("Metal"));Light({X,3020,465},2400,950,FLinearColor(1,0.67,0.38));}
 Box({3050,2180,125},{150,80,250},TEXT("Metal")); // cover on the first approach
 Box({2870,1250,60},{160,85,120},TEXT("Wood"));
 Sign({3305,1950,300},{0,0,0},TEXT("WROCŁAW\nNADODRZE"),28);
 Sign({3850,2120,260},{0,90,0},TEXT("ul. Łokietka"),24);
 Sign({4700,3190,280},{0,-90,0},TEXT("LOKAL 7 →"),28);
 Box({2560,1980,125},{6,6,250},TEXT("Metal"));Box({2560,1980,250},{70,8,70},TEXT("SignBlue"));
 Sign({2560,1974,252},{0,-90,0},TEXT("P"),48);
 // A red no-entry sign seals the scenic street end.
 Box({1880,2840,120},{7,7,240},TEXT("Metal"));Box({1880,2840,250},{8,70,70},TEXT("SignRed"));
 Box({1874,2840,250},{3,50,12},TEXT("Paper"));
}
void ASliceWorld::AudioLoop(TObjectPtr<UAudioComponent>& Component,const TCHAR* Name,float Volume) {
 Component=NewObject<UAudioComponent>(this);Component->SetupAttachment(RootComponent);AddInstanceComponent(Component);
 const FString Path=FString::Printf(TEXT("/Game/Generated/Audio/%s.%s"),Name,Name);
 Component->SetSound(LoadObject<USoundBase>(nullptr,*Path));Component->bAutoActivate=false;Component->bIsUISound=false;Component->SetVolumeMultiplier(Volume);Component->RegisterComponent();Component->Play();
}
void ASliceWorld::BeginPlay() {
 Super::BeginPlay();Apartment();Outdoors();
 auto* Atmosphere=NewObject<USkyAtmosphereComponent>(this);Atmosphere->SetupAttachment(RootComponent);AddInstanceComponent(Atmosphere);Atmosphere->RegisterComponent();
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2000),FRotator(-32,-35,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(2.5f);
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetIntensity(0.55);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(0.012f);
 AudioLoop(RoomAudio,TEXT("Apartment"),0.22f);AudioLoop(StreetAudio,TEXT("Street"),0.f);AudioLoop(ChaseAudio,TEXT("Chase"),0.f);
}
void ASliceWorld::Tick(float Dt) {
 Super::Tick(Dt);auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(PuzzleLight) PuzzleLight->SetIntensity(M->State.Complete(5)?3600:0);
 auto* P=UGameplayStatics::GetPlayerPawn(this,0);if(!P) return;
 const bool Outside=P->GetActorLocation().Y>900;
 if(RoomAudio) RoomAudio->SetVolumeMultiplier(Outside?0.f:0.22f);
 if(StreetAudio) StreetAudio->SetVolumeMultiplier(Outside?0.3f:0.05f);
 auto* GM=Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode());
 if(ChaseAudio) ChaseAudio->SetVolumeMultiplier(GM && GM->Enemy && GM->Enemy->IsThreat()?0.38f:0.f);
}
