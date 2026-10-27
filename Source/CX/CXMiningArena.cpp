#include "CXMiningArena.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

ACXMiningArena::ACXMiningArena(){RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("경기장 원점"));}
void ACXMiningArena::OnConstruction(const FTransform& T)
{
    Super::OnConstruction(T);
    // 저장된 이전 경기장도 새 레이아웃으로 갱신한다. 형상을 바꿀 때 버전을 올린다.
    if (Generated.IsEmpty() || BuiltLayoutVersion != 3) Rebuild();
}
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
    auto* UpperRoad=Group(TEXT("Cube"),FLinearColor(.055f,.095f,.13f),true);
    auto* Structure=Group(TEXT("Cube"),FLinearColor(.24f,.32f,.36f),true);
    auto* Guard=Group(TEXT("Cube"),FLinearColor(.12f,.20f,.24f),true);
    auto* Mint=Group(TEXT("Cube"),FLinearColor(.13f,.9f,.65f),false);
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

    {
        // A/B는 도로 윗면의 중심이다. 두꺼운 충돌 슬래브를 윗면 아래에 배치하여 연결부 턱을 없앤다.
        auto Rail = [&](FVector A, FVector B)
        {
            const FVector D=B-A;
            const FRotator R=D.Rotation();
            const FVector Up=R.RotateVector(FVector::UpVector);
            Add(Guard,(A+B)*.5f+Up*55.f,{D.Size()/100.f,.5f,1.1f},R);
            Add(Mint,(A+B)*.5f+Up*110.f,{D.Size()/100.f,.65f,.08f},R);
        };
        auto RoadSection = [&](UInstancedStaticMeshComponent* Surface, FVector A, FVector B, float Width, float Thickness, bool bRails)
        {
            const FVector D=B-A;
            const FRotator R=D.Rotation();
            const FVector Up=R.RotateVector(FVector::UpVector);
            const FVector Right=R.RotateVector(FVector::RightVector);
            Add(Surface,(A+B)*.5f-Up*(Thickness*.5f),{D.Size()/100.f,Width/100.f,Thickness/100.f},R);
            if(bRails)for(float Side:{-1.f,1.f})Rail(A+Right*Side*(Width*.5f-25.f),B+Right*Side*(Width*.5f-25.f));
        };
        auto Arrow = [&](FVector P, FRotator R)
        {
            const FVector Forward=R.Vector(),Right=R.RotateVector(FVector::RightVector),Up=R.RotateVector(FVector::UpVector);
            auto Stroke=[&](FVector A,FVector B)
            {
                const FVector D=B-A;
                Add(Mint,(A+B)*.5f+Up*3.f,{D.Size()/100.f,.38f,.04f},FRotationMatrix::MakeFromXZ(D,Up).Rotator());
            };
            Stroke(P-Forward*220.f,P+Forward*220.f);
            Stroke(P+Forward*220.f,P+Right*150.f);
            Stroke(P+Forward*220.f,P-Right*150.f);
        };
        auto Sign = [&](const TCHAR* Text,FVector P,float Yaw,float Size)
        {
            auto* Label=NewObject<UTextRenderComponent>(this);Label->SetupAttachment(RootComponent);
            Label->SetText(FText::FromString(Text));Label->SetHorizontalAlignment(EHTA_Center);
            Label->SetWorldSize(Size);Label->SetTextRenderColor(FColor(125,255,220));
            Label->SetRelativeLocation(P);Label->SetRelativeRotation(FRotator(0,Yaw,0));
            Label->RegisterComponent();Generated.Add(Label);
        };
        auto Pillar = [&](float X,float Y,float Top)
        {
            if(Top>50.f)Add(Structure,{X,Y,Top*.5f},{1.8f,1.8f,Top/100.f});
        };
        auto JumpGate = [&](float X,float Y,float Base,float Width)
        {
            for(float Side:{-1.f,1.f})Add(Structure,{X,Y+Side*(Width*.5f+150.f),Base+380.f},{1.2f,1.2f,7.6f});
            Add(Structure,{X,Y,Base+790.f},{1.2f,(Width+500.f)/100.f,1.2f});
            Add(Structure,{X,Y,Base+650.f},{.8f,13.f,1.0f});
            Sign(TEXT("AIR JUMP"),{X-70.f,Y,Base+770.f},180.f,115.f);
            Sign(TEXT("80-110 KM/H"),{X-50.f,Y,Base+640.f},180.f,70.f);
        };

        // 지상 연습 코스: 완만한 진입에서 23도까지 경사를 점차 키운 뒤 열린 공중 구간으로 도약한다.
        // 별도 중력/속도 강제 변경 없이 Chaos 차량의 실제 운동량으로 체공하고 넓은 내리막에 착지한다.
        constexpr float GroundJumpY=-5800.f;
        RoadSection(Road,{-13200,GroundJumpY,8},{-8500,GroundJumpY,8},1800,30,false);
        RoadSection(Road,{-12000,-9000,8},{-12000,GroundJumpY,8},1800,30,false);
        FVector GroundRampPoint(-8500,GroundJumpY,8);
        for(float Pitch:{4.f,8.f,14.f,19.f,23.f})
        {
            const FVector Next=GroundRampPoint+FVector(1200,0,1200*FMath::Tan(FMath::DegreesToRadians(Pitch)));
            RoadSection(Concrete,GroundRampPoint,Next,1800,100,true);
            Arrow((GroundRampPoint+Next)*.5f,FRotator(Pitch,0,0));
            const FVector Mid=(GroundRampPoint+Next)*.5f;
            for(float Side:{-1.f,1.f})Pillar(Mid.X,GroundJumpY+Side*650.f,Mid.Z-100.f);
            GroundRampPoint=Next;
        }
        const FRotator GroundLipRotation(23,0,0);
        Add(Orange,GroundRampPoint-GroundLipRotation.Vector()*90.f+GroundLipRotation.RotateVector(FVector::UpVector)*4.f,
            {1.6f,17.5f,.06f},GroundLipRotation);
        RoadSection(Concrete,{0,GroundJumpY,700},{4500,GroundJumpY,8},2400,120,true);
        for(float X:{750.f,2500.f})for(float Side:{-1.f,1.f})Pillar(X,GroundJumpY+Side*900.f,700.f-X*692.f/4500.f-120.f);
        RoadSection(Road,{4500,GroundJumpY,8},{11500,GroundJumpY,8},2400,30,false);
        JumpGate(-8500,GroundJumpY,8,1800);
        Arrow({-11000,GroundJumpY,12},FRotator::ZeroRotator);
        Arrow({7500,GroundJumpY,12},FRotator::ZeroRotator);

        // 2층 진입/하강 램프. 중앙과 지하 도로는 비워 두어 1층에서도 계속 주행할 수 있다.
        constexpr float H=UpperDeckHeight;
        const FVector LeftHigh(-10500,5000,H),RightHigh(10500,5000,H);
        auto AccessHeight=[&](float T){return FMath::Lerp(8.f,H,T*T*(3.f-2.f*T));};
        // 진입과 정상부는 경사를 완만하게 낮춰 일반 2층 진입로에서 뜻하지 않게 튀어 오르지 않게 한다.
        for(float X:{-10500.f,10500.f})for(int32 I=0;I<8;++I)
        {
            const float T0=I/8.f,T1=(I+1)/8.f;
            FVector A(X,FMath::Lerp(-2500.f,5000.f,T0),AccessHeight(T0));
            FVector B(X,FMath::Lerp(-2500.f,5000.f,T1),AccessHeight(T1));
            if(X>0)Swap(A,B);
            RoadSection(UpperRoad,A,B,1600,100,true);
            if(I==2 || I==5)Arrow((A+B)*.5f,(B-A).Rotation());
        }
        RoadSection(UpperRoad,LeftHigh,{-10500,7500,H},1600,100,false);
        RoadSection(UpperRoad,{10500,7500,H},RightHigh,1600,100,false);
        RoadSection(UpperRoad,{-11300,7500,H},{-4700,7500,H},2200,100,false);
        RoadSection(UpperRoad,{2500,7500,H},{11300,7500,H},2200,100,false);
        for(float X:{-11300.f,11300.f})Rail({X,5000,H},{X,8600,H});
        for(float X:{-9700.f,9700.f})Rail({X,5000,H},{X,6400,H});
        Rail({-9700,6400,H},{-4700,6400,H});Rail({2500,6400,H},{9700,6400,H});
        Rail({-11300,8600,H},{-6700,8600,H});Rail({-5300,8600,H},{-4700,8600,H});
        Rail({2500,8600,H},{5300,8600,H});Rail({6700,8600,H},{11300,8600,H});

        // 고가 점프: 약 23m 높이에서 40m 간격을 건너 2층 도로로 착지한다.
        FVector UpperRampPoint(-4700,7500,H);
        for(float Pitch:{6.f,12.f,20.f,26.f})
        {
            const FVector Next=UpperRampPoint+FVector(800,0,800*FMath::Tan(FMath::DegreesToRadians(Pitch)));
            RoadSection(UpperRoad,UpperRampPoint,Next,2000,100,true);
            Arrow((UpperRampPoint+Next)*.5f,FRotator(Pitch,0,0));
            const FVector Mid=(UpperRampPoint+Next)*.5f;
            for(float Side:{-1.f,1.f})Pillar(Mid.X,7500.f+Side*800.f,Mid.Z-100.f);
            UpperRampPoint=Next;
        }
        const FRotator UpperLipRotation(26,0,0);
        Add(Orange,UpperRampPoint-UpperLipRotation.Vector()*90.f+UpperLipRotation.RotateVector(FVector::UpVector)*4.f,
            {1.6f,19.5f,.06f},UpperLipRotation);
        JumpGate(-4700,7500,H,2000);

        // 점프를 원하지 않으면 북쪽의 같은 높이 우회로를 이용한다. 모든 접속부의 난간을 열어 둔다.
        RoadSection(UpperRoad,{-6000,7500,H},{-6000,10300,H},1400,100,false);
        RoadSection(UpperRoad,{-6700,10300,H},{6700,10300,H},1400,100,false);
        RoadSection(UpperRoad,{6000,10300,H},{6000,7500,H},1400,100,false);
        Rail({-6700,11000,H},{6700,11000,H});Rail({-5300,9600,H},{5300,9600,H});
        Rail({-6700,8600,H},{-6700,11000,H});Rail({-5300,8600,H},{-5300,9600,H});
        Rail({6700,8600,H},{6700,11000,H});Rail({5300,8600,H},{5300,9600,H});
        Arrow({-8500,7500,H+4},FRotator::ZeroRotator);Arrow({8500,7500,H+4},FRotator::ZeroRotator);
        Arrow({-6000,9200,H+4},FRotator(0,90,0));Arrow({0,10300,H+4},FRotator::ZeroRotator);
        Arrow({6000,9200,H+4},FRotator(0,-90,0));
        for(float X:{-10000.f,-7500.f,-5000.f,3500.f,7500.f,10000.f})
        {
            Add(Structure,{X,7500,H-150},{2.2f,22.f,1.f});
            for(float Y:{6600.f,8400.f})Pillar(X,Y,H-200);
        }
        for(float X:{-6000.f,-3000.f,0.f,3000.f,6000.f})Pillar(X,10300,H-100);
        for(float X:{-6000.f,6000.f})Pillar(X,9000,H-100);
        for(float X:{-10500.f,10500.f})for(float Y:{-500.f,1800.f,4000.f})
            Pillar(X,Y,AccessHeight((Y+2500.f)/7500.f)-100.f);
        for(float X:{-1300.f,1300.f})Add(Structure,{X,9580,H+350},{1.f,1.f,7.f});
        Add(Structure,{0,9580,H+450},{30.f,.6f,5.f});
        Sign(TEXT("2F SKY CIRCUIT"),{0,9540,H+450},-90.f,170.f);
        Sign(TEXT("2F UP"),{-10500,-2300,430},-90.f,110.f);
        Sign(TEXT("SLOW CORNER"),{-10500,3800,H+250},-90.f,75.f);
        Sign(TEXT("DOWN"),{10500,4800,H+300},90.f,100.f);
        Sign(TEXT("BYPASS"),{-6400,8400,H+220},180.f,90.f);
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
    BuiltLayoutVersion = 3;
}
