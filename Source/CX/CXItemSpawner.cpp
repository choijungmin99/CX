#include "CXItemSpawner.h"
#include "CXPickup.h"
#include "TimerManager.h"

ACXItemSpawner::ACXItemSpawner()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("SpawnPoint"));
    ItemClass=ACXCoin::StaticClass();bReplicates=false;
}
void ACXItemSpawner::BeginPlay(){Super::BeginPlay();if(HasAuthority())SpawnItem();}
void ACXItemSpawner::SpawnItem()
{
    if(!HasAuthority() || IsValid(SpawnedItem) || !ItemClass)return;
    // SpawnActor 중 오버랩이 발생하기 전에 참조와 델리게이트를 준비하기 위해 Deferred Spawn을 사용한다.
    SpawnedItem=GetWorld()->SpawnActorDeferred<ACXPickup>(ItemClass,GetActorTransform(),this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(SpawnedItem)
    {
        // 2초 리스폰은 유지하면서 같은 자리에 정차/전복한 차량의 무한 획득을 차단한다.
        if(bHasSpawnedItem)SpawnedItem->BlockVehiclesInsideRespawnArea();
        bHasSpawnedItem=true;
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
