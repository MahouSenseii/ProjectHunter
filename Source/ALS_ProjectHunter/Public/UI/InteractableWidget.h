// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Character/ALSPlayerController.h"
#include "Components/InteractableManager.h"
#include "Library/InteractionEnumLibrary.h"
#include "InteractableWidget.generated.h"

class UFL_InteractUtility;
enum class EItemRarity : uint8;
class UImage;
class UTextBlock;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UInteractableWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;
	UFUNCTION() void  BindEventDispatchers();
	UFUNCTION() void UnbindEventDispatchers();
	virtual bool Initialize() override;

	void InitializePlayerControllerAndBindings();
	AALSPlayerController* GetALSPlayerController() const;
	void BeginDestroy();
	UFUNCTION() void InitializeMaterials();
	UFUNCTION() void InitializeDynamicMaterial(UMaterialInterface* MaterialToSet, UMaterialInstanceDynamic*& DynamicMaterialRef);
	UFUNCTION() void InitializeUIStates();
	UFUNCTION() void HandleGamepadStateChange();
	UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintGetter) FText GetInteractionText();
	UFUNCTION(BlueprintCallable) void SetFillDecimalValue(float Value);
	UFUNCTION() static void SetMaterialDecimal(UMaterialInstanceDynamic* DynamicMaterial, float DecimalValue);
	UFUNCTION(BlueprintCallable) void SetAppropriateFillingBackground();
	UFUNCTION(BlueprintCallable) bool IsUsingGamepad();
	UFUNCTION(BlueprintCallable) FLinearColor GetImageBackgroundColorandOpacity() const;
	UFUNCTION(BlueprintCallable) void EventBorderFill(float Value);
	UFUNCTION(BlueprintSetter) void SetGrade(EItemRarity InGrade) { Grade = InGrade; }
	UFUNCTION(BlueprintGetter, Category = "Icon") FSlateBrush GetInteractionIcon();
	UFUNCTION()void SetKeyImageSize() const;
	UPROPERTY()UFL_InteractUtility* MyUtilityInstance;




	// Variables 
public:

	UPROPERTY(BlueprintReadWrite, Category = "Components")
	UWidgetComponent* InteractionWidget;

	UPROPERTY(BlueprintReadWrite, Category = "Components")
	UInteractableManager* InteractableManager;

	UPROPERTY(BlueprintReadWrite, Category = "Animations")
	UWidgetAnimation* FillAnimOpacity;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UTextBlock* InteractionDescription;

	UPROPERTY(BlueprintReadOnly, EditInstanceOnly, meta = (ExposeOnSpawn))
	FText Text;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EInteractType InputType = EInteractType::Single;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Img_Background;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* Img_FillBorder;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget))
	UImage* Img_Key;

protected:

	EItemRarity Grade;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material Fill")
	UMaterialInterface* M_SquareFill;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material Fill")
	UMaterialInterface* M_CircularFill;

	UPROPERTY(BlueprintReadWrite, Category = "Materials")
	UMaterialInstanceDynamic* DynamicSquareMaterial;

	UPROPERTY(BlueprintReadWrite, Category = "Materials")
	UMaterialInstanceDynamic* DynamicCircularMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeF;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeD;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeC;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeB;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeA;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeS;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeUnkown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GradeColors")
	FLinearColor Color_GradeCorrupted;



};
