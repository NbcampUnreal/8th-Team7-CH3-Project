#include "Ping/PDPingInputComponent.h"
#include "Input/PDInputComponent.h"
#include "Input/PDInputConfig.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Ping/PDPingSubsystem.h"
#include "Ping/PDPingTypes.h"
#include "Ping/PDPingWheelBase.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Ping/PDPingMarker.h"

UPDPingInputComponent::UPDPingInputComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPDPingInputComponent::BindInputs(UPDInputComponent* PDIC, const UPDInputConfig* InputConfig)
{
    if (!PDIC || !InputConfig) return;
    
    if (PingMarkerClass)
    {
        if (UPDPingSubsystem* Sub = GetPingSubsystem())
        {
            Sub->DefaultMarkerClass = PingMarkerClass;
        }
    }

    PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Ping,
        ETriggerEvent::Started, this, &UPDPingInputComponent::OnPingStarted);
    PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_Ping,
        ETriggerEvent::Completed, this, &UPDPingInputComponent::OnPingCompleted);

    PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_PingConfirm,
        ETriggerEvent::Started, this, &UPDPingInputComponent::OnPingConfirmStarted);
    PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_PingConfirm,
        ETriggerEvent::Triggered, this, &UPDPingInputComponent::OnPingConfirmTriggered);
    PDIC->BindNativeAction(InputConfig, PDGameplayTags::Input_PingConfirm,
        ETriggerEvent::Completed, this, &UPDPingInputComponent::OnPingConfirmCompleted);
}


void UPDPingInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (PingWheelInstance)
    {
        PingWheelInstance->RemoveFromParent();
        PingWheelInstance = nullptr;
    }
    if (UPDPingSubsystem* Sub = GetPingSubsystem())
    {
        Sub->SetGPressed(false);
        Sub->SetWheelOpen(false);
    }
    bIsGPressed = false;
    Super::EndPlay(EndPlayReason);
}

APlayerController* UPDPingInputComponent::GetOwningPC() const
{
    return Cast<APlayerController>(GetOwner());
}

UPDPingSubsystem* UPDPingInputComponent::GetPingSubsystem() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UPDPingSubsystem>() : nullptr;
}

bool UPDPingInputComponent::GetMouseScreenPos(FVector2D& OutPos) const
{
    OutPos = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
    return true;
}

bool UPDPingInputComponent::GetCursorImpactPoint(FVector& OutLocation) const
{
    APlayerController* PC = GetOwningPC();
    if (!PC) return false;

    FHitResult Hit;
    if (!PC->GetHitResultUnderCursorByChannel(
            UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
    {
        return false;
    }
    OutLocation = Hit.ImpactPoint;
    return true;
}

//IA_Ping(G)
void UPDPingInputComponent::OnPingStarted()
{
    bIsGPressed = true;
    if (UPDPingSubsystem* Sub = GetPingSubsystem())
    {
        Sub->SetGPressed(true);
    }
}

void UPDPingInputComponent::OnPingCompleted()
{
    bIsGPressed = false;
    if (UPDPingSubsystem* Sub = GetPingSubsystem())
    {
        Sub->SetGPressed(false);
    }
}

//IA_PingConfirm (LMB)
void UPDPingInputComponent::OnPingConfirmStarted()
{
    if (!bIsGPressed) return;

    GetCursorImpactPoint(PingTargetLocation);
    GetMouseScreenPos(PingWheelScreenCenter);

    if (UWorld* World = GetWorld())
    {
        ClickStartTime = World->GetTimeSeconds();
    }
}

void UPDPingInputComponent::OnPingConfirmTriggered()
{
    if (!bIsGPressed) return;

    if (IsValid(PingWheelInstance))
    {
        FVector2D Mouse;
        GetMouseScreenPos(Mouse);
        PingWheelInstance->UpdateSelection(Mouse - PingWheelScreenCenter);
        return;
    }

    UWorld* World = GetWorld();
    if (!World || !PingWheelClass) return;

    const double Elapsed = World->GetTimeSeconds() - ClickStartTime;
    if (Elapsed <= WheelActivationDelay) return;

    APlayerController* PC = GetOwningPC();
    if (!PC) return;

    PingWheelInstance = CreateWidget<UPDPingWheelBase>(PC, PingWheelClass);
    if (!PingWheelInstance) return;

    PingWheelInstance->AddToViewport();
    PingWheelInstance->SetWheelScreenPosition(PingWheelScreenCenter);

    FVector2D Mouse;
    GetMouseScreenPos(Mouse);
    PingWheelInstance->UpdateSelection(Mouse - PingWheelScreenCenter);

    if (UPDPingSubsystem* Sub = GetPingSubsystem())
    {
        Sub->SetWheelOpen(true);
    }
}

void UPDPingInputComponent::OnPingConfirmCompleted()
{
    UPDPingSubsystem* Sub = GetPingSubsystem();

    if (IsValid(PingWheelInstance))
    {
        const EPDPingType Selected = PingWheelInstance->GetSelectedPingType();
        if (Sub) Sub->SpawnPing(Selected, PingTargetLocation);

        PingWheelInstance->RemoveFromParent();
        PingWheelInstance = nullptr;
        if (Sub) Sub->SetWheelOpen(false);
    }
    else if (bIsGPressed)
    {
        //짧은 탭 => Default 핑
        if (Sub) Sub->SpawnPing(EPDPingType::Default, PingTargetLocation);
    }
}