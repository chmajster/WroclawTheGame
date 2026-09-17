#include "World/SliceWorld.h"
#include "Interaction/SliceProp.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
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
void ASliceWorld::Light(FVector Position,float Intensity,float Radius,FLinearColor Color) {
 auto* L=NewObject<UPointLightComponent>(this);L->SetupAttachment(RootComponent);AddInstanceComponent(L);
 L->SetIntensity(Intensity);L->SetAttenuationRadius(Radius);L->SetLightColor(Color);L->SetCastShadows(false);L->RegisterComponent();L->SetWorldLocation(Position);
}
void ASliceWorld::Apartment(){
 Box({600,450,350},{1200,900,20},TEXT("Wood"));Box({600,450,680},{1220,920,20},TEXT("Plaster"));
 Box({0,450,510},{20,900,300},TEXT("Plaster"));Box({600,0,510},{1200,20,300},TEXT("Plaster"));Box({600,900,510},{1200,20,300},TEXT("Plaster"));
 Box({1200,150,510},{20,300,300},TEXT("Plaster"));Box({1200,700,510},{20,400,300},TEXT("Plaster"));Box({1200,400,650},{20,200,60},TEXT("Plaster"));
 Box({420,610,500},{20,580,280},TEXT("Plaster"));
 Box({250,180,395},{180,210,70},TEXT("Wood"));Box({250,180,439},{175,205,18},TEXT("Fabric"));
 Box({600,130,395},{180,100,65},TEXT("Wood"));Box({1060,120,400},{230,110,80},TEXT("Fabric"));
 Box({80,800,402},{100,110,80},TEXT("Wood"));Box({550,835,420},{210,100,120},TEXT("Wood"));
 Light({650,420,625},1500,1150,FLinearColor(.55,.65,.85));
 PuzzleLight=NewObject<UPointLightComponent>(this,TEXT("ApartmentPower"));PuzzleLight->SetupAttachment(RootComponent);AddInstanceComponent(PuzzleLight);
 PuzzleLight->SetIntensity(0);PuzzleLight->SetAttenuationRadius(1200);PuzzleLight->RegisterComponent();PuzzleLight->SetWorldLocation({700,400,620});
 Box({1260,400,350},{120,220,20},TEXT("Concrete"));
 for(int I=0;I<18;++I){const float Top=340-I*20;Box({1340.f+I*40,400,Top/2-5},{40,220,Top+10},TEXT("Concrete"));}
 Box({1620,290,370},{840,20,760},TEXT("Plaster"));Box({1620,520,370},{840,20,760},TEXT("Plaster"));
 Box({2220,600,-10},{360,600,20},TEXT("Concrete"));Box({2220,300,150},{360,20,300},TEXT("Plaster"));
 // A closed main exit and two real side openings on the ground floor.
 Box({2220,900,150},{360,20,300},TEXT("Brick"));
 Box({2040,550,150},{20,60,300},TEXT("Plaster"));Box({2040,860,150},{20,80,300},TEXT("Plaster"));
 Box({2400,400,150},{20,200,300},TEXT("Plaster"));Box({2400,820,150},{20,160,300},TEXT("Plaster"));
 Sign({2240,882,220},{0,-90,0},TEXT("WEJŚCIE ZABLOKOWANE"),18);Light({2220,640,265},1700,650,FLinearColor(1,.75,.5));
}
void ASliceWorld::Routes(){
 // Basement: walk down 16 x 20 cm steps, solve the grate, climb a separate stair to the courtyard.
 for(int I=0;I<18;++I){const float Top=-FMath::Min(320.f,(I+1)*20.f);Box({2020.f-I*40,680,(Top-340)/2},{40,240,Top+340},TEXT("Concrete"));}
 Box({1680,550,20},{720,20,680},TEXT("Plaster"));Box({1680,810,20},{720,20,680},TEXT("Plaster"));
 Box({760,960,-330},{1120,880,20},TEXT("Concrete"));
 Box({200,960,-160},{20,880,320},TEXT("Brick"));Box({760,520,-160},{1120,20,320},TEXT("Brick"));
 Box({1320,1100,-160},{20,600,320},TEXT("Brick"));Box({680,1400,-160},{960,20,320},TEXT("Brick"));
 Box({760,960,20},{1120,880,20},TEXT("Concrete"));
 Box({1760,1560,-330},{1200,320,20},TEXT("Concrete"));
 for(int I=0;I<18;++I){const float Top=-300+FMath::Min(I*20.f,300.f);Box({1620.f+I*40,1550,(Top-340)/2},{40,300,Top+340},TEXT("Concrete"));}
 Box({1780,1730,-100},{1240,20,480},TEXT("Brick"));Box({1780,1390,-100},{900,20,480},TEXT("Brick"));
 Box({2380,1560,-10},{120,320,20},TEXT("Concrete"));
 // Alcoves break sight lines and keep clues distributed in a compact basement.
 Box({680,850,-210},{240,30,220},TEXT("Brick"));Box({880,1120,-210},{30,250,220},TEXT("Brick"));
 Light({550,1000,-70},220,750,FLinearColor(.4,.5,.7));
 // Technical route: an independent ascent, room and external descent.
 for(int I=0;I<18;++I){const float Top=(I+1)*20;Box({2420.f+I*40,620,Top/2-5},{40,240,Top+10},TEXT("Concrete"));}
 Box({2760,490,330},{720,20,680},TEXT("Plaster"));Box({2760,750,330},{720,20,680},TEXT("Plaster"));
 Box({3560,650,350},{880,700,20},TEXT("Concrete"));Box({3560,300,510},{880,20,300},TEXT("Plaster"));
 Box({4000,650,510},{20,700,300},TEXT("Plaster"));Box({3120,400,510},{20,200,300},TEXT("Plaster"));Box({3120,870,510},{20,260,300},TEXT("Plaster"));
 Box({3395,1000,510},{550,20,300},TEXT("Plaster"));Box({3945,1000,510},{110,20,300},TEXT("Plaster"));
 Box({3560,650,670},{880,700,20},TEXT("Plaster"));Light({3560,650,620},2400,1000,FLinearColor(.6,.7,1));
 for(int I=0;I<18;++I){const float Top=340-I*20;Box({3780,1020.f+I*40,Top/2-5},{220,40,Top+10},TEXT("Concrete"));}
 Box({3660,1360,220},{20,720,440},TEXT("Brick"));Box({3900,1360,220},{20,720,440},TEXT("Brick"));
 Sign({2220,350,220},{0,90,0},TEXT("PIWNICA ←    TECHNICZNE →"),17);
}
void ASliceWorld::Outdoors(){
 // Boundary masonry closes the outer edges without adding inaccessible city geometry.
 Box({3100,1190,100},{2600,20,200},TEXT("Brick"));
 Box({4390,1050,180},{20,300,360},TEXT("Brick"));Box({4550,890,180},{300,20,360},TEXT("Brick"));
 Box({7750,2190,400},{500,20,800},TEXT("Brick"));Box({8020,2550,400},{40,700,800},TEXT("Brick"));
 Box({6450,3320,200},{100,40,400},TEXT("Brick"));Box({7750,3320,220},{100,40,440},TEXT("Brick"));
 Box({8720,3550,220},{40,300,440},TEXT("Brick"));Box({8570,3390,220},{300,20,440},TEXT("Brick"));
 Box({10200,4490,220},{600,20,440},TEXT("Brick"));Box({10520,4750,220},{40,500,440},TEXT("Brick"));
 Box({9580,4850,170},{40,300,340},TEXT("Brick"));Box({9800,5020,170},{400,40,340},TEXT("Brick"));
 Box({2240,370,45},{120,45,50},TEXT("Fabric"));Box({2170,370,55},{30,30,30},TEXT("Paper"));
 // Courtyard floor deliberately leaves an opening above the ascending basement staircase.
 Box({3100,1300,-10},{2600,200,20},TEXT("Cobble"));Box({3100,1960,-10},{2600,480,20},TEXT("Cobble"));Box({3420,1560,-10},{1960,320,20},TEXT("Cobble"));
 Box({4550,1550,-10},{300,1300,20},TEXT("Cobble"));
 Box({1780,1800,360},{40,800,720},TEXT("Brick"));
 Box({3100,1860,100},{200,140,200},TEXT("Metal"));Box({4060,1740,100},{170,120,200},TEXT("Wood"));
 Box({4900,2750,-15},{6200,1100,30},TEXT("Asphalt"));
 Box({6700,1600,-10},{1600,1200,20},TEXT("Concrete"));
 Box({6700,990,350},{1600,20,700},TEXT("Brick"));Box({7520,1600,350},{40,1200,700},TEXT("Brick"));
 Box({6700,1750,75},{280,130,110},TEXT("Metal"));Box({6700,1750,150},{140,110,65},TEXT("Glass"));
 Box({7160,2860,75},{280,140,110},TEXT("Metal"));Box({7160,2860,150},{150,110,70},TEXT("Glass"));
 // Shop: front code or key-operated side entrance, both physically connected.
 Box({5300,1550,-10},{1200,1300,20},TEXT("Concrete"));Box({5300,900,160},{1200,20,320},TEXT("Plaster"));
 Box({5900,1550,160},{20,1300,320},TEXT("Brick"));Box({4700,1075,160},{20,350,320},TEXT("Brick"));Box({4700,1825,160},{20,750,320},TEXT("Brick"));
 Box({4950,2200,160},{500,20,320},TEXT("Plaster"));Box({5650,2200,160},{500,20,320},TEXT("Plaster"));
 Box({5300,1550,330},{1200,1300,20},TEXT("Plaster"));Box({5400,1480,40},{400,100,80},TEXT("Wood"));
 Light({5300,1600,280},3000,1300,FLinearColor(1,.85,.65));
 // Street scenery keeps the district closed; there is no full-city simulation.
 Box({1780,2760,550},{40,1120,1100},TEXT("Brick"));Box({4100,3320,500},{4600,40,1000},TEXT("Plaster"));
 for(int I=0;I<11;++I){const float X=2050+I*390;for(int J=0;J<3;++J){Box({X,3290,350.f+J*250},{120,15,160},TEXT("Glass"));Box({X,3275,265.f+J*250},{145,30,15},TEXT("Stone"));}}
 for(int I=0;I<7;++I){const float X=2350+I*760;Box({X,3080,240},{10,10,480},TEXT("Metal"));Light({X,3020,450},2300,1000,FLinearColor(1,.67,.38));}
 Box({4900,2620,570},{6200,3,3},TEXT("Metal"));Box({4900,2800,570},{6200,3,3},TEXT("Metal"));
 Sign({4700,2230,270},{0,90,0},TEXT("WROCŁAW / NADODRZE"),26);Sign({5400,2230,230},{0,90,0},TEXT("SKLEP — OD 1986"),24);
 Box({6100,2220,125},{6,6,250},TEXT("Metal"));Box({6100,2220,250},{70,8,70},TEXT("SignBlue"));Sign({6100,2230,255},{0,90,0},TEXT("P"),48);
 // Abandoned unit and the chase dogleg.
 Box({7100,3800,-10},{1200,1000,20},TEXT("Concrete"));Box({6750,3300,160},{500,20,320},TEXT("Brick"));Box({7450,3300,160},{500,20,320},TEXT("Brick"));
 Box({6500,3800,160},{20,1000,320},TEXT("Brick"));Box({7700,3800,160},{20,1000,320},TEXT("Brick"));Box({7100,4300,160},{1200,20,320},TEXT("Brick"));
 Box({7100,3800,330},{1200,1000,20},TEXT("Plaster"));Box({7370,3900,40},{200,120,80},TEXT("Wood"));Light({7100,3800,280},1700,1250,FLinearColor(.65,.8,1));
 Box({8100,3350,-10},{600,900,20},TEXT("Cobble"));Box({8250,3600,-10},{900,400,20},TEXT("Cobble"));
 Box({7780,3520,220},{40,500,440},TEXT("Brick"));Box({8420,3210,220},{40,500,440},TEXT("Brick"));
 Box({8100,3450,40},{600,45,80},TEXT("Wood")); // Jumpable, above the player's 45 cm step height.
 Box({8220,3660,160},{40,280,320},TEXT("Brick"));
 // Garage front puzzle, interior cover, permanently usable rear escape.
 Box({9000,4200,-10},{1200,1000,20},TEXT("Concrete"));
 Box({8400,4200,160},{20,1000,320},TEXT("Brick"));Box({9090,3700,160},{1020,20,320},TEXT("Brick"));
 Box({9600,4100,160},{20,800,320},TEXT("Brick"));Box({9000,4700,160},{1200,20,320},TEXT("Brick"));
 Box({9000,4200,330},{1200,1000,20},TEXT("Plaster"));Box({8990,4090,95},{250,160,190},TEXT("Metal"));
 Light({9000,4200,280},1800,1200,FLinearColor(1,.65,.45));
 // Rear path turns behind masonry before the workshop: hiding does not require a kill.
 Box({10050,4750,-10},{900,500,20},TEXT("Cobble"));Box({9750,4450,220},{300,30,440},TEXT("Brick"));
 Box({9900,4680,150},{40,300,300},TEXT("Brick"));Box({10400,4900,220},{30,500,440},TEXT("Brick"));
 Box({10600,5500,-10},{1200,1000,20},TEXT("Concrete"));Box({10000,5500,170},{20,1000,340},TEXT("Brick"));
 Box({10050,5000,170},{100,20,340},TEXT("Brick"));Box({10750,5000,170},{900,20,340},TEXT("Brick"));
 Box({11200,5500,170},{20,1000,340},TEXT("Brick"));Box({10600,6000,170},{1200,20,340},TEXT("Brick"));Box({10600,5500,350},{1200,1000,20},TEXT("Plaster"));
 Box({10800,5500,45},{260,180,90},TEXT("Wood"));Light({10600,5500,290},2600,1300,FLinearColor(.7,.85,1));
 Sign({10200,4980,270},{0,-90,0},TEXT("STARY WARSZTAT"),26);
}
void ASliceWorld::Interactions(){
 for(const auto& A:Wroclaw::Catalog()){
  if(A.kind=="virtual" || A.kind=="zone" || (A.x==0 && A.y==0))continue;
  FVector Size(35,25,18);
  if(A.id=="apartment_unlocked")Size={20,190,260};
  if(A.id=="basement_enter" || A.id=="technical_enter" || A.id=="shop_back")Size={20,190,260};
  if(A.id=="basement_symbols")Size={160,20,260};
  if(A.id=="technical_lock" || A.id=="shop_code" || A.id=="panel" || A.id=="garage" || A.id=="workshop")Size={185,20,270};
  auto* P=GetWorld()->SpawnActor<ASliceProp>(FVector(A.x,A.y,A.z),FRotator::ZeroRotator);P->Configure(UTF8_TO_TCHAR(A.id.c_str()),Size);
 }
}
void ASliceWorld::AudioLoop(TObjectPtr<UAudioComponent>& Component,const TCHAR* Name,float Volume) {
 Component=NewObject<UAudioComponent>(this);Component->SetupAttachment(RootComponent);AddInstanceComponent(Component);
 const FString Path=FString::Printf(TEXT("/Game/Generated/Audio/%s.%s"),Name,Name);
 Component->SetSound(LoadObject<USoundBase>(nullptr,*Path));Component->bAutoActivate=false;Component->bIsUISound=false;Component->SetVolumeMultiplier(Volume);Component->RegisterComponent();Component->Play();
}
void ASliceWorld::BeginPlay() {
 Super::BeginPlay();Apartment();Routes();Outdoors();Interactions();
 auto* Atmosphere=NewObject<USkyAtmosphereComponent>(this);Atmosphere->SetupAttachment(RootComponent);AddInstanceComponent(Atmosphere);Atmosphere->RegisterComponent();
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2000),FRotator(-32,-35,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sun->GetLightComponent()->SetIntensity(2.5f);
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetIntensity(0.55);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(0.012f);
 AudioLoop(RoomAudio,TEXT("Apartment"),0.22f);AudioLoop(StreetAudio,TEXT("Street"),0.f);AudioLoop(ChaseAudio,TEXT("Chase"),0.f);
}
void ASliceWorld::Tick(float Dt) {
 Super::Tick(Dt);auto* M=GetGameInstance()->GetSubsystem<USliceMission>();
 if(PuzzleLight) PuzzleLight->SetIntensity(M->State.Done("power")?4200:0);
 auto* P=UGameplayStatics::GetPlayerPawn(this,0);if(!P) return;
 const bool Outside=P->GetActorLocation().Y>1200 && P->GetActorLocation().Z<200;
 if(RoomAudio) RoomAudio->SetVolumeMultiplier(Outside?0.f:0.22f);
 if(StreetAudio) StreetAudio->SetVolumeMultiplier(Outside?0.3f:0.05f);
 auto* GM=Cast<ASliceGameMode>(GetWorld()->GetAuthGameMode());
 if(ChaseAudio) ChaseAudio->SetVolumeMultiplier(GM && GM->HasThreat()?0.38f:0.f);
}
