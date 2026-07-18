#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "HunterDamagePopupPresentationComponent.generated.h"

class APlayerController;
class UCombatManager;
class UHunterDamagePopupWidget;
class UWidgetComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogDamagePopupPresentation, Log, All);

UCLASS(ClassGroup = (ProjectHunter), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UHunterDamagePopupPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHunterDamagePopupPresentationComponent();

	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Popup")
	TSubclassOf<UHunterDamagePopupWidget> PopupWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|Legacy",
		meta = (DeprecatedProperty, DeprecationMessage = "World-space damage popups no longer use viewport Z order."))
	int32 ViewportZOrder = 25;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup")
	FVector AdditionalWorldOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	FVector2D WorldDrawSize = FVector2D(160.f, 80.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	FVector WorldWidgetScale = FVector(0.15f, 0.15f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	bool bDrawAtDesiredSize = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|Legacy",
		meta = (DeprecatedProperty, DeprecationMessage = "Damage popups now stay at their spawn world location instead of attaching to the damaged actor."))
	bool bAttachToDamagedActor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	bool bFaceLocalCameraOnSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	bool bContinuouslyFaceLocalCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|World")
	bool bTwoSidedWorldWidget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup", meta = (ClampMin = "0.0"))
	float AutoRemoveDelay = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup")
	FVector2D ViewportAlignment = FVector2D(0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup")
	bool bRequireLocallyControlledOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup")
	bool bFallbackToFirstLocalPlayerController = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Popup|Debug")
	bool bLogSpawnFailures = false;

	UFUNCTION(BlueprintCallable, Category = "Damage Popup")
	void HandleDamagePopupRequested(const FCombatDamagePopupData& PopupData);

	UFUNCTION(BlueprintCallable, Category = "Damage Popup")
	UHunterDamagePopupWidget* SpawnDamagePopup(const FCombatDamagePopupData& PopupData);

protected:
	void BindToCombatManager();
	void UnbindFromCombatManager();
	APlayerController* ResolvePlayerController() const;
	FRotator ResolveWorldWidgetRotation(const FVector& WorldLocation, const APlayerController* PlayerController) const;
	void UpdateWorldWidgetFacing(UWidgetComponent* WidgetComponent, const APlayerController* PlayerController) const;
	void UpdateActiveWorldWidgetFacing();

	UPROPERTY(Transient)
	TObjectPtr<UCombatManager> BoundCombatManager = nullptr;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UWidgetComponent>> ActiveWorldWidgetComponents;
};
