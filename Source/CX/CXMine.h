#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXMine.generated.h"
class USphereComponent;
class UStaticMeshComponent;

/** Owner는 설치 차량. 폭발과 가속 고장 판정은 서버만 수행하며 효과는 Multicast로 전달한다. */
UCLASS()
class CX_API ACXMine : public AActor
{
    GENERATED_BODY()
public:
    ACXMine();
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, Category="채굴|지뢰") TObjectPtr<USphereComponent> Trigger;
    UPROPERTY(VisibleAnywhere, Category="채굴|지뢰") TObjectPtr<UStaticMeshComponent> Mesh;
    UFUNCTION() void OnOverlap(UPrimitiveComponent* Part,AActor* Other,UPrimitiveComponent* OtherPart,int32 Body,bool Sweep,const FHitResult& Hit);
    UFUNCTION(NetMulticast,Reliable) void MulticastExplosion(FVector Location);
private:
    bool bExploded=false;
};
