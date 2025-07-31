// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "Library/PHItemEnumLibrary.h"
#include "UI/Widgets/PHUserWidget.h"
#include "Components/TextBlock.h"
#include "DamagePopup.generated.h"

/**
 * @class UDamagePopup
 * @brief A widget class for displaying damage popups in the game.
 *
 * This class is derived from UPHUserWidget and is designed to handle and display
 * damage values for visual feedback in the game. It allows customization of the
 * damage amount and supports integration with animations to enhance the user experience.
 *
 * @note Ensure that the damage amount and animations are handled properly in the blueprint
 *       for seamless functionality.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UDamagePopup : public UPHUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable)
	void SetDamageData(int32 InAmount, EDamageTypes InType, bool bIsCrit);
	static FLinearColor GetColorForDamageType(EDamageTypes InType);

	UPROPERTY(Transient, EditAnywhere, meta=(BindWidgetAnim))
	UWidgetAnimation* PopupAnimation;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePopup")
	int32 DamageAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePopup")
	EDamageTypes DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DamagePopup")
	bool bIsCritical;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DamagePopup")
	UTexture2D* CritIcon;

	UPROPERTY(meta=(BindWidget))
	class UTextBlock* DamageText;

	UPROPERTY(meta = (BindWidget))
	UImage* CritImage;
};
