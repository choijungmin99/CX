#include "Tests/CXNetworkTestDirector.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "CXGameMode.h"
#include "CXPawn.h"
#include "CXPickup.h"
#include "CXMine.h"
#include "CXItemSpawner.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACXNetworkTestDirector::ACXNetworkTestDirector(){PrimaryActorTick.bCanEverTick=true;bReplicates=false;}
void ACXNetworkTestDirector::Record(const FString& Name,bool Pass)
{
    if(Results.Contains(Name))Results[Name]&=Pass;else Results.Add(Name,Pass);
    UE_LOG(LogTemp,Display,TEXT("CX_TEST %s=%s"),*Name,Pass?TEXT("PASS"):TEXT("FAIL"));
}
void ACXNetworkTestDirector::Tick(float Dt)
{
    Super::Tick(Dt);
    if(!HasAuthority())return;
    if(bCourseMode){TickCourse(Dt);return;}
    if(Stage<0)
    {
        Players.Empty();
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<ACXPlayerController>(It->Get()))if(PC->GetPawn())Players.Add(PC);
        if(Players.Num()!=2)return;
        if(!Players[0]->IsLocalController())Swap(Players[0],Players[1]);
        Stage=0;NextStep=GetWorld()->GetTimeSeconds()+1;
    }
    if(Stage<=16 && GetWorld()->GetTimeSeconds()>=NextStep)
    {RunStage();++Stage;NextStep=GetWorld()->GetTimeSeconds()+1.4;}
}
void ACXNetworkTestDirector::Probe(const FString& Label,int32 Phase)
{
    for(ACXPlayerController* PC:Players)
        if(auto* PS=PC->GetPlayerState<ACXPlayerState>())
            PC->ClientTestProbe(Label,PS->Coins,PS->BonusMaxSpeed,Phase);
}
void ACXNetworkTestDirector::RunStage()
{
    auto* GM=GetWorld()->GetAuthGameMode<ACXGameMode>();auto* GS=GetWorld()->GetGameState<ACXGameState>();
    if(!GM||!GS||Players.Num()!=2)return;
    auto* H=Cast<ACXPawn>(Players[0]->GetPawn());auto* G=Cast<ACXPawn>(Players[1]->GetPawn());
    if(!H||!G)
    {
        // 차량 생성 실패도 즉시 실패 보고서를 남긴다. 결과 파일 없이 외부 타임아웃까지 기다리지 않는다.
        Record(TEXT("pawns_exist"),false);FinishReport();Stage=17;return;
    }
    auto* HP=H->GetPlayerState<ACXPlayerState>();auto* GP=G->GetPlayerState<ACXPlayerState>();
    if(!HP||!GP)return;
    const TArray<FString> Captions={
        TEXT("출발 대기: 운전·공격 잠금 확인"),TEXT("위조 요청 차단과 서버 타이머"),TEXT("실제 클라이언트 차량 주행"),
        TEXT("코인·속도 아이템 동시 획득"),TEXT("소유자 전용 기록과 채팅 동기화"),TEXT("잔액 부족 시 공격 금액 보존"),
        TEXT("게스트 공격 RPC와 서버 라인트레이스"),TEXT("5원 탈취와 공격 연사 제한"),TEXT("바퀴 부위 피격 페널티"),
        TEXT("차량 뒤 지뢰 설치와 Owner 확인"),TEXT("지뢰 오버랩과 폭발"),TEXT("가속 페달 고장 유지"),
        TEXT("2초 뒤 고장 회복·단독 우승"),TEXT("동점은 공동 우승"),TEXT("종료 상태 공격·복구 차단"),
        TEXT("다시 경기: 상태와 개인 기록 초기화"),TEXT("검증 결과 저장")};
    for(ACXPlayerController* PC:Players)PC->ClientNotify(Captions[Stage]);
    switch(Stage)
    {
    case 0:
        for(ACXPlayerController* PC:Players)PC->ClientStartDemoCapture();
        Record(TEXT("two_real_connections"),Players[1]->GetNetConnection()!=nullptr);
        Record(TEXT("coin_frequency_1"),GetDefault<ACXCoin>()->GetNetUpdateFrequency()==1.f);
        Record(TEXT("buff_frequency_1"),GetDefault<ACXSpeedBuff>()->GetNetUpdateFrequency()==1.f);
        GS->SetPhase(ECXMatchPhase::Waiting,8);PreviousClock=8;
        BeforeLocation=G->GetActorLocation();Players[1]->ClientTestAction(0);
        Probe(TEXT("waiting"),0);
        break;
    case 1:
        Record(TEXT("server_clock_decreases"),GS->RemainingSeconds<PreviousClock);
        Record(TEXT("waiting_rejects_attacks_and_mines"),G->SuccessfulAttacks==0 && G->MinesPlaced==0);
        Record(TEXT("waiting_throttle_steering_zero"),G->GetChaosVehicleMovement()->GetThrottleInput()==0 && G->GetChaosVehicleMovement()->GetSteeringInput()==0 && G->GetChaosVehicleMovement()->GetHandbrakeInput());
        break;
    case 2:
        GM->StartPlaying();BeforeLocation=G->GetActorLocation();Players[1]->ClientTestAction(7);
        break;
    case 3:
        Record(TEXT("client_drives_chaos_vehicle"),FVector::Dist2D(BeforeLocation,G->GetActorLocation())>30 || G->GetVelocity().Size2D()>30);
        Players[1]->ClientTestAction(11);
        for(TActorIterator<ACXItemSpawner> It(GetWorld());It;++It)if(IsValid(It->SpawnedItem))
        {
            TestedSpawner=*It;OriginalItem=It->SpawnedItem;It->SpawnedItem->Destroy();break;
        }
        HP->ResetRoundStats();GP->ResetRoundStats();
        GetWorld()->SpawnActor<ACXCoin>(H->GetActorLocation()+FVector(0,0,35),FRotator::ZeroRotator);
        GetWorld()->SpawnActor<ACXCoin>(G->GetActorLocation()+FVector(0,0,35),FRotator::ZeroRotator);
        GetWorld()->SpawnActor<ACXSpeedBuff>(G->GetActorLocation()+FVector(0,0,35),FRotator::ZeroRotator);
        break;
    case 4:
        Record(TEXT("spawner_waits_before_2s"),TestedSpawner && !IsValid(TestedSpawner->SpawnedItem));
        Record(TEXT("server_overlap_coin_10"),HP->Coins>=10 && GP->Coins>=10);
        Record(TEXT("server_overlap_speed_10"),GP->BonusMaxSpeed>=10);
        Probe(TEXT("private_stats"),1);
        Players[0]->ClientTestAction(1);Players[1]->ClientTestAction(2);
        break;
    case 5:
    {
        Record(TEXT("spawner_respawns_same_class_after_2s"),TestedSpawner && IsValid(TestedSpawner->SpawnedItem) && TestedSpawner->SpawnedItem->GetClass()==TestedSpawner->ItemClass);
        Players[0]->ClientTestAction(13);Players[1]->ClientTestAction(13);
        HP->RemoveCoins(HP->Coins);GP->RemoveCoins(GP->Coins);HP->AddCoins(4);GP->AddCoins(10);
        FDamageEvent E;H->TakeDamage(5,E,Players[1],G);
        Record(TEXT("victim_under_5_no_transfer"),HP->Coins==4 && GP->Coins==10);
        HP->AddCoins(6);
        // 테스트 배치만 고정한다. 공격 판정 자체는 다음 단계의 클라이언트 RPC가 실행한다.
        H->GetMesh()->SetSimulatePhysics(false);G->GetMesh()->SetSimulatePhysics(false);
        // 코인 재생성 지점과 분리한 시험 구역이다. 공격 중 우연한 추가 획득을 방지한다.
        H->SetActorLocationAndRotation(FVector(2000,6000,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
        const float LaneY=FParse::Param(FCommandLine::Get(),TEXT("CXLaserCapture"))?5000.f:6000.f;
        G->SetActorLocationAndRotation(FVector(1000,LaneY,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
        Players[0]->ClientTestPlaceCar(FVector(2000,6000,100));
        Players[1]->ClientTestPlaceCar(FVector(1000,LaneY,100));
        break;
    }
    case 6:
    {
        BeforeAttacks=G->SuccessfulAttacks;Players[1]->ClientTestAction(3);
        if(FParse::Param(FCommandLine::Get(),TEXT("CXLaserCapture")))
        {
            // 빈 차선에서 빗나간 발사 후, 쿨다운이 끝나면 원래 명중 시험을 이어간다.
            const TWeakObjectPtr<ACXPlayerController> WeakPC(Players[1]);FTimerHandle ShotTimer;
            GetWorldTimerManager().SetTimer(ShotTimer,[WeakPC]()
            {
                auto* PC=WeakPC.Get();if(!PC)return;auto* Car=Cast<ACXPawn>(PC->GetPawn());if(!Car)return;
                Car->SetActorLocationAndRotation(FVector(1000,6000,100),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
                PC->ClientTestPlaceCar(FVector(1000,6000,100));PC->ClientTestAction(3);
            },.7f,false);
        }
        break;
    }
    case 7:
        Record(TEXT("client_rpc_server_line_trace"),G->SuccessfulAttacks==BeforeAttacks+1);
        Record(TEXT("attack_transfers_exactly_5"),HP->Coins==5 && GP->Coins==15);
        BeforeAttacks=G->SuccessfulAttacks;Players[1]->ClientTestAction(6);break;
    case 8:
    {
        Record(TEXT("attack_cooldown_rejects_burst"),G->SuccessfulAttacks==BeforeAttacks+1);
        const FVector Wheel=H->GetMesh()->GetBoneLocation(TEXT("Phys_Wheel_BL"));
        const float Side=Wheel.Y<H->GetActorLocation().Y?-1.f:1.f;
        const FVector AimLocation(Wheel.X-950,Wheel.Y+Side*35,Wheel.Z-55);
        G->SetActorLocationAndRotation(AimLocation,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
        Players[1]->ClientTestPlaceCar(AimLocation);
        Players[1]->ClientTestAction(3);
        break;
    }
    case 9:
        Record(TEXT("wheel_real_trace_bone"),G->LastAttackBone.ToString().Contains(TEXT("Wheel")));
        Record(TEXT("wheel_point_damage_negative"),HP->GetWheelPenalty()<0 && HP->BonusMaxSpeed<0);
        Players[1]->ClientTestAction(4);break;
    case 10:
    {
        ACXMine* Mine=nullptr;for(TActorIterator<ACXMine> It(GetWorld());It;++It){Mine=*It;break;}
        Record(TEXT("mine_spawn_and_owner"),Mine && Mine->GetOwner()==G && G->MinesPlaced>0);
        if(Mine)
        {
            H->SetActorLocation(Mine->GetActorLocation(),false,nullptr,ETeleportType::TeleportPhysics);
            H->GetMesh()->UpdateOverlaps();
        }
        break;
    }
    case 11:
        Record(TEXT("mine_breaks_accelerator"),H->bAcceleratorBroken && H->GetChaosVehicleMovement()->GetThrottleInput()==0);
        Probe(TEXT("mine_broken"),1);
        break;
    case 12:
        Record(TEXT("mine_recovers_after_2s"),!H->bAcceleratorBroken);
        Record(TEXT("wheel_penalty_recovers_to_zero"),FMath::IsNearlyZero(HP->GetWheelPenalty()) && FMath::IsNearlyZero(HP->BonusMaxSpeed));
        GM->FinishMatch();
        Record(TEXT("single_winner_server"),GS->Winners.Num()==1 && GS->Winners[0]==GP);
        Probe(TEXT("finished_single"),2);
        break;
    case 13:
        HP->AddCoins(GP->Coins-HP->Coins);GM->FinishMatch();
        Record(TEXT("tie_winners_all_included"),GS->Winners.Num()==2);
        Probe(TEXT("finished_tie"),2);
        break;
    case 14:
        BeforeAttacks=G->SuccessfulAttacks;BeforeMines=G->MinesPlaced;BeforeLocation=G->GetActorLocation();
        Players[1]->ClientTestAction(0);break;
    case 15:
        Record(TEXT("finished_rejects_actions"),G->SuccessfulAttacks==BeforeAttacks && G->MinesPlaced==BeforeMines && FVector::Dist(BeforeLocation,G->GetActorLocation())<1);
        GM->RestartRound();break;
    case 16:
        Record(TEXT("rematch_two_distinct_vehicles"),H!=G && H->GetController()==Players[0] && G->GetController()==Players[1]
            && FVector::Dist2D(H->GetActorLocation(),G->GetActorLocation())>GM->VehicleSpawnCheckHalfExtent.Size2D()*2.f);
        Record(TEXT("rematch_both_vehicles_simulate_physics"),H->GetMesh()->IsSimulatingPhysics() && G->GetMesh()->IsSimulatingPhysics());
        Record(TEXT("rematch_resets_private_stats"),HP->Coins==0 && GP->Coins==0 && HP->BonusMaxSpeed==0 && GP->BonusMaxSpeed==0);
        Record(TEXT("rematch_waiting_and_empty_winners"),GS->MatchPhase==ECXMatchPhase::Waiting && GS->Winners.IsEmpty() && GS->RoundNumber==2);
        FinishReport();break;
    }
}
void ACXNetworkTestDirector::FinishReport()
{
    bool Pass=Results.Num()>=(bCourseMode?9:40);FString Json=TEXT("{\n  \"checks\": {\n");int32 Count=0;
    for(auto& Pair:Results)
    {
        Pass&=Pair.Value;Json+=FString::Printf(TEXT("    \"%s\": %s%s\n"),*Pair.Key,Pair.Value?TEXT("true"):TEXT("false"),++Count<Results.Num()?TEXT(","):TEXT(""));
    }
    Json+=FString::Printf(TEXT("  },\n  \"check_count\": %d,\n  \"passed\": %s"),Results.Num(),Pass?TEXT("true"):TEXT("false"));
    if(bCourseMode)
    {
        Json+=TEXT(",\n  \"measurements\": {\n");int32 MeasurementIndex=0;
        for(const auto& Pair:CourseMeasurements)
            Json+=FString::Printf(TEXT("    \"%s\": %.3f%s\n"),*Pair.Key,Pair.Value,++MeasurementIndex<CourseMeasurements.Num()?TEXT(","):TEXT(""));
        Json+=TEXT("  }");
    }
    Json+=TEXT("\n}\n");
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Tests");IFileManager::Get().MakeDirectory(*Folder,true);
    FFileHelper::SaveStringToFile(Json,*(Folder/(bCourseMode?TEXT("CourseReport.json"):TEXT("NetworkReport.json"))),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("%s passed=%d checks=%d"),bCourseMode?TEXT("CX_COURSE_TEST_COMPLETE"):TEXT("CX_NETWORK_TEST_COMPLETE"),Pass,Results.Num());
}
