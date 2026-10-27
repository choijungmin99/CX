#include "CXMine.h"
#include "CXPawn.h"
#include "CXGameState.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

ACXMine::ACXMine()
{
    bReplicates=true;SetNetUpdateFrequency(5.f);
    Trigger=CreateDefaultSubobject<USphereComponent>(TEXT("지뢰 감지 범위"));SetRootComponent(Trigger);Trigger->SetSphereRadius(160);
    Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);Trigger->SetCollisionResponseToChannel(ECC_Vehicle,ECR_Overlap);
    Trigger->OnComponentBeginOverlap.AddDynamic(this,&ACXMine::OnOverlap);
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("지뢰 표시"));Mesh->SetupAttachment(Trigger);Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));Mesh->SetRelativeScale3D(FVector(1.8f,1.8f,.18f));
    SetLifeSpan(25);
}
void ACXMine::BeginPlay()
{
    Super::BeginPlay();if(!HasAuthority())Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Mining/M_Items.M_Items")))
    {auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),FLinearColor(1,.06f,.02f));Mesh->SetMaterial(0,M);}
}
void ACXMine::OnOverlap(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,int32,bool,const FHitResult&)
{
    if(!HasAuthority() || bExploded || Other==GetOwner())return;
    auto* S=GetWorld()->GetGameState<ACXGameState>();if(!S||!S->IsPlaying())return;
    auto* Car=Cast<ACXPawn>(Other);if(!Car)return;
    bExploded=true;Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Car->BreakAccelerator();S->MulticastWorldFX(GetActorLocation(),true,false);Destroy();
    UE_LOG(LogTemp,Display,TEXT("CX_MINE_EXPLODED victim=%s"),*GetNameSafe(Car));
}
void ACXMine::MulticastExplosion_Implementation(FVector P)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    if(auto* FX=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion")))UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),FX,P);
    if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/StarterContent/Audio/Explosion01.Explosion01")))UGameplayStatics::PlaySoundAtLocation(this,S,P,.6f);
}
