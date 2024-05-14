// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Character/Player/PHPlayerCharacter.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FL_InteractUtility.generated.h"

/**
 *
 */

class UBaseItem;
class UInteractableManager;
class AALSBaseCharacter;

UENUM(BlueprintType)
enum class EGamepadIcon : uint8
{
	GI_None UMETA(DisplayName = "None"),
	GI_FaceButtonBottom UMETA(DisplayName = "Face Button Bottom"),
	GI_FaceButtonLeft UMETA(DisplayName = "Face Button Left"),
	GI_FaceButtonRight UMETA(DisplayName = "Face Button Righ"),
	GI_FaceButtonTop UMETA(DisplayName = "Face Button Top"),
	GI_LeftThumbstick UMETA(DisplayName = "Left Thumbstick"),
	GI_RightThumbstick UMETA(DisplayName = "Right Thumbstick"),
	GI_RightThumbstickButton UMETA(DisplayName = "Right Thumbstick Button"),
	GI_LeftThumbstickButton UMETA(DisplayName = "Left Thumbstick Button"),
	GI_LeftShoulder UMETA(DisplayName = "Left Shoulder"),
	GI_RightShoulder UMETA(DisplayName = "Right Shoulder"),
	GI_LeftTrigger UMETA(DisplayName = "Left Trigger"),
	GI_RightTrigger UMETA(DisplayName = "Right Trigger"),
	GI_DpadUp UMETA(DisplayName = "Dpad Up"),
	GI_DpadDown UMETA(DisplayName = "Dpad Down"),
	GI_DpadRight UMETA(DisplayName = "Dpad Right"),
	GI_DpadLeft UMETA(DisplayName = "Dpad Left"),
	GI_SpecialLeft UMETA(DisplayName = "Special Left"),
	GI_SpecialRight UMETA(DisplayName = "Special Right"),

};

UENUM(BlueprintType)
enum class EKeyboardIcon : uint8
{
	None UMETA(DisplayName = "None"),

	Key_A UMETA(DisplayName = "A"),
	Key_B UMETA(DisplayName = "B"),
	Key_C UMETA(DisplayName = "C"),
	Key_D UMETA(DisplayName = "D"),
	Key_E UMETA(DisplayName = "E"),
	Key_F UMETA(DisplayName = "F"),
	Key_G UMETA(DisplayName = "G"),
	Key_H UMETA(DisplayName = "H"),
	Key_I UMETA(DisplayName = "I"),
	Key_J UMETA(DisplayName = "J"),
	Key_K UMETA(DisplayName = "K"),
	Key_L UMETA(DisplayName = "L"),
	Key_M UMETA(DisplayName = "M"),
	Key_N UMETA(DisplayName = "N"),
	Key_O UMETA(DisplayName = "O"),
	Key_P UMETA(DisplayName = "P"),
	Key_Q UMETA(DisplayName = "Q"),
	Key_R UMETA(DisplayName = "R"),
	Key_S UMETA(DisplayName = "S"),
	Key_T UMETA(DisplayName = "T"),
	Key_U UMETA(DisplayName = "U"),
	Key_V UMETA(DisplayName = "V"),
	Key_W UMETA(DisplayName = "W"),
	Key_X UMETA(DisplayName = "X"),
	Key_Y UMETA(DisplayName = "Y"),
	Key_Z UMETA(DisplayName = "Z"),

	// Add as many keys as you want

	Key_Space UMETA(DisplayName = "Space"),
	Key_Enter UMETA(DisplayName = "Enter"),
	Key_Tab UMETA(DisplayName = "Tab"),

	Key_Up UMETA(DisplayName = "Arrow Up"),
	Key_Down UMETA(DisplayName = "Arrow Down"),
	Key_Left UMETA(DisplayName = "Arrow Left"),
	Key_Right UMETA(DisplayName = "Arrow Right"),

	Key_Shift UMETA(DisplayName = "Shift"),
	Key_Ctrl UMETA(DisplayName = "Ctrl"),
	Key_Alt UMETA(DisplayName = "Alt")
};

UCLASS()
class ALS_PROJECTHUNTER_API UFL_InteractUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintGetter, Category = "Getter")
	static UTexture2D* GetGamepadIcon(EGamepadIcon Input);


	UFUNCTION(BlueprintGetter, Category = "Getter")
	static UTexture2D* GetKeyboardIcon(EKeyboardIcon Input);

	UFUNCTION(BlueprintGetter)
	static UInteractableManager* GetCurrentInteractableObject(const APHPlayerCharacter* OwningPlayer);
	static EGamepadIcon TranslateToGamepadIcon(const FKey& Key);
	static EKeyboardIcon TranslateToKeyboardIcon(const FKey& Key);
};





