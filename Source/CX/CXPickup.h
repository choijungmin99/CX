#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UParticleSystem;
class USoundBase;

/** 동일한 아이템 한 개를 서버에서만 생성·획득·파괴하고, 모든 클라이언트가 그 결과를 본다. */
UCLASS(Abstract)
class CX_API ACXPickup : public AActor
{
    GENERATED_BODY()
public:
    ACXPickup();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="채굴|아이템") TObjectPtr<USphereComponent> Trigger;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="채굴|아이템") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(EditDefaultsOnly, Category="채굴|효과", meta=(ToolTip="획득 시 모든 화면에 재생할 파티클입니다. 코인은 금빛 반짝임, 속도 아이템은 약 0.7초 안에 사라지는 푸른 파동을 사용합니다. 차량 피격 효과와 독립된 애셋입니다.")) TObjectPtr<UParticleSystem> PickupParticle;
    UPROPERTY(EditDefaultsOnly, Category="채굴|효과", meta=(ToolTip="획득 시 재생할 공간 효과음입니다. 코인은 맑은 획득음, 속도 아이템은 0.65초의 부드러운 상승음이 기본값이며 반복 재생하지 않습니다.")) TObjectPtr<USoundBase> PickupSound;
    bool IsConsumed() const {return bConsumed;}
    /** 리스폰 직전에 이미 영역 안에 있던 차량은 완전히 나갔다 들어와야 다시 획득한다. 서버 전용. */
    void BlockVehiclesInsideRespawnArea();
    UFUNCTION() void OnOverlap(UPrimitiveComponent* Part,AActor* Other,UPrimitiveComponent* OtherPart,int32 Body,bool Sweep,const FHitResult& Hit);
    UFUNCTION() void OnEndOverlap(UPrimitiveComponent* Part,AActor* Other,UPrimitiveComponent* OtherPart,int32 Body);
    UFUNCTION(NetMulticast,Reliable) void MulticastPickupFX(FVector Location);
protected:
    virtual bool Grant(class ACXPlayerState* PS) PURE_VIRTUAL(ACXPickup::Grant,return false;);
    virtual FLinearColor ItemColor() const {return FLinearColor(1,.65f,.04f);}
    virtual bool IsSpeedItem() const {return false;}
private:
    TSet<TWeakObjectPtr<AActor>> VehiclesAwaitingExit;
    bool bConsumed=false;
    float AnimationTime=0;
};

UCLASS()
class CX_API ACXCoin : public ACXPickup
{
    GENERATED_BODY()
public:
    ACXCoin();
protected:
    virtual bool Grant(class ACXPlayerState* PS) override;
};

UCLASS()
class CX_API ACXSpeedBuff : public ACXPickup
{
    GENERATED_BODY()
public:
    ACXSpeedBuff();
protected:
    virtual bool Grant(class ACXPlayerState* PS) override;
    virtual FLinearColor ItemColor() const override {return FLinearColor(.04f,.95f,.68f);}
    virtual bool IsSpeedItem() const override {return true;}
};
