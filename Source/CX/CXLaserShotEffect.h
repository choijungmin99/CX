#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CXLaserShotEffect.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPointLightComponent;

/** 서버 판정과 분리된 짧은 레이저 연출이다. 각 화면에서 생성하고 자동으로 정리한다. */
UCLASS(NotBlueprintable, Transient)
class ACXLaserShotEffect : public AActor
{
    GENERATED_BODY()
public:
    ACXLaserShotEffect();
    void Initialize(AActor* Shooter, FVector Start, FVector End, UMaterialInterface* Material, float Duration);
    virtual void Tick(float DeltaSeconds) override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Core;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Glow;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Muzzle;
    UPROPERTY() TObjectPtr<UPointLightComponent> MuzzleLight;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> MuzzleMaterial;
    TWeakObjectPtr<AActor> Source;
    FVector LocalStart = FVector::ZeroVector;
    FVector WorldEnd = FVector::ZeroVector;
    FVector InitialStart = FVector::ZeroVector;
    float Age = 0.f;
    float Lifetime = .16f;
    void UpdateVisuals();
};
