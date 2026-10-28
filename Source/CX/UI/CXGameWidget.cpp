#include "CXGameWidget.h"
#include "ChatUI.h"
#include "CXGameState.h"
#include "CXPlayerState.h"
#include "CXPlayerController.h"
#include "CXPawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

namespace CXUIStyle
{
    const FLinearColor Mint(.12f,.92f,.67f);
    const FLinearColor Gold(1.f,.68f,.16f);
    const FLinearColor Dark(.012f,.025f,.04f,.9f);
    const FLinearColor Muted(.55f,.68f,.74f);
}
UTextBlock* UCXGameWidget::Label(const FString& S,int32 Size,FLinearColor Color)
{
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(S));
    T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",Size));T->SetColorAndOpacity(FSlateColor(Color));return T;
}
void UCXGameWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
    auto Panel=[&](FVector2D Anchor,FVector2D Align,FVector2D Pos,FVector2D Size)
    {
        auto* B=WidgetTree->ConstructWidget<UBorder>();B->SetBrushColor(CXUIStyle::Dark);B->SetPadding(FMargin(18));
        auto* Slot=Root->AddChildToCanvas(B);Slot->SetAnchors(FAnchors(Anchor.X,Anchor.Y));Slot->SetAlignment(Align);Slot->SetPosition(Pos);Slot->SetSize(Size);return B;
    };
    auto* Personal=Panel({0,0},{0,0},{24,24},{310,215});auto* PBox=WidgetTree->ConstructWidget<UVerticalBox>();Personal->SetContent(PBox);
    PBox->AddChildToVerticalBox(Label(TEXT("분노의 질주 : 원화 채굴"),19,CXUIStyle::Mint));
    PBox->AddChildToVerticalBox(Label(TEXT("나의 원화"),13,CXUIStyle::Muted))->SetPadding(FMargin(0,12,0,0));
    CoinsText=Label(TEXT("0 원"),38,CXUIStyle::Gold);PBox->AddChildToVerticalBox(CoinsText);
    BonusText=Label(TEXT("추가 최고 속도  +0 km/h"),16);BonusText->SetToolTipText(FText::FromString(TEXT("속도 아이템은 +10 km/h, 바퀴 피격은 일시적으로 -30 km/h를 적용합니다.")));PBox->AddChildToVerticalBox(BonusText);
    DamageText=Label(TEXT("차량 정상"),13,CXUIStyle::Mint);PBox->AddChildToVerticalBox(DamageText)->SetPadding(FMargin(0,8,0,0));
    auto* Status=Panel({1,0},{1,0},{-24,24},{320,205});auto* SBox=WidgetTree->ConstructWidget<UVerticalBox>();Status->SetContent(SBox);
    PhaseText=Label(TEXT("출발 대기"),18,CXUIStyle::Mint);SBox->AddChildToVerticalBox(PhaseText);
    ClockText=Label(TEXT("00:10"),38);SBox->AddChildToVerticalBox(ClockText);
    SBox->AddChildToVerticalBox(Label(TEXT("시간 안에 가장 많은 원화를!"),12,CXUIStyle::Muted));
    // 한 창만 실행한 경우와 실제 상대가 접속한 경우를 화면에서 구분한다.
    // 공개 접속 인원만 표시하고 다른 플레이어의 개인 원화는 읽지 않는다.
    ConnectionText=Label(TEXT("접속 상태 확인 중"),14,CXUIStyle::Gold);
    SBox->AddChildToVerticalBox(ConnectionText)->SetPadding(FMargin(0,8,0,0));
    ConnectionHintText=Label(TEXT("상대 플레이어 접속 대기"),12,CXUIStyle::Muted);
    ConnectionHintText->SetAutoWrapText(true);
    ConnectionHintText->SetToolTipText(FText::FromString(TEXT("다른 차량은 접속한 플레이어가 운전합니다. 같은 PC에서는 각 게임 창을 클릭한 뒤 운전하세요.")));
    SBox->AddChildToVerticalBox(ConnectionHintText);
    auto* Driving=Panel({1,1},{1,1},{-24,-24},{340,205});auto* DBox=WidgetTree->ConstructWidget<UVerticalBox>();Driving->SetContent(DBox);
    SpeedText=Label(TEXT("000 km/h"),32);DBox->AddChildToVerticalBox(SpeedText);
    DBox->AddChildToVerticalBox(Label(TEXT("W/S 가속·제동  |  A/D 조향"),14,CXUIStyle::Muted));
    DBox->AddChildToVerticalBox(Label(TEXT("좌클릭 범퍼 공격  |  F 지뢰 설치"),14,CXUIStyle::Muted));
    DBox->AddChildToVerticalBox(Label(TEXT("Space 핸드브레이크  |  Backspace 복구"),12,CXUIStyle::Muted));
    DBox->AddChildToVerticalBox(Label(TEXT("Enter 채팅  |  R 종료 후 다시 경기"),13,CXUIStyle::Mint));
    DBox->AddChildToVerticalBox(Label(TEXT("Esc 게임 종료"),13,CXUIStyle::Mint));
    TSubclassOf<UChatUI> ChatClass=ChatWidgetClass;
    if(!ChatClass)ChatClass=LoadClass<UChatUI>(nullptr,TEXT("/Game/UI/WBP_ChatUI.WBP_ChatUI_C"));
    if(!ChatClass)ChatClass=UChatUI::StaticClass();
    Chat=CreateWidget<UChatUI>(GetOwningPlayer(),ChatClass);
    auto* ChatSlot=Root->AddChildToCanvas(Chat);ChatSlot->SetAnchors(FAnchors(0,1));ChatSlot->SetAlignment({0,1});ChatSlot->SetPosition({24,-24});ChatSlot->SetSize({500,215});
    NoticeText=Label(TEXT("금색 코인 +10원  /  민트 버프 +10 km/h"),20,CXUIStyle::Gold);
    auto* NS=Root->AddChildToCanvas(NoticeText);NS->SetAnchors(FAnchors(.5f,.22f));NS->SetAlignment({.5f,0});NS->SetAutoSize(true);
    ResultPanel=Panel({.5f,.44f},{.5f,.5f},{0,0},{540,200});auto* RBox=WidgetTree->ConstructWidget<UVerticalBox>();ResultPanel->SetContent(RBox);
    ResultText=Label(TEXT("경기 종료"),30,CXUIStyle::Gold);ResultText->SetJustification(ETextJustify::Center);RBox->AddChildToVerticalBox(ResultText);
    auto* RHint=Label(TEXT("동점 최고 기록은 공동 우승으로 처리됩니다."),14,CXUIStyle::Muted);RHint->SetJustification(ETextJustify::Center);RBox->AddChildToVerticalBox(RHint)->SetPadding(FMargin(0,16));
    auto* RButton=WidgetTree->ConstructWidget<UButton>();RButton->SetContent(Label(TEXT("다시 경기  [R]"),18));RButton->OnClicked.AddDynamic(this,&UCXGameWidget::Rematch);RBox->AddChildToVerticalBox(RButton);
    ResultPanel->SetVisibility(ESlateVisibility::Collapsed);
    NetworkPanel=Panel({1,0},{1,0},{-24,245},{320,155});auto* NBox=WidgetTree->ConstructWidget<UVerticalBox>();NetworkPanel->SetContent(NBox);
    auto* Host=WidgetTree->ConstructWidget<UButton>();Host->SetContent(Label(TEXT("리슨 서버 열기"),15));Host->OnClicked.AddDynamic(this,&UCXGameWidget::HostGame);NBox->AddChildToVerticalBox(Host);
    IPInput=WidgetTree->ConstructWidget<UEditableTextBox>();IPInput->SetText(FText::FromString(TEXT("127.0.0.1")));IPInput->SetToolTipText(FText::FromString(TEXT("접속할 호스트의 IP 주소입니다. 같은 PC의 두 창은 127.0.0.1을 사용하세요.")));NBox->AddChildToVerticalBox(IPInput)->SetPadding(FMargin(0,8));
    auto* Join=WidgetTree->ConstructWidget<UButton>();Join->SetContent(Label(TEXT("서버에 접속"),15));Join->OnClicked.AddDynamic(this,&UCXGameWidget::JoinGame);NBox->AddChildToVerticalBox(Join);
}
void UCXGameWidget::NativeTick(const FGeometry& G,float Dt)
{
    Super::NativeTick(G,Dt);
    auto* PC=GetOwningPlayer<ACXPlayerController>();if(!PC)return;
    // 절대 PlayerArray를 순회해 상대의 Coins를 UI에 표시하지 않는다.
    auto* PS=PC->GetPlayerState<ACXPlayerState>();auto* S=GetWorld()->GetGameState<ACXGameState>();
    const ENetMode NetMode=PC->GetNetMode();
    const int32 PlayerCount=S?S->PlayerArray.Num():0;
    const TCHAR* Role=NetMode==NM_Client?TEXT("클라이언트"):NetMode==NM_ListenServer?TEXT("리슨 호스트"):TEXT("오프라인");
    ConnectionText->SetText(FText::FromString(FString::Printf(TEXT("%s · 접속 %d/2명"),Role,PlayerCount)));
    const bool bConnectedPair=NetMode!=NM_Standalone && PlayerCount>=2;
    ConnectionText->SetColorAndOpacity(FSlateColor(bConnectedPair?CXUIStyle::Mint:CXUIStyle::Gold));
    ConnectionHintText->SetText(FText::FromString(NetMode==NM_Standalone
        ?TEXT("2인 실행 또는 서버 열기/접속이 필요합니다.")
        :TEXT("상대 플레이어 접속 대기")));
    ConnectionHintText->SetVisibility(bConnectedPair?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
    if(PS){CoinsText->SetText(FText::FromString(FString::Printf(TEXT("%d 원"),PS->Coins)));BonusText->SetText(FText::FromString(FString::Printf(TEXT("추가 최고 속도  %+.1f km/h"),PS->BonusMaxSpeed)));}
    if(auto* Car=Cast<ACXPawn>(PC->GetPawn()))
    {
        SpeedText->SetText(FText::FromString(FString::Printf(TEXT("%03.0f km/h"),FMath::Abs(Car->GetChaosVehicleMovement()->GetForwardSpeed())*.036f)));
        DamageText->SetText(FText::FromString(Car->bAcceleratorBroken?TEXT("지뢰 피격 · 가속 고장 2초"):TEXT("차량 정상")));
        DamageText->SetColorAndOpacity(FSlateColor(Car->bAcceleratorBroken?CXUIStyle::Gold:CXUIStyle::Mint));
    }
    if(S)
    {
        PhaseText->SetText(FText::FromString(S->MatchPhase==ECXMatchPhase::Waiting?TEXT("출발 대기"):S->MatchPhase==ECXMatchPhase::Playing?TEXT("채굴 진행 중"):TEXT("경기 종료")));
        ClockText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"),S->RemainingSeconds/60,S->RemainingSeconds%60)));
        bool Finished=S->MatchPhase==ECXMatchPhase::Finished;ResultPanel->SetVisibility(Finished?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        if(Finished)
        {
            const bool Won=S->Winners.Contains(PS);
            ResultText->SetText(FText::FromString(Won?(S->Winners.Num()>1?TEXT("공동 우승!"):TEXT("우승!")):TEXT("아쉽지만 패배했습니다")));
        }
        NetworkPanel->SetVisibility(PC->GetNetMode()==NM_Standalone?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    }
    NoticeRemaining=FMath::Max(0.f,NoticeRemaining-Dt);
    NoticeText->SetVisibility(NoticeRemaining>0?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
}
void UCXGameWidget::FocusChatInput(){if(Chat)Chat->FocusInput();}
FReply UCXGameWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    // 채팅 입력창이나 접속 IP 칸에 포커스가 있어도 Esc를 일관되게 처리한다.
    if(Event.GetKey()==EKeys::Escape)
    {
        if(!Event.IsRepeat())if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->ToggleQuitConfirmation();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry,Event);
}
void UCXGameWidget::AddChatMessage(const FString& S,const FString& M){if(Chat)Chat->OnNewChatMessage(S,M);}
void UCXGameWidget::ShowNotice(const FString& M){NoticeText->SetText(FText::FromString(M));NoticeRemaining=2.5f;}
void UCXGameWidget::HostGame(){UGameplayStatics::OpenLevel(this,TEXT("CXMiningArena"),true,TEXT("listen"));}
void UCXGameWidget::JoinGame()
{
    FString Address=IPInput->GetText().ToString().TrimStartAndEnd();
    // 메뉴가 임의 콘솔 명령·맵 옵션의 입력 통로가 되지 않도록 단순 IP/호스트:포트만 허용한다.
    if(Address.IsEmpty() || Address.Len()>100)return;
    for(TCHAR Ch:Address)if(!FChar::IsAlnum(Ch) && Ch!='.' && Ch!=':' && Ch!='-')return;
    GetOwningPlayer()->ClientTravel(Address,TRAVEL_Absolute);
}
void UCXGameWidget::Rematch(){if(auto* PC=GetOwningPlayer<ACXPlayerController>())PC->ServerRequestRematch();}
