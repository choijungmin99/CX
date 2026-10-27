#include "CXLaserShotEffect.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACXLaserShotEffect::ACXLaserShotEffect()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    bReplicates = false;
    SetActorEnableCollision(false);
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WhiteCore"));
    Glow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CyanGlow"));
    Muzzle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MuzzleFlash"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    for (UStaticMeshComponent* Part : { Core.Get(), Glow.Get(), Muzzle.Get() })
    {
        Part->SetupAttachment(RootComponent);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        Part->SetCastShadow(false);
        Part->SetReceivesDecals(false);
        Part->SetStaticMesh(Part == Muzzle ? Sphere.Object : Cylinder.Object);
    }
    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
    MuzzleLight->SetupAttachment(RootComponent);
    MuzzleLight->SetLightColor(FLinearColor(.25f, .8f, 1.f));
    MuzzleLight->SetAttenuationRadius(220.f);
    MuzzleLight->SetCastShadows(false);
}

void ACXLaserShotEffect::Initialize(AActor* Shooter, FVector Start, FVector End, UMaterialInterface* Material, float Duration)
{
    Source = Shooter;
    InitialStart = Start;
    LocalStart = Shooter ? Shooter->GetActorTransform().InverseTransformPosition(Start) : Start;
    WorldEnd = End;
    Lifetime = FMath::Clamp(Duration, .08f, .3f);
    if (Material)
    {
        CoreMaterial = UMaterialInstanceDynamic::Create(Material, this);
        GlowMaterial = UMaterialInstanceDynamic::Create(Material, this);
        MuzzleMaterial = UMaterialInstanceDynamic::Create(Material, this);
        CoreMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(12.f, 22.f, 26.f));
        GlowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(.08f, 3.5f, 7.f));
        MuzzleMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(2.f, 10.f, 16.f));
        Core->SetMaterial(0, CoreMaterial);
        Glow->SetMaterial(0, GlowMaterial);
        Muzzle->SetMaterial(0, MuzzleMaterial);
    }
    // 첫 프레임부터 가는 발광선이 보이며 약 25ms 만에 명중 위치까지 뻗는다.
    Age = .008f;
    UpdateVisuals();
    SetLifeSpan(Lifetime);
}

void ACXLaserShotEffect::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    UpdateVisuals();
}

void ACXLaserShotEffect::UpdateVisuals()
{
    // 발사 중에도 앞 범퍼를 따라가므로 주행 중 발사구가 공중에 남지 않는다.
    const AActor* Shooter = Source.Get();
    const FVector Start = Shooter ? Shooter->GetActorTransform().TransformPosition(LocalStart) : InitialStart;
    const FVector VisibleEnd = FMath::Lerp(Start, WorldEnd, FMath::Clamp(Age / .025f, 0.f, 1.f));
    const FVector Direction = VisibleEnd - Start;
    const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector));
    const float Fade = FMath::Pow(FMath::Clamp(1.f - Age / Lifetime, 0.f, 1.f), 1.5f);
    const float Width = .8f + .2f * Fade;
    const FVector Center = (Start + VisibleEnd) * .5f;
    Core->SetWorldTransform(FTransform(Rotation, Center, FVector(1.1f * Width / 50.f, 1.1f * Width / 50.f, Direction.Size() / 100.f)));
    Glow->SetWorldTransform(FTransform(Rotation, Center, FVector(3.6f * Width / 50.f, 3.6f * Width / 50.f, Direction.Size() / 100.f)));
    const float Flash = FMath::Exp(-Age / .022f);
    Muzzle->SetWorldLocation(Start);
    Muzzle->SetWorldScale3D(FVector((4.f + 7.f * Flash) / 50.f));
    MuzzleLight->SetWorldLocation(Start);
    MuzzleLight->SetIntensity(1800.f * Flash);
    if (CoreMaterial) CoreMaterial->SetScalarParameterValue(TEXT("Fade"), Fade);
    if (GlowMaterial) GlowMaterial->SetScalarParameterValue(TEXT("Fade"), Fade * .45f);
    if (MuzzleMaterial) MuzzleMaterial->SetScalarParameterValue(TEXT("Fade"), Flash);
}
