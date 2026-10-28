#include "Tests/CXNetworkTestDirector.h"
#include "CXGameMode.h"
#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXMiningArena.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ACXNetworkTestDirector::TickRecovery(float Delta)
{
    if(RecoveryStage==7)return;
    if(RecoveryStage<0)
    {
        Players.Empty();
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<ACXPlayerController>(It->Get()))if(PC->GetPawn())Players.Add(PC);
        if(Players.Num()!=2)return;
        if(!Players[0]->IsLocalController())Swap(Players[0],Players[1]);
        auto* GM=GetWorld()->GetAuthGameMode<ACXGameMode>();auto* Car=Cast<ACXPawn>(Players[1]->GetPawn());if(!GM||!Car)return;
        GM->StartPlaying();
        Record(TEXT("two_real_connections"),Players[1]->GetNetConnection()!=nullptr);
        FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CXRampFillTest),false,Car);
        auto Solid=[&](FVector Position){return GetWorld()->OverlapAnyTestByObjectType(Position,FQuat::Identity,Objects,FCollisionShape::MakeBox(FVector(30,30,25)),Params);};
        Record(TEXT("central_ramp_underside_is_solid"),Solid({-1700,-1600,100}));
        Record(TEXT("ground_jump_underside_is_solid"),Solid({-7000,-5800,65}));
        Record(TEXT("upper_access_underside_is_solid"),Solid({-10500,0,120}));
        Record(TEXT("high_bridge_underpass_remains_open"),!Solid({-8500,7500,150}));
        Car->GetChaosVehicleMovement()->ResetVehicle();
        Car->SetActorLocationAndRotation({-1700,-3500,45},FRotator(0,90,0),false,nullptr,ETeleportType::TeleportPhysics);
        Car->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Car->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Players[1]->ClientCourseTestDrive({-1700,-3500,45},FRotator(0,90,0));
        RecoveryStage=0;RecoveryElapsed=0;return;
    }
    auto* Car=Cast<ACXPawn>(Players[1]->GetPawn());if(!Car)return;
    RecoveryElapsed+=Delta;
    if(RecoveryStage==0 && RecoveryElapsed>6.f)
    {
        const FVector Position=Car->GetActorLocation();
        Record(TEXT("side_impact_cannot_enter_low_gap"),!Position.ContainsNaN() && (Position.Y<-2300.f || Position.Z>320.f));
        // 소유자와 서버의 물리를 잠시 고정해, 잘못된 위치에 끼인 상태를 재현한다.
        Car->GetMesh()->SetSimulatePhysics(false);
        RecoveryReference={-1700,-1600,45};
        Car->SetActorLocationAndRotation(RecoveryReference,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
        Players[1]->ClientTestPlaceCar(RecoveryReference);
        RecoveryStage=1;RecoveryElapsed=0;
    }
    else if(RecoveryStage==1 && RecoveryElapsed>.45f)
    {
        Players[1]->ClientTestAction(14);
        RecoveryStage=2;RecoveryElapsed=0;
    }
    else if(RecoveryStage==2 && RecoveryElapsed>1.0f)
    {
        const FVector Position=Car->GetActorLocation();
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CXRecoveryServerTest),false,Car);
        const bool Clear=!GetWorld()->OverlapBlockingTestByChannel(Position+Car->GetActorUpVector()*70.f,Car->GetActorQuat(),
            ECC_Vehicle,FCollisionShape::MakeBox(FVector(160,75,30)),Params);
        Record(TEXT("server_recovery_leaves_blocked_space"),Clear && FVector::Dist(Position,RecoveryReference)>150.f);
        Record(TEXT("server_recovery_restores_physics"),Car->GetMesh()->IsSimulatingPhysics());
        Players[1]->ClientRecoveryTestProbe(Position);
        Players[1]->ClientCourseScreenshot(TEXT("Recovery_After.png"));
        RecoveryReference=Position;
        Players[1]->ClientTestAction(7);
        RecoveryStage=30;RecoveryElapsed=0;
    }
    else if(RecoveryStage==30 && Results.Contains(TEXT("owner_recovery_drive_input_received")))
    {
        // 최초 PSO 생성/화면 캡처가 클라이언트를 지연시켜도 실제 가속 입력 이후부터 측정한다.
        RecoveryReference=Car->GetActorLocation();RecoveryStage=3;RecoveryElapsed=0;
    }
    else if(RecoveryStage==30 && RecoveryElapsed>15.f)
    {
        Record(TEXT("owner_recovery_drive_input_received"),false);
        Record(TEXT("client_drives_after_recovery"),false);
        RecoveryStage=7;FinishReport();
    }
    else if(RecoveryStage==3 && RecoveryElapsed>2.5f)
    {
        Record(TEXT("client_drives_after_recovery"),FVector::Dist2D(Car->GetActorLocation(),RecoveryReference)>100.f);
        Car->GetMesh()->SetSimulatePhysics(false);
        RecoveryReference={-8500,7500,ACXMiningArena::UpperDeckHeight+45.f};
        Car->SetActorLocationAndRotation(RecoveryReference,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
        Players[1]->ClientTestPlaceCar(RecoveryReference);
        RecoveryStage=4;RecoveryElapsed=0;
    }
    else if(RecoveryStage==4 && RecoveryElapsed>.45f)
    {
        Players[1]->ClientTestAction(14);RecoveryStage=5;RecoveryElapsed=0;
    }
    else if(RecoveryStage==5 && RecoveryElapsed>1.f)
    {
        const FVector Position=Car->GetActorLocation();
        Record(TEXT("recovery_preserves_upper_floor"),Position.Z>ACXMiningArena::UpperDeckHeight
            && Position.Z<ACXMiningArena::UpperDeckHeight+250.f && FVector::Dist2D(Position,RecoveryReference)<1000.f);
        Players[1]->ClientRecoveryTestProbe(Position);
        RecoveryStage=6;RecoveryElapsed=0;
    }
    else if(RecoveryStage==6 && RecoveryElapsed>.8f)
    {
        RecoveryStage=7;FinishReport();
    }
}
