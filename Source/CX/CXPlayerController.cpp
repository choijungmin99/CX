#include "CXPlayerController.h"
#include "CXPawn.h"
#include "CXGameMode.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "UI/CXGameWidget.h"
#include "UI/CXQuitDialog.h"
#include "GameFramework/PlayerInput.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"

ACXPlayerController::ACXPlayerController()
{
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultIMC(TEXT("/Game/VehicleTemplate/Input/IMC_Default.IMC_Default"));
    InputMappingContext=DefaultIMC.Object;
    PrimaryActorTick.bCanEverTick=true;
}
void ACXPlayerController::BeginPlay()
{
    Super::BeginPlay();
    // UI는 로컬 컨트롤러에만 생성한다. 데디케이티드 서버에는 화면과 LocalPlayer가 없다.
    if(!IsLocalController())return;
    TSubclassOf<UCXGameWidget> WidgetClass=GameWidgetClass;
    if(!WidgetClass)WidgetClass=LoadClass<UCXGameWidget>(nullptr,TEXT("/Game/UI/WBP_CXHUD.WBP_CXHUD_C"));
    if(!WidgetClass)WidgetClass=UCXGameWidget::StaticClass();
    GameWidget=CreateWidget<UCXGameWidget>(this,WidgetClass);
    if(GameWidget)GameWidget->AddToViewport();
    SetInputMode(FInputModeGameOnly());bShowMouseCursor=false;
    if(GetNetMode()==NM_Standalone){SetInputMode(FInputModeGameAndUI());bShowMouseCursor=true;}
    if(GetWorld()->GetGameViewport() && GetWorld()->GetGameViewport()->GetWindow())
        GetWorld()->GetGameViewport()->GetWindow()->SetTitle(FText::FromString(GetNetMode()==NM_Client?TEXT("CX - CLIENT"):TEXT("CX - HOST")));
}
void ACXPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if(auto* LP=GetLocalPlayer())if(auto* Sub=LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
    {
        if(InputMappingContext)Sub->AddMappingContext(InputMappingContext,0);
        if(bUseSteeringWheelControls && SteeringWheelInputMappingContext)Sub->AddMappingContext(SteeringWheelInputMappingContext,1);
        auto* Combat=CombatMapping?CombatMapping.Get():LoadObject<UInputMappingContext>(nullptr,TEXT("/Game/Input/IMC_Combat.IMC_Combat"));
        if(Combat)Sub->AddMappingContext(Combat,2);
    }
    InputComponent->BindKey(EKeys::Enter,IE_Pressed,this,&ACXPlayerController::FocusChat);
    InputComponent->BindKey(EKeys::T,IE_Pressed,this,&ACXPlayerController::FocusChat);
    InputComponent->BindKey(EKeys::R,IE_Pressed,this,&ACXPlayerController::Rematch);
    InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&ACXPlayerController::ToggleQuitConfirmation);
}
void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ACXPlayerController,VehiclePawn);
}
void ACXPlayerController::OnPossess(APawn* P)
{
    Super::OnPossess(P);VehiclePawn=Cast<ACXPawn>(P);
    if(VehiclePawn)VehiclePawn->ApplyMatchInputLock();
}
void ACXPlayerController::Tick(float Dt)
{
    Super::Tick(Dt);
    if(IsLocalController() && !VehiclePawn)VehiclePawn=Cast<ACXPawn>(GetPawn());
    if(bCapturingDemo && IsLocalController())
    {
        const double Now=GetWorld()->GetTimeSeconds();
        if(Now-DemoStartTime>30)
        {
            bCapturingDemo=false;
            UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("Audio"),DemoFrameFolder,nullptr);
        }
        else if(Now-LastDemoFrame>=.1 && !FScreenshotRequest::IsScreenshotRequested())
        {
            LastDemoFrame=Now;
            // 게임의 실제 렌더 타깃을 읽는다. DX12 창을 GDI로 녹화할 때 생기는 정지 화면을 피한다.
            FScreenshotRequest::RequestScreenshot(DemoFrameFolder/FString::Printf(TEXT("Frame_%05d.png"),DemoFrameIndex++),true,false);
        }
    }
}
void ACXPlayerController::FocusChat()
{
    if(!IsLocalController() || !GameWidget || bChatOpen || IsQuitDialogOpen())return;
    bChatOpen=true;
    if(auto* P=Cast<ACXPawn>(GetPawn()))P->ApplyMatchInputLock();
    FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);bShowMouseCursor=true;
    GameWidget->FocusChatInput();
}
void ACXPlayerController::CloseChat()
{
    bChatOpen=false;
    if(!IsQuitDialogOpen())RestoreLocalInputMode();
}
void ACXPlayerController::RestoreLocalInputMode()
{
    if(PlayerInput)PlayerInput->FlushPressedKeys();
    if(GetNetMode()==NM_Standalone)
    {
        FInputModeGameAndUI Mode;Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);bShowMouseCursor=true;
    }
    else {SetInputMode(FInputModeGameOnly());bShowMouseCursor=false;}
}
bool ACXPlayerController::IsQuitDialogOpen() const {return IsValid(QuitDialog)&&QuitDialog->IsInViewport();}
void ACXPlayerController::ToggleQuitConfirmation()
{
    if(!IsLocalController())return;
    if(IsQuitDialogOpen()){CancelQuit();return;}
    QuitDialog=CreateWidget<UCXQuitDialog>(this,UCXQuitDialog::StaticClass());
    if(!QuitDialog)return;
    bRestoreChatAfterQuitCancel=bChatOpen;bChatOpen=false;
    QuitDialog->AddToViewport(1000);
    if(PlayerInput)PlayerInput->FlushPressedKeys();
    if(auto* Car=Cast<ACXPawn>(GetPawn()))Car->ApplyMatchInputLock();
    // 리슨 서버의 경기 전체를 멈추지 않고 이 창의 입력만 UI로 전환한다.
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(QuitDialog->TakeWidget());Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);bShowMouseCursor=true;QuitDialog->FocusContinueButton();
}
void ACXPlayerController::CancelQuit()
{
    if(!IsQuitDialogOpen())return;
    QuitDialog->RemoveFromParent();QuitDialog=nullptr;
    const bool bRestoreChat=bRestoreChatAfterQuitCancel;bRestoreChatAfterQuitCancel=false;
    RestoreLocalInputMode();
    if(bRestoreChat)FocusChat();
    if(auto* Car=Cast<ACXPawn>(GetPawn()))Car->ApplyMatchInputLock();
}
void ACXPlayerController::ConfirmQuit()
{
    // 최종 확인 버튼에서만 이 로컬 게임 창을 종료한다.
    if(IsLocalController()&&IsQuitDialogOpen())UKismetSystemLibrary::QuitGame(this,this,EQuitPreference::Quit,false);
}
void ACXPlayerController::SubmitChat(const FString& Text)
{
    if(IsQuitDialogOpen())return;
    const FString Clean=Text.TrimStartAndEnd();
    if(!Clean.IsEmpty())ServerSendChat(Clean);
    CloseChat();
}
void ACXPlayerController::ServerSendChat_Implementation(const FString& Message)
{
    // 발신자는 클라이언트 문자열이 아닌 서버의 PlayerState로 결정한다.
    FString Clean=Message.TrimStartAndEnd();
    if(Clean.IsEmpty() || Clean.Len()>200 || GetWorld()->GetTimeSeconds()-LastChatTime<.35)return;
    Clean.ReplaceInline(TEXT("\n"),TEXT(" "));Clean.ReplaceInline(TEXT("\r"),TEXT(" "));
    for(TCHAR Ch:Clean)if(Ch<32)return;
    auto* PS=GetPlayerState<ACXPlayerState>();if(!PS)return;
    LastChatTime=GetWorld()->GetTimeSeconds();
    for(FConstPlayerControllerIterator It=GetWorld()->GetPlayerControllerIterator();It;++It)
        if(auto* PC=Cast<ACXPlayerController>(It->Get()))PC->ClientReceiveChat(PS->GetPlayerName(),Clean);
    UE_LOG(LogTemp,Display,TEXT("CX_CHAT sender=%s length=%d"),*PS->GetPlayerName(),Clean.Len());
}
void ACXPlayerController::ClientReceiveChat_Implementation(const FString& Sender,const FString& Message)
{
    ++ReceivedChatMessages;
    if(GameWidget)GameWidget->AddChatMessage(Sender,Message);
}
void ACXPlayerController::ClientNotify_Implementation(const FString& Message)
{
    if(GameWidget)GameWidget->ShowNotice(Message);
}
void ACXPlayerController::Rematch()
{
    if(!bChatOpen && !IsQuitDialogOpen())ServerRequestRematch();
}
void ACXPlayerController::ServerRequestRematch_Implementation()
{
    // 경기 중 재시작 요청은 GameMode에서 거부한다.
    if(auto* GM=GetWorld()->GetAuthGameMode<ACXGameMode>())GM->RestartRound();
}

#include "Tests/CXNetworkTestDirector.h"
#include "UI/ChatUI.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"

void ACXPlayerController::ClientTestAction_Implementation(int32 Action)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXNetTest")))return;
    auto* Car=Cast<ACXPawn>(GetPawn());if(!Car)return;
    switch(Action)
    {
    case 0: Car->DriveForTest(1,1);Car->ServerFire();Car->ServerPlaceMine();Car->ServerResetVehicle();break;
    case 1: if(GameWidget && GameWidget->Chat)GameWidget->Chat->OnChatInputTextCommitted(FText::FromString(TEXT("호스트: 코인을 모으러 출발합니다!")),ETextCommit::OnEnter);break;
    case 2: if(GameWidget && GameWidget->Chat)GameWidget->Chat->OnChatInputTextCommitted(FText::FromString(TEXT("게스트: 채팅 동기화 확인했습니다.")),ETextCommit::OnEnter);break;
    case 3: Car->ServerFire();break;
    case 4: Car->ServerPlaceMine();break;
    case 6: for(int32 I=0;I<20;++I)Car->ServerFire();break;
    case 7:
    {
        // PlayerController RPC가 GameState의 Playing 복제보다 먼저 도착할 수 있다.
        // 실제 W 입력은 매 프레임 Triggered되지만 단발 검증 입력은 상태 동기화 후 주입한다.
        TWeakObjectPtr<ACXPawn> WeakCar(Car);FTimerHandle H;
        GetWorldTimerManager().SetTimer(H,[WeakCar]()
        {
            if(auto* P=WeakCar.Get())
            {
                P->DriveForTest(1,0);
                if(FParse::Param(FCommandLine::Get(),TEXT("CXRecoveryTest")))
                    if(auto* PC=Cast<ACXPlayerController>(P->GetController()))
                        PC->ServerTestResult(TEXT("owner_recovery_drive_input_received"),P->CanDrive() && !P->bAcceleratorBroken);
            }
        },.4f,false);
        break;
    }
    case 11: Car->DriveForTest(0,0);break;
    case 14: Car->ServerResetVehicle();break;
    case 13:
        ServerSendChat(TEXT(""));ServerSendChat(FString::ChrN(250,TCHAR('x')));
        if(GetNetMode()==NM_Client)
        {
            auto* PS=GetPlayerState<ACXPlayerState>();
            const bool Rejected=PS && !PS->AddCoins(999) && !PS->RemoveCoins(1) && !PS->AddBonusMaxSpeed(999);
            ServerTestResult(TEXT("client_cannot_mutate_stats"),Rejected);
        }
        break;
    }
}
void ACXPlayerController::ClientTestProbe_Implementation(const FString& Label,int32 ExpectedCoins,float ExpectedBonus,int32 Phase)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXNetTest")))return;
    TWeakObjectPtr<ACXPlayerController> WeakThis(this);
    FTimerHandle Handle;
    // 다른 액터 채널의 프로퍼티 복제를 기다린 후 검사한다. RPC와 Actor 복제 도착 순서는 동일하지 않다.
    GetWorldTimerManager().SetTimer(Handle,[WeakThis,Label,ExpectedCoins,ExpectedBonus,Phase]()
    {
        auto* PC=WeakThis.Get();if(!PC)return;
        auto* PS=PC->GetPlayerState<ACXPlayerState>();auto* S=PC->GetWorld()->GetGameState<ACXGameState>();
        auto* Car=Cast<ACXPawn>(PC->GetPawn());
        bool Good=PS && S && Car && PC->GameWidget && PC->GameWidget->Chat;
        Good=Good && PS->Coins==ExpectedCoins && FMath::Abs(PS->BonusMaxSpeed-ExpectedBonus)<5;
        Good=Good && int32(S->MatchPhase)==Phase && Car->CanDrive()==(Phase==1);
        PC->ServerTestResult(Label+TEXT("_local_ui_state"),Good);
        if(PC->GetNetMode()==NM_Client && S)
        {
            bool Private=true;
            for(APlayerState* Base:S->PlayerArray)if(auto* Other=Cast<ACXPlayerState>(Base))
                if(Other!=PS)Private &= Other->Coins==0 && Other->BonusMaxSpeed==0;
            PC->ServerTestResult(Label+TEXT("_owner_only_privacy"),Private);
        }
        if(Label==TEXT("finished_single") || Label==TEXT("finished_tie"))
            PC->ServerTestResult(TEXT("chat_received_on_both_clients"),PC->ReceivedChatMessages==2);
    },.45f,false);
}
void ACXPlayerController::ServerTestResult_Implementation(const FString& Label,bool Pass)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXNetTest")))return;
    for(TActorIterator<ACXNetworkTestDirector> It(GetWorld());It;++It){It->Record(Label,Pass);break;}
}
void ACXPlayerController::ClientStartDemoCapture_Implementation()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXRecordDemo")))return;
    DemoFrameFolder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("DemoFrames")/(GetNetMode()==NM_Client?TEXT("CLIENT"):TEXT("HOST")));
    IFileManager::Get().MakeDirectory(*DemoFrameFolder,true);
    DemoStartTime=GetWorld()->GetTimeSeconds();LastDemoFrame=-100;DemoFrameIndex=0;bCapturingDemo=true;
    UAudioMixerBlueprintLibrary::StartRecordingOutput(this,32.f,nullptr);
}
void ACXPlayerController::ClientTestPlaceCar_Implementation(FVector P)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXNetTest")))return;
    if(auto* Car=Cast<ACXPawn>(GetPawn()))
    {
        // 비물리 시험 배치는 autonomous 소유자에도 명시적으로 맞춰 양쪽 시연 시점을 일치시킨다.
        Car->GetMesh()->SetSimulatePhysics(false);
        Car->SetActorLocationAndRotation(P,FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
    }
}

void ACXPlayerController::ClientCourseTestDrive_Implementation(FVector Location, FRotator Rotation)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXCourseTest")) && !FParse::Param(FCommandLine::Get(),TEXT("CXRecoveryTest")))return;
    // 첫 접속의 소유 Pawn 참조와 Playing 복제가 도착한 뒤 배치와 입력을 함께 적용한다.
    const TWeakObjectPtr<ACXPlayerController> WeakController(this);FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle,[WeakController,Location,Rotation]()
    {
        auto* PC=WeakController.Get();if(!PC)return;
        auto* Car=Cast<ACXPawn>(PC->GetPawn());if(!Car)return;
        Car->GetChaosVehicleMovement()->ResetVehicle();
        Car->SetActorLocationAndRotation(Location,Rotation,false,nullptr,ETeleportType::TeleportPhysics);
        Car->GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
        Car->GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Car->GetMesh()->WakeAllRigidBodies();
        Car->DriveForTest(1.f,0.f);
        UE_LOG(LogTemp,Display,TEXT("CX_COURSE_CLIENT_DRIVE %s"),*Location.ToString());
    },.8f,false);
}

void ACXPlayerController::ClientCourseScreenshot_Implementation(const FString& FileName)
{
    if((!FParse::Param(FCommandLine::Get(),TEXT("CXCourseTest")) && !FParse::Param(FCommandLine::Get(),TEXT("CXRecoveryTest"))) || FScreenshotRequest::IsScreenshotRequested())return;
    const FString Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Tests/CourseScreens"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FScreenshotRequest::RequestScreenshot(Folder/FPaths::GetCleanFilename(FileName),true,false);
}

void ACXPlayerController::ClientRecoveryTestProbe_Implementation(FVector ExpectedLocation)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("CXRecoveryTest")))return;
    const TWeakObjectPtr<ACXPlayerController> WeakPC(this);FTimerHandle Handle;
    GetWorldTimerManager().SetTimer(Handle,[WeakPC,ExpectedLocation]()
    {
        auto* PC=WeakPC.Get();if(!PC)return;auto* Car=Cast<ACXPawn>(PC->GetPawn());if(!Car)return;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(CXRecoveryOwnerTest),false,Car);
        const bool Clear=!PC->GetWorld()->OverlapBlockingTestByChannel(Car->GetActorLocation()+Car->GetActorUpVector()*70.f,
            Car->GetActorQuat(),ECC_Vehicle,FCollisionShape::MakeBox(FVector(160,75,30)),Params);
        PC->ServerTestResult(TEXT("owner_recovery_replicated_and_clear"),Clear && Car->CanDrive() && Car->GetMesh()->IsSimulatingPhysics()
            && FVector::Dist(Car->GetActorLocation(),ExpectedLocation)<160.f);
    },.3f,false);
}
