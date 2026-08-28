// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "PHCharacterPreviewWidget.generated.h"

class UPHMenuCameraComponent;

/**
 * The hole in the menu the character is seen through.
 *
 * Draws nothing but its own framing: the menu camera has already swung the
 * real character into this part of the screen, so anything opaque here would
 * simply hide them. What the widget contributes is input - dragging inside it
 * spins the character and the wheel zooms, which is also the only reliable way
 * to get the wheel at all, since Slate consumes it before the player controller
 * ever sees it in GameAndUI mode.
 *
 * Reparent WBP_CharacterPreview to this and give it a transparent hit area.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UPHCharacterPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPHCharacterPreviewWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Character Preview")
	bool IsDragging() const { return bDragging; }

	/** The HUD's menu camera, or null when the menu camera is not in play. */
	UFUNCTION(BlueprintPure, Category = "Character Preview")
	UPHMenuCameraComponent* GetMenuCamera() const;

protected:
	virtual void NativeConstruct() override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	/** Hook the turntable grab up to a cursor change or a brighter frame. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Character Preview|Events")
	void OnPreviewDragStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Character Preview|Events")
	void OnPreviewDragFinished();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input")
	bool bDragToRotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input")
	FKey DragMouseButton = EKeys::LeftMouseButton;

	/** Negative flips the drag direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input")
	float DragSensitivity = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input")
	bool bWheelToZoom = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input",
		meta = (ClampMin = "0.0"))
	float WheelZoomStep = 0.12f;

	/** Double-click puts the character back to the facing they opened on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Preview|Input")
	bool bResetTurntableOnDoubleClick = true;

private:
	void SetDragging(bool bNewDragging);

	bool bDragging = false;
};
