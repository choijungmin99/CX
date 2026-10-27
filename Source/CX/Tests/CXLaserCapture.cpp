#include "Tests/CXLaserCapture.h"
#include "CXLaserShotEffect.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void CXRecordLaserFXForTest(AActor* Context, ACXLaserShotEffect* Beam, UAudioComponent* Audio, bool bHit)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("CXNetTest"))) return;
    UWorld* World = Context->GetWorld();
    const FString Role = Context->GetNetMode() == NM_Client ? TEXT("CLIENT") : TEXT("HOST");
    UE_LOG(LogTemp, Display, TEXT("CX_LASER_FX role=%s beam=%d sound=%d hit=%d"), *Role, IsValid(Beam), IsValid(Audio), bHit);
    const TWeakObjectPtr<ACXLaserShotEffect> WeakBeam(Beam);
    const TWeakObjectPtr<UAudioComponent> WeakAudio(Audio);
    FTimerHandle CleanupTimer;
    World->GetTimerManager().SetTimer(CleanupTimer, [WeakBeam, WeakAudio, Role]()
    {
        const UAudioComponent* Sound = WeakAudio.Get();
        UE_LOG(LogTemp, Display, TEXT("CX_LASER_CLEANUP role=%s beam_gone=%d sound_stopped=%d"),
            *Role, !WeakBeam.IsValid(), !Sound || !Sound->IsPlaying());
    }, .8f, false);
    if (!FParse::Param(FCommandLine::Get(), TEXT("CXLaserCapture"))) return;
    // 검증 캡처에서만 측면 카메라를 사용해 차량에 가려지는 발사구와 광선을 함께 확인한다.
    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    const TWeakObjectPtr<APlayerController> WeakPC(PC);
    const TWeakObjectPtr<AActor> PreviousView(PC ? PC->GetViewTarget() : nullptr);
    ACameraActor* Camera = World->SpawnActor<ACameraActor>();
    if (Camera && PC)
    {
        const FVector Position = Context->GetActorLocation();
        const FVector CameraLocation = Position + FVector(-450.f, -950.f, 600.f);
        Camera->SetActorLocationAndRotation(CameraLocation, (Position + FVector(800.f, 0.f, 40.f) - CameraLocation).Rotation());
        Camera->GetCameraComponent()->SetFieldOfView(70.f);
        PC->SetViewTarget(Camera);
    }
    const TWeakObjectPtr<ACameraActor> WeakCamera(Camera);
    FTimerHandle RestoreTimer;
    World->GetTimerManager().SetTimer(RestoreTimer, [WeakPC, PreviousView, WeakCamera]()
    {
        if (WeakPC.IsValid() && PreviousView.IsValid()) WeakPC->SetViewTarget(PreviousView.Get());
        if (WeakCamera.IsValid()) WeakCamera->Destroy();
    }, .2f, false);
    static int32 CaptureIndex = 0;
    const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Tests/LaserScreens"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Filename = Folder / FString::Printf(TEXT("Laser_%s_%02d.png"), *Role, CaptureIndex++);
    FTimerHandle ScreenshotTimer;
    World->GetTimerManager().SetTimer(ScreenshotTimer, [Filename]()
    {
        if (!FScreenshotRequest::IsScreenshotRequested()) FScreenshotRequest::RequestScreenshot(Filename, true, false);
    }, .035f, false);
}
