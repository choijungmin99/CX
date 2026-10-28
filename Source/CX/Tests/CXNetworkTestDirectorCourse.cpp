#include "Tests/CXNetworkTestDirector.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameMode.h"
#include "CXMiningArena.h"
#include "CXPawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void ACXNetworkTestDirector::BeginPlay()
{
    Super::BeginPlay();
    bCourseMode=FParse::Param(FCommandLine::Get(),TEXT("CXCourseTest"));
    bRecoveryMode=FParse::Param(FCommandLine::Get(),TEXT("CXRecoveryTest"));
    bPickupMode=FParse::Param(FCommandLine::Get(),TEXT("CXPickupTest"));
}

void ACXNetworkTestDirector::StartCourseScenario(int32 Scenario)
{
    CourseScenario=Scenario;CourseElapsed=0;CourseAirSeconds=0;bCourseAirborne=false;bCoursePeakCaptured=false;
    const FVector Locations[]={FVector(-12800,-5800,45),FVector(-10500,-3800,45),FVector(-9000,7500,ACXMiningArena::UpperDeckHeight+45)};
    const FRotator Rotation(0,Scenario==1?90.f:0.f,0);
    auto* GM=GetWorld()->GetAuthGameMode<ACXGameMode>();
    auto* Car=Cast<ACXPawn>(Players[1]->GetPawn());
    if(!GM||!Car){Record(TEXT("course_vehicle_exists"),false);bCourseFinished=true;FinishReport();return;}
    GM->StartPlaying();
    if(auto* PS=Car->GetPlayerState<ACXPlayerState>())PS->ResetRoundStats();
    Car->GetChaosVehicleMovement()->ResetVehicle();
    Car->SetActorLocationAndRotation(Locations[Scenario],Rotation,false,nullptr,ETeleportType::TeleportPhysics);
    Car->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Car->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Car->GetMesh()->WakeAllRigidBodies();
    CoursePeakHeight=Locations[Scenario].Z;
    CoursePreviousHeight=CoursePeakHeight;
    // 가속은 서버가 강제로 이동시키지 않고 실제 소유 클라이언트에서 입력한다.
    Players[1]->ClientCourseTestDrive(Locations[Scenario],Rotation);
    UE_LOG(LogTemp,Display,TEXT("CX_COURSE_SCENARIO %d start=%s"),Scenario,*Locations[Scenario].ToString());
}

void ACXNetworkTestDirector::CompleteCourseScenario(bool bReachedGoal)
{
    if(CourseScenario==0)
    {
        Record(TEXT("ground_jump_takes_off"),bCourseAirborne);
        Record(TEXT("ground_jump_airtime"),CourseAirSeconds>=1.5f);
        Record(TEXT("ground_jump_lands"),bReachedGoal);
        CourseMeasurements.Add(TEXT("ground_airtime_seconds"),CourseAirSeconds);
        CourseMeasurements.Add(TEXT("ground_peak_height_cm"),CoursePeakHeight);
    }
    else if(CourseScenario==1)
    {
        Record(TEXT("upper_ramp_reaches_second_floor"),bReachedGoal);
        CourseMeasurements.Add(TEXT("upper_ramp_height_cm"),CoursePeakHeight);
    }
    else
    {
        Record(TEXT("upper_jump_takes_off"),bCourseAirborne);
        Record(TEXT("upper_jump_airtime"),CourseAirSeconds>=1.5f);
        Record(TEXT("upper_jump_lands_on_second_floor"),bReachedGoal);
        CourseMeasurements.Add(TEXT("upper_airtime_seconds"),CourseAirSeconds);
        CourseMeasurements.Add(TEXT("upper_peak_height_cm"),CoursePeakHeight);
    }
    if(CourseScenario<2)StartCourseScenario(CourseScenario+1);
    else {bCourseFinished=true;FinishReport();}
}

void ACXNetworkTestDirector::TickCourse(float Dt)
{
    if(bCourseFinished)return;
    if(CourseScenario<0)
    {
        Players.Empty();
        for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
            if(auto* PC=Cast<ACXPlayerController>(It->Get()))if(PC->GetPawn())Players.Add(PC);
        if(Players.Num()!=2)return;
        if(!Players[0]->IsLocalController())Swap(Players[0],Players[1]);
        Record(TEXT("two_real_connections"),Players[1]->GetNetConnection()!=nullptr);
        FHitResult Lower,Upper;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CXCourseLayers),false);
        const bool bLower=GetWorld()->LineTraceSingleByChannel(Lower,{-8500,7500,500},{-8500,7500,-500},ECC_Visibility,Params);
        const bool bUpper=GetWorld()->LineTraceSingleByChannel(Upper,{-8500,7500,2200},{-8500,7500,900},ECC_Visibility,Params);
        Record(TEXT("upper_and_lower_roads_have_collision"),bLower&&bUpper&&FMath::Abs(Lower.ImpactPoint.Z-8)<20
            &&FMath::Abs(Upper.ImpactPoint.Z-ACXMiningArena::UpperDeckHeight)<20);
        StartCourseScenario(0);return;
    }
    auto* Car=Cast<ACXPawn>(Players[1]->GetPawn());
    if(!Car){Record(TEXT("course_vehicle_exists"),false);bCourseFinished=true;FinishReport();return;}
    CourseElapsed+=Dt;
    const FVector P=Car->GetActorLocation();
    CoursePeakHeight=FMath::Max(CoursePeakHeight,float(P.Z));
    const bool bGrounded=Car->IsVehicleGrounded();
    const bool bInJumpArea=CourseScenario==0?(P.X>-2900&&P.Z>500):(CourseScenario==2&&P.X>-1800&&P.Z>ACXMiningArena::UpperDeckHeight+500);
    if(bInJumpArea&&!bGrounded)bCourseAirborne=true;
    if(bCourseAirborne&&!bGrounded)CourseAirSeconds+=Dt;
    if(bCourseAirborne&&!bCoursePeakCaptured&&P.Z<CoursePreviousHeight&&P.Z>(CourseScenario==2?ACXMiningArena::UpperDeckHeight+500:1000))
    {
        bCoursePeakCaptured=true;
        Players[1]->ClientCourseScreenshot(FString::Printf(TEXT("Course_%d_Air.png"),CourseScenario));
    }
    CoursePreviousHeight=P.Z;
    bool bGoal=false;
    if(CourseScenario==0)bGoal=bCourseAirborne&&bGrounded&&P.X>0;
    else if(CourseScenario==1)bGoal=P.Y>5300&&P.Z>ACXMiningArena::UpperDeckHeight-100&&bGrounded;
    else bGoal=bCourseAirborne&&bGrounded&&P.X>2500&&P.Z>ACXMiningArena::UpperDeckHeight-100;
    if(bGoal||CourseElapsed>25.f)
    {
        if(bGoal)Players[1]->ClientCourseScreenshot(FString::Printf(TEXT("Course_%d_Landing.png"),CourseScenario));
        UE_LOG(LogTemp,Display,TEXT("CX_COURSE_SCENARIO %d goal=%d position=%s airtime=%.2f peak=%.1f"),
            CourseScenario,bGoal,*P.ToString(),CourseAirSeconds,CoursePeakHeight);
        CompleteCourseScenario(bGoal);
    }
}
