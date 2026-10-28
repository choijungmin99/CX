#include "CXPickup.h"
#include "CXPawn.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "CXPlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ACXPickup::ACXPickup()
{
    bReplicates=true;SetReplicateMovement(false);
    SetNetUpdateFrequency(1.f);SetMinNetUpdateFrequency(1.f);
    PrimaryActorTick.bCanEverTick=true;
    Trigger=CreateDefaultSubobject<USphereComponent>(TEXT("PickupTrigger"));SetRootComponent(Trigger);
    Trigger->SetSphereRadius(145);Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);Trigger->SetCollisionResponseToChannel(ECC_Vehicle,ECR_Overlap);
    Trigger->OnComponentBeginOverlap.AddDynamic(this,&ACXPickup::OnOverlap);
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));Mesh->SetupAttachment(Trigger);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
ACXCoin::ACXCoin()
{
    // 코인에는 짧은 금빛 반짝임만 사용한다. 차량 피격 파티클과 독립된 애셋이다.
    static ConstructorHelpers::FObjectFinder<UParticleSystem> CoinFX(TEXT("/Game/Mining/Particles/P_CoinPickup.P_CoinPickup"));
    PickupParticle=CoinFX.Object;
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Mesh->SetRelativeScale3D(FVector(1.15f,1.15f,.22f));Mesh->SetRelativeRotation(FRotator(90,0,0));
}
ACXSpeedBuff::ACXSpeedBuff()
{
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mesh->SetRelativeScale3D(FVector(.9f,.9f,.9f));Mesh->SetRelativeRotation(FRotator(0,45,0));
}
void ACXPickup::BeginPlay()
{
    Super::BeginPlay();
    if(!HasAuthority())Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(!PickupParticle)
    {
        const TCHAR* ParticlePath=IsSpeedItem()
            ? TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks")
            : TEXT("/Game/Mining/Particles/P_CoinPickup.P_CoinPickup");
        PickupParticle=LoadObject<UParticleSystem>(nullptr,ParticlePath);
    }
    if (!PickupSound)
    {
        // 코인은 전용 획득음을 사용한다. 차량 충격음을 높여 재생하지 않는다.
        const TCHAR* SoundPath = IsSpeedItem()
            ? TEXT("/Game/StarterContent/Audio/Collapse01.Collapse01")
            : TEXT("/Game/Mining/Audio/S_CoinPickup.S_CoinPickup");
        PickupSound = LoadObject<USoundBase>(nullptr, SoundPath);
    }
    if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Mining/M_Items.M_Items")))
    {
        auto* M=UMaterialInstanceDynamic::Create(Base,this);M->SetVectorParameterValue(TEXT("Tint"),ItemColor());Mesh->SetMaterial(0,M);
    }
}
void ACXPickup::Tick(float Dt)
{
    Super::Tick(Dt);if(GetNetMode()==NM_DedicatedServer)return;
    AnimationTime+=Dt;
    Mesh->SetRelativeLocation(FVector(0,0,FMath::Sin(AnimationTime*2)*18));
    Mesh->AddLocalRotation(FRotator(0,Dt*45,0));
}
bool ACXCoin::Grant(ACXPlayerState* PS) {return PS&&PS->AddCoins(10);}
bool ACXSpeedBuff::Grant(ACXPlayerState* PS) {return PS&&PS->AddBonusMaxSpeed(10);}
void ACXPickup::OnOverlap(UPrimitiveComponent*,AActor* Other,UPrimitiveComponent*,int32,bool,const FHitResult&)
{
    if(!HasAuthority() || bConsumed)return;
    auto* S=GetWorld()->GetGameState<ACXGameState>();if(!S||!S->IsPlaying())return;
    auto* Car=Cast<ACXPawn>(Other);if(!Car)return;
    auto* PS=Car->GetPlayerState<ACXPlayerState>();if(!Grant(PS))return;
    // 여러 바퀴/차체의 연속 오버랩과 동시 접촉에도 중복 지급하지 않는다.
    bConsumed=true;Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // NetUpdateFrequency=1인 아이템이 첫 복제 전에 획득되어도 효과가 누락되지 않는다.
    S->MulticastWorldFX(GetActorLocation(),false,IsSpeedItem(),PickupParticle,PickupSound);
    if(auto* PC=Cast<ACXPlayerController>(Car->GetController()))PC->ClientNotify(IsSpeedItem()?TEXT("추가 최고 속도 +10 km/h"):TEXT("원화 +10원"));
    UE_LOG(LogTemp,Display,TEXT("CX_PICKUP type=%s coins=%d bonus=%.1f"),IsSpeedItem()?TEXT("Speed"):TEXT("Coin"),PS->Coins,PS->BonusMaxSpeed);
    Destroy();
}
void ACXPickup::MulticastPickupFX_Implementation(FVector P)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    if(PickupParticle)UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),PickupParticle,P);
    if(PickupSound)UGameplayStatics::PlaySoundAtLocation(this,PickupSound,P,.5f,IsSpeedItem()?1.3f:1.f);
}
