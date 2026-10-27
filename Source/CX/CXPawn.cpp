// Copyright Epic Games, Inc. All Rights Reserved.

#include "CXPawn.h"
#include "CXWheelFront.h"
#include "CXWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXPlayerController.h"
#include "CXMine.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

DEFINE_LOG_CATEGORY(LogTemplateVehicle);

ACXPawn::ACXPawn()
{
    bReplicates=true;SetReplicateMovement(true);
    SetNetUpdateFrequency(30.f);SetMinNetUpdateFrequency(15.f);
    // Chaos 템플릿의 바퀴 물리 바디가 충돌에서 제외되어도 타이어 부위를 조준할 수 있다.
    // 시각적 바퀴 Bone을 따라가는 쿼리 전용 구체이며 물리 힘이나 아이템 오버랩은 만들지 않는다.
    const TArray<FName> WheelBones={TEXT("Phys_Wheel_FL"),TEXT("Phys_Wheel_FR"),TEXT("Phys_Wheel_BL"),TEXT("Phys_Wheel_BR")};
    for(int32 I=0;I<WheelBones.Num();++I)
    {
        auto* Zone=CreateDefaultSubobject<USphereComponent>(*FString::Printf(TEXT("WheelHitZone_%d"),I));
        Zone->SetupAttachment(GetMesh(),WheelBones[I]);Zone->SetSphereRadius(42.f);
        Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Zone->SetCollisionResponseToAllChannels(ECR_Ignore);Zone->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Zone->SetGenerateOverlapEvents(false);
        Zone->ComponentTags.Add(TEXT("WheelHitZone"));WheelHitZones.Add(Zone);
    }
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void ACXPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
        if(!FireAction)FireAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Fire.IA_Fire"));
        if(!MineAction)MineAction=LoadObject<UInputAction>(nullptr,TEXT("/Game/Input/IA_Mine.IA_Mine"));
        if(FireAction)EnhancedInputComponent->BindAction(FireAction,ETriggerEvent::Started,this,&ACXPawn::FireInput);
        if(MineAction)EnhancedInputComponent->BindAction(MineAction,ETriggerEvent::Started,this,&ACXPawn::MineInput);
		// steering
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACXPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACXPawn::Steering);

		// throttle
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACXPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACXPawn::Throttle);

		// break
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACXPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopBrake);

		// handbrake
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACXPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACXPawn::StopHandbrake);

		// look around
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACXPawn::LookAround);

		// toggle camera
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACXPawn::ToggleCamera);

		// reset the vehicle
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACXPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogTemplateVehicle, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACXPawn::Tick(float Delta)
{
	Super::Tick(Delta);
    ApplyMatchInputLock();
    if(bAcceleratorBroken)ChaosVehicleMovement->SetThrottleInput(0.f);
    // 소유자 전용 속도 값은 서버와 소유자만 사용하고, 원격 차량은 복제된 물리 결과를 표시한다.
    if((HasAuthority() || IsLocallyControlled()) && GetPlayerState<ACXPlayerState>())
    {
        const float Limit=FMath::Max(20.f,BaseMaxSpeed+GetPlayerState<ACXPlayerState>()->BonusMaxSpeed)/.036f;
        FVector Velocity=GetMesh()->GetPhysicsLinearVelocity();
        const float Horizontal=Velocity.Size2D();
        if(Horizontal>Limit)
        {
            Velocity.X*=Limit/Horizontal;Velocity.Y*=Limit/Horizontal;
            GetMesh()->SetPhysicsLinearVelocity(Velocity);
        }
    }


	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACXPawn::Steering(const FInputActionValue& Value)
{
    const float Input=Value.Get<float>();
    ChaosVehicleMovement->SetSteeringInput(CanDrive() && FMath::IsFinite(Input)?FMath::Clamp(Input,-1.f,1.f):0.f);
}

void ACXPawn::Throttle(const FInputActionValue& Value)
{
    const float Input=Value.Get<float>();
    ChaosVehicleMovement->SetThrottleInput(CanDrive() && !bAcceleratorBroken && FMath::IsFinite(Input)?FMath::Clamp(Input,-1.f,1.f):0.f);
}

void ACXPawn::Brake(const FInputActionValue& Value)
{
    ChaosVehicleMovement->SetBrakeInput(CanDrive()?FMath::Clamp(Value.Get<float>(),0.f,1.f):1.f);
}

void ACXPawn::StartBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopBrake(const FInputActionValue& Value)
{
	// call the Blueprint hook for the break lights
	BrakeLights(false);

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACXPawn::StartHandbrake(const FInputActionValue& Value)
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void ACXPawn::StopHandbrake(const FInputActionValue& Value)
{
    ChaosVehicleMovement->SetHandbrakeInput(!CanDrive());
    BrakeLights(!CanDrive());
}

void ACXPawn::LookAround(const FInputActionValue& Value)
{
	// get the flat angle value for the input
	float LookValue = Value.Get<float>();

	// add the input
	BackSpringArm->AddLocalRotation(FRotator(0.0f, LookValue, 0.0f));
}

void ACXPawn::ToggleCamera(const FInputActionValue& Value)
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACXPawn::ResetVehicle(const FInputActionValue& Value)
{
    // 클라이언트의 임의 텔레포트를 막기 위해 서버가 경기 상태와 복구 쿨다운을 다시 검사한다.
    if(CanDrive())ServerResetVehicle();
}

#undef LOCTEXT_NAMESPACE

void ACXPawn::BeginPlay()
{
    Super::BeginPlay();
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    GetMesh()->SetGenerateOverlapEvents(true);
    if(!HitParticle)HitParticle=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks"));
    if(!HitSound)HitSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/StarterContent/Audio/Collapse01.Collapse01"));
    ApplyMatchInputLock();
}
void ACXPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACXPawn,bAcceleratorBroken);
}
bool ACXPawn::IsMatchLocked() const
{
    const auto* S=GetWorld()?GetWorld()->GetGameState<ACXGameState>():nullptr;
    return !S || !S->IsPlaying();
}
bool ACXPawn::CanDrive() const
{
    if(IsMatchLocked())return false;
    const auto* PC=Cast<ACXPlayerController>(GetController());
    return !PC || !PC->IsChatOpen();
}
bool ACXPawn::CanFire() const
{
    return CanDrive() && GetWorld()->GetTimeSeconds()-LastAttackTime>=AttackCooldown;
}
void ACXPawn::ApplyMatchInputLock()
{
    if(!ChaosVehicleMovement)return;
    if(!CanDrive())
    {
        ChaosVehicleMovement->SetThrottleInput(0.f);
        ChaosVehicleMovement->SetSteeringInput(0.f);
        ChaosVehicleMovement->SetHandbrakeInput(true);
        ChaosVehicleMovement->SetBrakeInput(1.f);
    }
    else if(bStateInputLock)
    {
        // Playing으로 전환된 한 번만 해제한다. 이후에는 실제 Space 입력을 덮어쓰지 않는다.
        ChaosVehicleMovement->SetHandbrakeInput(false);
        ChaosVehicleMovement->SetBrakeInput(0.f);
    }
    bStateInputLock=!CanDrive();
}
void ACXPawn::FireInput(const FInputActionValue&) {Fire();}
void ACXPawn::MineInput(const FInputActionValue&) {PlaceMine();}
void ACXPawn::Fire() {if(CanFire())ServerFire();}
void ACXPawn::PlaceMine() {if(CanDrive())ServerPlaceMine();}

void ACXPawn::ServerFire_Implementation()
{
    if(!CanFire())return;
    LastAttackTime=GetWorld()->GetTimeSeconds();
    const FVector Start=GetActorLocation()+GetActorForwardVector()*250+GetActorUpVector()*55;
    const FVector End=Start+GetActorForwardVector()*AttackRange;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CXAttack),false,this);
    Params.bReturnPhysicalMaterial=true;
    FHitResult Hit;
    GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params);
    auto* Victim=Cast<ACXPawn>(Hit.GetActor());
    if(Victim && Victim!=this)
    {
        // 쿼리 부위 컴포넌트는 실제 부착 소켓의 Bone 이름을 PointDamage에 전달한다.
        if(Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("WheelHitZone")))Hit.BoneName=Hit.GetComponent()->GetAttachSocketName();
        LastAttackBone=Hit.BoneName;
        // PointDamage는 물리 애셋의 BoneName을 전달하므로 바퀴 피격을 판별할 수 있다.
        UGameplayStatics::ApplyPointDamage(Victim,5.f,GetActorForwardVector(),Hit,GetController(),this,UDamageType::StaticClass());
        ++SuccessfulAttacks;
    }
    MulticastAttackFX(Start,Hit.bBlockingHit?Hit.ImpactPoint:End,Victim!=nullptr);
    UE_LOG(LogTemp,Display,TEXT("CX_ATTACK victim=%s bone=%s"),*GetNameSafe(Victim),*Hit.BoneName.ToString());
}
float ACXPawn::TakeDamage(float Damage,const FDamageEvent& Event,AController* InstigatorController,AActor* Causer)
{
    auto* Attacker=Cast<ACXPawn>(Causer);
    if(!HasAuthority() || IsMatchLocked() || !Attacker || Attacker==this || Attacker->GetController()!=InstigatorController)return 0.f;
    auto* VictimPS=GetPlayerState<ACXPlayerState>();
    auto* AttackPS=Attacker->GetPlayerState<ACXPlayerState>();
    if(!VictimPS || !AttackPS)return 0.f;
    // 5원 미만이면 양쪽 잔액을 모두 그대로 둔다. 일부 금액을 강제로 빼앗지 않는다.
    if(VictimPS->Coins>=5 && AttackPS->Coins<=MAX_int32-5 && VictimPS->RemoveCoins(5))
        AttackPS->AddCoins(5);
    if(Event.IsOfType(FPointDamageEvent::ClassID))
    {
        const auto& Point=static_cast<const FPointDamageEvent&>(Event);
        if(Point.HitInfo.BoneName.ToString().Contains(TEXT("Wheel"),ESearchCase::IgnoreCase))
        {
            VictimPS->ApplyWheelPenalty(-30.f);
            if(auto* PC=Cast<ACXPlayerController>(GetController()))PC->ClientNotify(TEXT("바퀴 피격! 추가 속도 -30, 초당 10씩 회복"));
        }
    }
    return Damage;
}
void ACXPawn::MulticastAttackFX_Implementation(FVector Start,FVector End,bool bHit)
{
    if(GetNetMode()==NM_DedicatedServer)return;
    DrawDebugLine(GetWorld(),Start,End,bHit?FColor::Red:FColor::Cyan,false,.35f,0,4.f);
    if(bHit)
    {
        if(HitParticle)UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),HitParticle,End);
        if(HitSound)UGameplayStatics::PlaySoundAtLocation(this,HitSound,End,.5f);
    }
}
void ACXPawn::ServerPlaceMine_Implementation()
{
    if(!CanDrive() || GetWorld()->GetTimeSeconds()-LastMineTime<3.f)return;
    LastMineTime=GetWorld()->GetTimeSeconds();
    FActorSpawnParameters P;P.Owner=this;P.Instigator=this;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    GetWorld()->SpawnActor<ACXMine>(ACXMine::StaticClass(),GetActorLocation()-GetActorForwardVector()*360+FVector(0,0,10),FRotator::ZeroRotator,P);
    ++MinesPlaced;
}
void ACXPawn::BreakAccelerator()
{
    if(!HasAuthority() || IsMatchLocked())return;
    bAcceleratorBroken=true;OnRep_AcceleratorBroken();ForceNetUpdate();
    // 타이머를 덮어써서 연속 피격 시 마지막 피격부터 정확히 2초를 유지한다.
    GetWorldTimerManager().SetTimer(AcceleratorTimer,this,&ACXPawn::RecoverAccelerator,2.f,false);
}
void ACXPawn::RecoverAccelerator()
{
    bAcceleratorBroken=false;ForceNetUpdate();
}
void ACXPawn::OnRep_AcceleratorBroken()
{
    if(bAcceleratorBroken && ChaosVehicleMovement)ChaosVehicleMovement->SetThrottleInput(0.f);
}
void ACXPawn::ServerResetVehicle_Implementation()
{
    if(!CanDrive() || GetWorld()->GetTimeSeconds()-LastResetTime<2.f)return;
    LastResetTime=GetWorld()->GetTimeSeconds();ResetVehicleOnServer();
}
void ACXPawn::ResetVehicleOnServer()
{
    if(!HasAuthority() || IsMatchLocked())return;
    FRotator R=GetActorRotation();R.Pitch=0;R.Roll=0;
    SetActorTransform(FTransform(R,GetActorLocation()+FVector(0,0,100)),false,nullptr,ETeleportType::TeleportPhysics);
    GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
    GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

void ACXPawn::DriveForTest(float Gas,float Turn)
{
    // 실제 Enhanced Input 콜백을 통과시켜 상태 제한과 Chaos 차량의 기본 ServerUpdateState 경로까지 검증한다.
    Throttle(FInputActionValue(Gas));Steering(FInputActionValue(Turn));
}
