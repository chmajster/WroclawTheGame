#include "Interaction/NoiseThrowable.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Perception/AISense_Hearing.h"
#include "Audio/SliceAudio.h"
ANoiseThrowable::ANoiseThrowable(){
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThrownObject"));RootComponent=Mesh;
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 Mesh->SetStaticMesh(Sphere.Object);Mesh->SetWorldScale3D(FVector(.1));Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
 Mesh->SetSimulatePhysics(true);Mesh->SetNotifyRigidBodyCollision(true);Mesh->SetCanEverAffectNavigation(false);
}
void ANoiseThrowable::BeginPlay(){Super::BeginPlay();Mesh->OnComponentHit.AddDynamic(this,&ANoiseThrowable::Hit);SetLifeSpan(12);}
void ANoiseThrowable::Hit(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Result){
 if(bReported || Other==GetInstigator())return;bReported=true;
 UAISense_Hearing::ReportNoiseEvent(GetWorld(),GetActorLocation(),1.2f,GetInstigator(),2400);
 USliceAudio::Play(this,TEXT("Hit"),GetActorLocation());
}
