// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "CXPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateVehicle, Log, All);

/**
 *  Vehicle Pawn class
 *  Handles common functionality for all vehicle types,
 *  including input handling and camera management.
 *
 *  Specific vehicle configurations are handled in subclasses.
 */
UCLASS(abstract)
class ACXPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	/** Spring Arm for the front camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	/** Front Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	/** Spring Arm for the back camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	/** Back Camera component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	/** Cast pointer to the Chaos Vehicle movement component */
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:

	/** Steering Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* SteeringAction;

	/** Throttle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ThrottleAction;

	/** Brake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BrakeAction;

	/** Handbrake Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* HandbrakeAction;

	/** Look Around Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAroundAction;

	/** Toggle Camera Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ToggleCameraAction;

	/** Reset Vehicle Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ResetVehicleAction;

	/** Keeps track of which camera is active */
	bool bFrontCameraActive = false;

public:
	ACXPawn();
    virtual void BeginPlay() override;
    virtual float TakeDamage(float Damage,const FDamageEvent& Event,AController* Instigator,AActor* Causer) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;

    /** 입력과 RPC 양쪽에서 검사한다. 클라이언트가 RPC를 직접 보내도 대기 중 공격할 수 없다. */
    bool CanDrive() const;
    UFUNCTION(BlueprintPure, Category="채굴|주행", meta=(ToolTip="Chaos 바퀴 중 하나 이상이 지면과 접촉하는지 확인합니다. 점프 중에는 false입니다."))
    bool IsVehicleGrounded() const;
    bool CanFire() const;
    void ApplyMatchInputLock();
    void BreakAccelerator();
    UFUNCTION(BlueprintCallable, Category="채굴|공격", meta=(ToolTip="로컬 입력을 받아 소유 차량의 공격 Server RPC를 요청합니다.")) void Fire();
    UFUNCTION(BlueprintCallable, Category="채굴|공격", meta=(ToolTip="소유 차량의 지뢰 설치 Server RPC를 요청합니다.")) void PlaceMine();
    UFUNCTION(Server,Reliable) void ServerFire();
    UFUNCTION(Server,Reliable) void ServerPlaceMine();
    UFUNCTION(Server,Reliable) void ServerResetVehicle();
    UFUNCTION(NetMulticast,Unreliable) void MulticastAttackFX(FVector Start,FVector End,bool bHit);
    /** 피격 차량의 부위 기준 상대 좌표를 전달하여 각 화면에서 움직이는 차량에 효과를 붙인다. */
    UFUNCTION(NetMulticast, Unreliable)
    void MulticastHitFX(FName BoneName, FVector LocalHitLocation, FVector LocalHitNormal);
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="채굴|입력", meta=(ToolTip="좌클릭에 매핑하는 Boolean InputAction 애셋입니다.")) TObjectPtr<UInputAction> FireAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="채굴|입력", meta=(ToolTip="F키에 매핑하는 Boolean InputAction 애셋입니다.")) TObjectPtr<UInputAction> MineAction;
    UPROPERTY(EditDefaultsOnly, Category="채굴|공격", meta=(ClampMin="100", ToolTip="앞 범퍼 중앙에서 시작하는 서버 라인트레이스의 최대 사거리(cm)입니다.")) float AttackRange=1800.f;
    UPROPERTY(EditDefaultsOnly, Category="채굴|공격", meta=(ClampMin="0.1", ToolTip="서버에서 검증하는 공격 재사용 대기 시간(초)입니다.")) float AttackCooldown=.45f;
    UPROPERTY(EditDefaultsOnly, Category="채굴|주행", meta=(ClampMin="20", ToolTip="속도 아이템을 얻기 전 기본 최고 속도(km/h)입니다. 서버와 소유자가 함께 제한합니다.")) float BaseMaxSpeed=110.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AcceleratorBroken, Category="채굴|피해", meta=(ToolTip="지뢰에 맞아 가속 입력이 잠긴 상태입니다. 서버가 2초 후 해제합니다.")) bool bAcceleratorBroken=false;
    UPROPERTY(EditDefaultsOnly, Category="채굴|효과", meta=(ToolTip="로컬 공간 설정을 미리 저장한 차량 피격 전용 파티클입니다. 패키징 전에 Cook되며 맞은 차량의 부위를 따라갑니다.")) TObjectPtr<class UParticleSystem> HitParticle;
    UPROPERTY(EditDefaultsOnly, Category="채굴|효과", meta=(ToolTip="공격 명중 시 재생할 효과음입니다.")) TObjectPtr<class USoundBase> HitSound;
    UPROPERTY(EditDefaultsOnly, Category="채굴|레이저", meta=(ToolTip="명중 여부와 관계없이 앞 범퍼에서 재생하는 전용 레이저 발사음입니다. 피격 파손음과 별도로 재생합니다."))
    TObjectPtr<class USoundBase> FireSound;
    UPROPERTY(EditDefaultsOnly, Category="채굴|레이저", meta=(ToolTip="레이저의 청백색 중심과 부드러운 외곽 광원에 사용하는 발광 재질입니다."))
    TObjectPtr<class UMaterialInterface> LaserMaterial;
    UPROPERTY(EditDefaultsOnly, Category="채굴|레이저", meta=(ClampMin="0.08", ClampMax="0.3", ToolTip="레이저가 나타났다가 흐려지는 시간(초)입니다. 서버의 즉시 라인트레이스 판정에는 영향을 주지 않습니다."))
    float LaserDuration = .16f;
    UPROPERTY(EditDefaultsOnly, Category="채굴|레이저", meta=(ClampMin="0", ClampMax="1", ToolTip="발사음 음량입니다. 가까이에서는 선명하고 멀어질수록 작게 들립니다."))
    float FireSoundVolume = .65f;
    UPROPERTY(EditDefaultsOnly, Category="채굴|디버그", meta=(ToolTip="과제 라인트레이스 확인용 선을 추가 표시합니다. 일반 플레이에서는 레이저 연출을 사용하므로 기본값은 꺼짐입니다."))
    bool bShowAttackTrace = false;
    UPROPERTY(EditDefaultsOnly, Category="채굴|효과", meta=(ClampMin="0.1", ClampMax="5.0", ToolTip="피격 부위에 붙은 파티클의 발생 시간(초)입니다. 이후 발생을 멈추고 남은 파티클이 사라지면 자동 정리합니다."))
    float HitEffectDuration = 0.8f;
    int32 SuccessfulAttacks=0;
    int32 MinesPlaced=0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="채굴|부위 판정", meta=(ToolTip="바퀴 Bone에 부착한 쿼리 전용 피격 범위입니다. 차량의 실제 물리 충돌에는 영향을 주지 않습니다.")) TArray<TObjectPtr<class USphereComponent>> WheelHitZones;
    FName LastAttackBone=NAME_None;
    bool IsMatchLocked() const;
    void ResetVehicleOnServer();
    void DriveForTest(float Gas,float Turn);
private:
    /** 차량마다 재사용하는 발사음의 거리 감쇠 설정이다. */
    UPROPERTY(Transient)
    TObjectPtr<class USoundAttenuation> FireAttenuation;
    double LastAttackTime=-100;
    double LastMineTime=-100;
    double LastResetTime=-100;
    FTimerHandle AcceleratorTimer;
    bool bStateInputLock=true;
    void RecoverAccelerator();
    UFUNCTION() void OnRep_AcceleratorBroken();
    void FireInput(const FInputActionValue& Value);
    void MineInput(const FInputActionValue& Value);
public:

	// Begin Pawn interface

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	// End Pawn interface

	// Begin Actor interface

	virtual void Tick(float Delta) override;

	// End Actor interface

protected:

	/** Handles steering input */
	void Steering(const FInputActionValue& Value);

	/** Handles throttle input */
	void Throttle(const FInputActionValue& Value);

	/** Handles brake input */
	void Brake(const FInputActionValue& Value);

	/** Handles brake start/stop inputs */
	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	/** Handles handbrake start/stop inputs */
	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	/** Handles look around input */
	void LookAround(const FInputActionValue& Value);

	/** Handles toggle camera input */
	void ToggleCamera(const FInputActionValue& Value);

	/** Handles reset vehicle input */
	void ResetVehicle(const FInputActionValue& Value);

	/** Called when the brake lights are turned on or off */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

public:
	/** Returns the front spring arm subobject */
	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }
	/** Returns the front camera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }
	/** Returns the back spring arm subobject */
	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }
	/** Returns the back camera subobject */
	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }
	/** Returns the cast Chaos Vehicle Movement subobject */
	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }
};
