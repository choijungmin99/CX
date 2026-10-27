#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMiningArena.generated.h"
class UInstancedStaticMeshComponent;

/** Vehicle 템플릿 차량이 실제 물리 주행하는 숲 속 채굴 경기장. 모든 장식은 로컬 좌표로 생성한다. */
UCLASS()
class CX_API ACXMiningArena : public AActor
{
    GENERATED_BODY()
public:
    ACXMiningArena();
    virtual void OnConstruction(const FTransform& Transform) override;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="채굴|경기장", meta=(ToolTip="경기장의 도로·램프·울타리·나무·암벽을 다시 만듭니다. 실제 아이템은 별도 CXItemSpawner로 배치합니다.")) void Rebuild();
private:
    UPROPERTY() TArray<TObjectPtr<UActorComponent>> Generated;
    UInstancedStaticMeshComponent* Group(const TCHAR* Mesh,FLinearColor Color,bool Collision);
};
