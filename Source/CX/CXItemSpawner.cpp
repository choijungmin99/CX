#include "CXItemSpawner.h"
#include "CXPickup.h"
#include "TimerManager.h"

ACXItemSpawner::ACXItemSpawner()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("스폰 위치"));
    ItemClass=ACXCoin::StaticClass();bReplicates=false;
}
void ACXItemSpawner::BeginPlay(){Super::BeginPlay();if(HasAuthority())SpawnItem();}
void ACXItemSpawner::SpawnItem()
{
    if(!HasAuthority() || IsValid(SpawnedItem) || !ItemClass)return;
    FActorSpawnParameters P;P.Owner=this;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    // SpawnActor 중 오버랩이 발생하기 전에 참조와 델리게이트를 준비하기 위해 Deferred Spawn을 사용한다.
    SpawnedItem=GetWorld()->SpawnActorDeferred<ACXPickup>(ItemClass,GetActorTransform(),this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(SpawnedItem)
    {
        SpawnedItem->OnDestroyed.AddDynamic(this,&ACXItemSpawner::OnItemDestroyed);
        SpawnedItem->FinishSpawning(GetActorTransform());
    }
}
void ACXItemSpawner::OnItemDestroyed(AActor*)
{
    if(!HasAuthority())return;
    SpawnedItem=nullptr;
    GetWorldTimerManager().SetTimer(RespawnTimer,this,&ACXItemSpawner::SpawnItem,FMath::Max(.1f,RespawnDelay),false);
}
void ACXItemSpawner::EndPlay(const EEndPlayReason::Type R)
{
    GetWorldTimerManager().ClearTimer(RespawnTimer);
    if(IsValid(SpawnedItem))SpawnedItem->OnDestroyed.RemoveDynamic(this,&ACXItemSpawner::OnItemDestroyed);
    Super::EndPlay(R);
}
