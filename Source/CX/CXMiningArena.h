#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMiningArena.generated.h"
class UInstancedStaticMeshComponent;

/** 지상 점프와 2층 고가 코스를 실제 차량 물리로 주행하는 숲 속 채굴 경기장. */
UCLASS()
class CX_API ACXMiningArena : public AActor
{
    GENERATED_BODY()
public:
    ACXMiningArena();
    static constexpr float UpperDeckHeight = 1400.f;
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="채굴|경기장", meta=(ToolTip="도로, 체공용 점프 램프, 2층 고가 코스와 우회로, 울타리와 숲을 다시 만듭니다. 실제 아이템은 별도 스포너로 배치합니다.")) void Rebuild();
private:
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Generated;
    UPROPERTY() int32 BuiltLayoutVersion = 0;
    UInstancedStaticMeshComponent* Group(const TCHAR* Mesh,FLinearColor Color,bool Collision);
};
