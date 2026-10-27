#include "CXMiningArena.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ACXMiningArena::ACXMiningArena(){RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("경기장 원점"));}
void ACXMiningArena::OnConstruction(const FTransform& T){Super::OnConstruction(T);if(Generated.IsEmpty())Rebuild();}
UInstancedStaticMeshComponent* ACXMiningArena::Group(const TCHAR* Name,FLinearColor C,bool Collision)
{
    auto* M=NewObject<UInstancedStaticMeshComponent>(this);M->SetupAttachment(RootComponent);
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Name,Name)));
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Mining/M_ArenaTint.M_ArenaTint"));
    if(Base){auto* MID=UMaterialInstanceDynamic::Create(Base,this);MID->SetVectorParameterValue(TEXT("Tint"),C);M->SetMaterial(0,MID);}
    M->SetCollisionProfileName(Collision?TEXT("BlockAll"):TEXT("NoCollision"));M->SetMobility(EComponentMobility::Static);M->RegisterComponent();Generated.Add(M);return M;
}
void ACXMiningArena::Rebuild()
{
    for(UActorComponent* C:Generated)if(IsValid(C))C->DestroyComponent();Generated.Empty();
    auto* Ground=Group(TEXT("Cube"),FLinearColor(.10f,.19f,.12f),true);
    auto* Road=Group(TEXT("Cube"),FLinearColor(.055f,.07f,.075f),true);
    auto* Concrete=Group(TEXT("Cube"),FLinearColor(.30f,.35f,.34f),true);
    auto* White=Group(TEXT("Cube"),FLinearColor(.85f,.86f,.72f),false);
    auto* Orange=Group(TEXT("Cube"),FLinearColor(1,.4f,.07f),false);
    auto Add=[](UInstancedStaticMeshComponent* M,FVector P,FVector S,FRotator R=FRotator::ZeroRotator){M->AddInstance(FTransform(R,P,S));};
    Add(Ground,{0,0,-100},{420,340,2});
    Add(Road,{0,0,4},{260,90,.08f});
    Add(Road,{0,9000,4},{260,40,.08f});Add(Road,{0,-9000,4},{260,40,.08f});
    Add(Road,{-11000,0,4},{40,220,.08f});Add(Road,{11000,0,4},{40,220,.08f});
    for(int32 X=-20;X<=20;++X)
        for(float Y:{-9000.f,0.f,9000.f})Add(White,{X*550.f,Y,10},{2.5f,.16f,.025f});
    // 대칭 램프: 양방향에서 오르고 착지할 수 있다. 정적 큐브 충돌을 사용하여 Chaos 차량을 지지한다.
    for(float Y:{-1600.f,1600.f})
    {
        Add(Concrete,{-1500,Y,280},{36,13,1.2f},FRotator(9,0,0));
        Add(Concrete,{2000,Y,280},{36,13,1.2f},FRotator(-9,0,0));
    }
    for(int32 I=-27;I<=27;++I)for(float Y:{-12200.f,12200.f})
    {
        Add(Concrete,{I*550.f,Y,100},{5.5f,.7f,2});Add((I%2)?White:Orange,{I*550.f,Y-45,110},{5.3f,.08f,1.2f});
    }
    for(int32 I=-21;I<=21;++I)for(float X:{-15000.f,15000.f})Add(Concrete,{X,I*550.f,100},{.7f,5.5f,2});
    auto* Trunk=Group(TEXT("Cylinder"),FLinearColor(.13f,.07f,.04f),true);
    auto* Pine=Group(TEXT("Cone"),FLinearColor(.045f,.24f,.14f),false);
    auto* Pine2=Group(TEXT("Cone"),FLinearColor(.08f,.3f,.17f),false);
    auto* Rocks=Group(TEXT("Cube"),FLinearColor(.32f,.35f,.34f),true);
    FRandomStream R(127);
    for(int32 I=0;I<460;++I)
    {
        float X=R.FRandRange(-28000,28000),Y=R.FRandRange(-23000,23000);
        if(FMath::Abs(X)<16500 && FMath::Abs(Y)<13500)continue;
        const float Height=R.FRandRange(1500,2800);
        Add(Trunk,{X,Y,Height*.3f},{1,1,Height*.006f});
        for(int32 J=0;J<3;++J)Add(I%2?Pine:Pine2,{X,Y,Height*(.42f+.19f*J)},{(12.f-2.8f*J),(12.f-2.8f*J),Height*.008f});
    }
    for(int32 I=-14;I<=14;++I)
    {
        float H=R.FRandRange(50,110);Add(Rocks,{I*2100.f,18500,H*40},{R.FRandRange(25,42),R.FRandRange(15,28),H},FRotator(0,R.FRandRange(-18,18),R.FRandRange(-8,8)));
    }
    Add(Concrete,{-9000,0,1450},{1.5f,100,5});
    for(float Y:{-5000.f,5000.f})Add(Concrete,{-9000,Y,650},{2,2,13});
    auto* Sign=NewObject<UTextRenderComponent>(this);Sign->SetupAttachment(RootComponent);Sign->SetText(FText::FromString(TEXT("WON MINING  /  FOREST CIRCUIT")));Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetWorldSize(210);Sign->SetRelativeLocation({-9100,0,1510});Sign->SetRelativeRotation(FRotator(0,180,0));Sign->RegisterComponent();Generated.Add(Sign);
}
