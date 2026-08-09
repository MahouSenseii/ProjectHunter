#include "Combat/Components/HunterDamagePopupPresentationComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "UI/HUD/HunterDamagePopupWidget.h"
#include "Combat/Components/CombatManager.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogDamagePopupPresentation);

UHunterDamagePopupPresentationComponent::UHunterDamagePopupPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
	SetAutoActivate(true);
}

void UHunterDamagePopupPresentationComponent::BeginPlay()
{
	Super::BeginPlay();
	BindToCombatManager();
}

void UHunterDamagePopupPresentationComponent::OnRegister()
{
	Super::OnRegister();
	BindToCombatManager();
}

void UHunterDamagePopupPresentationComponent::OnUnregister()
{
	UnbindFromCombatManager();
	ActiveWorldWidgetComponents.Reset();
	SetComponentTickEnabled(false);
	Super::OnUnregister();
}

void UHunterDamagePopupPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromCombatManager();
	ActiveWorldWidgetComponents.Reset();
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

void UHunterDamagePopupPresentationComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateActiveWorldWidgetFacing();
}

void UHunterDamagePopupPresentationComponent::BindToCombatManager()
{
	AActor* Owner = GetOwner();
	UCombatManager* CombatManager = Owner ? Owner->FindComponentByClass<UCombatManager>() : nullptr;
	if (!CombatManager)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup presentation found no CombatManager on %s."),
				*GetNameSafe(Owner));
		}
		else
		{
			UE_LOG(LogDamagePopupPresentation, Verbose,
				TEXT("Damage popup presentation found no CombatManager on %s."),
				*GetNameSafe(Owner));
		}
		return;
	}

	if (BoundCombatManager && BoundCombatManager != CombatManager)
	{
		UnbindFromCombatManager();
	}

	BoundCombatManager = CombatManager;
	BoundCombatManager->OnDamagePopupRequested.RemoveAll(this);
	BoundCombatManager->OnDamagePopupRequested.AddDynamic(
		this, &UHunterDamagePopupPresentationComponent::HandleDamagePopupRequested);
}

void UHunterDamagePopupPresentationComponent::UnbindFromCombatManager()
{
	if (BoundCombatManager)
	{
		BoundCombatManager->OnDamagePopupRequested.RemoveAll(this);
		BoundCombatManager = nullptr;
	}
}

void UHunterDamagePopupPresentationComponent::HandleDamagePopupRequested(const FCombatDamagePopupData& PopupData)
{
	if (bLogSpawnFailures)
	{
		UE_LOG(LogDamagePopupPresentation, Log,
			TEXT("Damage popup request received. Owner=%s Target=%s TotalDamage=%.2f"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(PopupData.TargetActor),
			PopupData.TotalDamage);
	}

	if (UHunterDamagePopupWidget* SpawnedWidget = SpawnDamagePopup(PopupData))
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Log,
				TEXT("Damage popup spawned. Owner=%s Widget=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(SpawnedWidget));
		}
	}
}

UHunterDamagePopupWidget* UHunterDamagePopupPresentationComponent::SpawnDamagePopup(
	const FCombatDamagePopupData& PopupData)
{
	if (!PopupWidgetClass || PopupData.TotalDamage <= KINDA_SMALL_NUMBER)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup skipped on %s. WidgetClass=%s TotalDamage=%.2f"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(PopupWidgetClass),
				PopupData.TotalDamage);
		}
		return nullptr;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup skipped on %s because no local PlayerController was resolved. RequireLocalOwner=%s FallbackFirstLocal=%s"),
				*GetNameSafe(GetOwner()),
				bRequireLocallyControlledOwner ? TEXT("true") : TEXT("false"),
				bFallbackToFirstLocalPlayerController ? TEXT("true") : TEXT("false"));
		}
		return nullptr;
	}

	FCombatDamagePopupData AdjustedPopupData = PopupData;
	AdjustedPopupData.WorldLocation += AdditionalWorldOffset;

	AActor* DamagedActor = AdjustedPopupData.TargetActor.Get();
	AActor* ComponentOwner = IsValid(DamagedActor) ? DamagedActor : GetOwner();
	if (!ComponentOwner)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup skipped because no component owner was available. Owner=%s Target=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(DamagedActor));
		}
		return nullptr;
	}

	UWidgetComponent* WidgetComponent = NewObject<UWidgetComponent>(ComponentOwner);
	if (!WidgetComponent)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup skipped because UWidgetComponent creation failed. Owner=%s ComponentOwner=%s WidgetClass=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ComponentOwner),
				*GetNameSafe(PopupWidgetClass));
		}
		return nullptr;
	}

	ComponentOwner->AddInstanceComponent(WidgetComponent);
	WidgetComponent->SetWidgetClass(PopupWidgetClass);
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	WidgetComponent->SetTwoSided(bTwoSidedWorldWidget);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetGenerateOverlapEvents(false);
	WidgetComponent->SetWindowFocusable(false);
	WidgetComponent->SetPivot(ViewportAlignment);
	WidgetComponent->SetDrawAtDesiredSize(bDrawAtDesiredSize);
	WidgetComponent->SetDrawSize(FIntPoint(
		FMath::Max(FMath::RoundToInt(WorldDrawSize.X), 1),
		FMath::Max(FMath::RoundToInt(WorldDrawSize.Y), 1)));
	WidgetComponent->SetWorldLocation(AdjustedPopupData.WorldLocation);
	if (bFaceLocalCameraOnSpawn || bContinuouslyFaceLocalCamera)
	{
		UpdateWorldWidgetFacing(WidgetComponent, PlayerController);
	}
	WidgetComponent->SetWorldScale3D(WorldWidgetScale);
	WidgetComponent->RegisterComponent();

	WidgetComponent->InitWidget();

	UHunterDamagePopupWidget* Widget = Cast<UHunterDamagePopupWidget>(WidgetComponent->GetUserWidgetObject());
	if (!Widget)
	{
		if (bLogSpawnFailures)
		{
			UE_LOG(LogDamagePopupPresentation, Warning,
				TEXT("Damage popup skipped because WidgetComponent did not create a HunterDamagePopupWidget. Owner=%s ComponentOwner=%s WidgetClass=%s"),
				*GetNameSafe(GetOwner()),
				*GetNameSafe(ComponentOwner),
				*GetNameSafe(PopupWidgetClass));
		}

		WidgetComponent->DestroyComponent();
		return nullptr;
	}

	Widget->SetOwningWorldWidgetComponent(WidgetComponent);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	Widget->InitializeDamagePopup(AdjustedPopupData);
	WidgetComponent->RequestRedraw();
	ActiveWorldWidgetComponents.Add(WidgetComponent);
	SetComponentTickEnabled(bContinuouslyFaceLocalCamera);

	if (AutoRemoveDelay > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<UWidgetComponent> WeakWidgetComponent = WidgetComponent;
			FTimerHandle AutoRemoveTimerHandle;
			World->GetTimerManager().SetTimer(
				AutoRemoveTimerHandle,
				FTimerDelegate::CreateWeakLambda(this, [WeakWidgetComponent]()
				{
					if (UWidgetComponent* LiveWidgetComponent = WeakWidgetComponent.Get())
					{
						LiveWidgetComponent->DestroyComponent();
					}
				}),
				AutoRemoveDelay,
				false);
		}
	}

	return Widget;
}

APlayerController* UHunterDamagePopupPresentationComponent::ResolvePlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
		if (OwnerPlayerController && OwnerPlayerController->IsLocalController())
		{
			return OwnerPlayerController;
		}

		if (bRequireLocallyControlledOwner && !bFallbackToFirstLocalPlayerController)
		{
			return nullptr;
		}
	}

	if (!bFallbackToFirstLocalPlayerController)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	APlayerController* FirstPlayerController = World ? World->GetFirstPlayerController() : nullptr;
	return FirstPlayerController && FirstPlayerController->IsLocalController()
		? FirstPlayerController
		: nullptr;
}

FRotator UHunterDamagePopupPresentationComponent::ResolveWorldWidgetRotation(
	const FVector& WorldLocation,
	const APlayerController* PlayerController) const
{
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return FRotator::ZeroRotator;
	}

	const FVector CameraLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
	return (CameraLocation - WorldLocation).Rotation();
}

void UHunterDamagePopupPresentationComponent::UpdateWorldWidgetFacing(
	UWidgetComponent* WidgetComponent,
	const APlayerController* PlayerController) const
{
	if (!IsValid(WidgetComponent) || !PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	WidgetComponent->SetWorldRotation(
		ResolveWorldWidgetRotation(WidgetComponent->GetComponentLocation(), PlayerController));
}

void UHunterDamagePopupPresentationComponent::UpdateActiveWorldWidgetFacing()
{
	if (!bContinuouslyFaceLocalCamera)
	{
		SetComponentTickEnabled(false);
		return;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	for (int32 ComponentIndex = ActiveWorldWidgetComponents.Num() - 1; ComponentIndex >= 0; --ComponentIndex)
	{
		UWidgetComponent* WidgetComponent = ActiveWorldWidgetComponents[ComponentIndex].Get();
		if (!IsValid(WidgetComponent))
		{
			ActiveWorldWidgetComponents.RemoveAtSwap(ComponentIndex, 1, EAllowShrinking::No);
			continue;
		}

		UpdateWorldWidgetFacing(WidgetComponent, PlayerController);
	}

	if (ActiveWorldWidgetComponents.Num() == 0)
	{
		SetComponentTickEnabled(false);
	}
}
