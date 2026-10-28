#include "Tests/CXNetworkTestDirector.h"
#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXItemSpawner.h"
#include "CXPickup.h"
#include "CXPawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"

// -CXNetTest -CXPickupTest: 호스트와 실제 클라이언트에서 같은 재획득 시나리오를 반복한다.
void ACXNetworkTestDirector::TickPickup()
{
    if(Stage>12)return;
    const FVector ItemPosition(5000,5000,485);
    const FVector CarPosition=ItemPosition-FVector(0,0,35);
    if(Stage<0)
    {
        Players.Empty();
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<ACXPlayerController>(It->Get()))if(PC->GetPawn())Players.Add(PC);
        if(Players.Num()!=2)return;
        if(!Players[0]->IsLocalController())Swap(Players[0],Players[1]);
        auto* GM=GetWorld()->GetAuthGameMode<ACXGameMode>();if(!GM)return;
        GM->StartPlaying();
        Record(TEXT("two_real_connections"),Players[1]->GetNetConnection()!=nullptr);
        for(int32 Index=0;Index<Players.Num();++Index)
        {
            auto* Car=Cast<ACXPawn>(Players[Index]->GetPawn());
            if(!Car)return;
            Car->DriveForTest(0,0);Car->GetChaosVehicleMovement()->ResetVehicle();
            Car->GetMesh()->SetSimulatePhysics(false);
            const FVector Outside=CarPosition+FVector(1500,Index*1500,0);
            Players[Index]->ClientTestPlaceCar(Outside);
            Car->SetActorLocationAndRotation(Outside,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
            Car->GetMesh()->UpdateOverlaps();
            Car->GetPlayerState<ACXPlayerState>()->ResetRoundStats();
        }
        Stage=0;NextStep=GetWorld()->GetTimeSeconds()+1.f;return;
    }
    if(GetWorld()->GetTimeSeconds()<NextStep)return;
    ACXPlayerController* PC=Players[PickupPlayerIndex];
    auto* Car=Cast<ACXPawn>(PC->GetPawn());
    auto* PS=PC->GetPlayerState<ACXPlayerState>();
    if(!Car||!PS)return;
    const FString Prefix=PickupPlayerIndex==0?TEXT("host_"):TEXT("client_");
    auto Check=[&](const TCHAR* Name,bool Good){Record(Prefix+Name,Good);};
    auto Pose=[&](FVector P,FRotator R=FRotator::ZeroRotator)
    {
        // 호스트는 서버와 같은 폰이다. 로컬 RPC의 임시 정방향 이동으로 먼저 획득하지 않게 한다.
        if(!PC->IsLocalController())PC->ClientTestPlaceCar(P);
        Car->SetActorLocationAndRotation(P,R,false,nullptr,ETeleportType::TeleportPhysics);
        Car->GetMesh()->UpdateOverlaps();
    };
    auto MakeSpawner=[&](TSubclassOf<ACXPickup> Kind)
    {
        TestedSpawner=GetWorld()->SpawnActorDeferred<ACXItemSpawner>(ACXItemSpawner::StaticClass(),
            FTransform(ItemPosition),this,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        TestedSpawner->ItemClass=Kind;TestedSpawner->RespawnDelay=2.f;
        TestedSpawner->FinishSpawning(FTransform(ItemPosition));
    };
    auto RemoveSpawner=[&]()
    {
        if(!IsValid(TestedSpawner))return;
        ACXPickup* Item=TestedSpawner->SpawnedItem;
        TestedSpawner->Destroy();if(IsValid(Item))Item->Destroy();TestedSpawner=nullptr;
    };
    float Delay=.25f;
    switch(Stage)
    {
    case 0:
        MakeSpawner(ACXCoin::StaticClass());OriginalItem=TestedSpawner->SpawnedItem;
        Check(TEXT("coin_initial_spawn"),OriginalItem.IsValid());
        Pose(CarPosition);
        Check(TEXT("upright_coin_entry_grants_10"),PS->Coins==10&&!OriginalItem.IsValid());
        Pose(CarPosition,FRotator(0,0,180));
        Check(TEXT("inverted_pose_reproduced"),Car->GetActorUpVector().Z<-.9f);
        Delay=1.f;break;
    case 1:
        Check(TEXT("coin_not_respawned_before_2s"),!IsValid(TestedSpawner->SpawnedItem));
        Delay=1.25f;break;
    case 2:
        OriginalItem=TestedSpawner->SpawnedItem;
        Check(TEXT("coin_respawns_same_class_after_2s"),OriginalItem.IsValid()&&OriginalItem->IsA<ACXCoin>());
        Check(TEXT("inverted_car_no_respawn_award"),PS->Coins==10);
        Delay=3.f;break;
    case 3:
        Check(TEXT("inverted_car_no_repeated_farming"),PS->Coins==10&&OriginalItem.IsValid()&&TestedSpawner->SpawnedItem==OriginalItem.Get());
        Pose(CarPosition);break;
    case 4:
        Check(TEXT("righting_inside_requires_exit"),PS->Coins==10&&OriginalItem.IsValid());
        Pose(CarPosition+FVector(1500,0,0));Delay=.35f;break;
    case 5:
        Pose(CarPosition);
        Check(TEXT("coin_exit_reentry_grants_once"),PS->Coins==20&&!OriginalItem.IsValid());
        Pose(CarPosition+FVector(1500,0,0));
        OriginalItem=GetWorld()->SpawnActor<ACXCoin>(ItemPosition,FRotator::ZeroRotator);
        Pose(CarPosition,FRotator(0,0,180));
        Check(TEXT("inverted_entry_cannot_collect"),PS->Coins==20&&OriginalItem.IsValid());break;
    case 6:
        Pose(CarPosition+FVector(1500,0,0));Pose(CarPosition,FRotator(0,0,90));
        Check(TEXT("sideways_entry_cannot_collect"),PS->Coins==20&&OriginalItem.IsValid());break;
    case 7:
        Pose(CarPosition+FVector(1500,0,0));Pose(CarPosition);
        Check(TEXT("upright_entry_still_collects"),PS->Coins==30&&!OriginalItem.IsValid());
        Pose(CarPosition+FVector(1500,0,0));RemoveSpawner();
        MakeSpawner(ACXSpeedBuff::StaticClass());Pose(CarPosition);
        Check(TEXT("speed_initial_entry_grants_10"),FMath::IsNearlyEqual(PS->BonusMaxSpeed,10.f));
        Delay=1.f;break;
    case 8:
        Check(TEXT("speed_not_respawned_before_2s"),!IsValid(TestedSpawner->SpawnedItem));
        Delay=1.25f;break;
    case 9:
        OriginalItem=TestedSpawner->SpawnedItem;
        Check(TEXT("speed_respawns_same_class_after_2s"),OriginalItem.IsValid()&&OriginalItem->IsA<ACXSpeedBuff>());
        Check(TEXT("upright_stationary_car_no_respawn_award"),FMath::IsNearlyEqual(PS->BonusMaxSpeed,10.f));
        Delay=3.f;break;
    case 10:
        Check(TEXT("speed_no_repeated_farming"),FMath::IsNearlyEqual(PS->BonusMaxSpeed,10.f)&&OriginalItem.IsValid()&&TestedSpawner->SpawnedItem==OriginalItem.Get());
        Pose(CarPosition+FVector(1500,0,0));Delay=.35f;break;
    case 11:
        Pose(CarPosition);
        Check(TEXT("speed_exit_reentry_grants_once"),FMath::IsNearlyEqual(PS->BonusMaxSpeed,20.f)&&!OriginalItem.IsValid());
        RemoveSpawner();
        Probe(Prefix+TEXT("pickup_guard"),1);Delay=.8f;break;
    case 12:
        if(PickupPlayerIndex==0){PickupPlayerIndex=1;Stage=-2;}
        else {Stage=100;FinishReport();}
        break;
    }
    ++Stage;NextStep=GetWorld()->GetTimeSeconds()+Delay;
}
