#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXNetworkTestDirector.generated.h"
class ACXPlayerController;
class ACXPawn;

/** 테스트 실행 인자에서만 작동하는 실제 2프로세스 기능·물리 주행 검증. */
UCLASS(NotBlueprintable)
class CX_API ACXNetworkTestDirector : public AActor
{
    GENERATED_BODY()
public:
    ACXNetworkTestDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    void Record(const FString& Name,bool bPassed);
private:
    TMap<FString,bool> Results;
    UPROPERTY() TArray<TObjectPtr<ACXPlayerController>> Players;
    int32 Stage=-1;
    double NextStep=0;
    int32 PreviousClock=0;
    int32 BeforeAttacks=0;
    int32 BeforeMines=0;
    FVector BeforeLocation=FVector::ZeroVector;
    UPROPERTY() TObjectPtr<class ACXItemSpawner> TestedSpawner;
    TWeakObjectPtr<class ACXPickup> OriginalItem;
    void RunStage();
    void Probe(const FString& Label,int32 Phase);
    void FinishReport();
    bool bCourseMode=false;
    bool bRecoveryMode=false;
    bool bPickupMode=false;
    int32 PickupPlayerIndex=0;
    void TickPickup();
    int32 RecoveryStage=-1;
    float RecoveryElapsed=0.f;
    FVector RecoveryReference=FVector::ZeroVector;
    void TickRecovery(float Delta);
    bool bCourseFinished=false;
    int32 CourseScenario=-1;
    float CourseElapsed=0;
    float CourseAirSeconds=0;
    float CoursePeakHeight=0;
    bool bCourseAirborne=false;
    bool bCoursePeakCaptured=false;
    float CoursePreviousHeight=0;
    TMap<FString,float> CourseMeasurements;
    void TickCourse(float Delta);
    void StartCourseScenario(int32 Scenario);
    void CompleteCourseScenario(bool bReachedGoal);
};
