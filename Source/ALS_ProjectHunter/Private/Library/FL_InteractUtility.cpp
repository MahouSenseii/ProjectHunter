// Copyright (c) 2024 Quentin Davis. All rights reserved.

#include "Library/FL_InteractUtility.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Components/InteractionManager.h"
#include "Item/WeaponItem.h"


UTexture2D* UFL_InteractUtility::GetGamepadIcon(const EGamepadIcon Input)
{
	UTexture2D* InputUTexture2D = nullptr;
	switch (Input)
	{
	case EGamepadIcon::GI_None:
		return InputUTexture2D;
	case EGamepadIcon::GI_FaceButtonBottom:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_A.T_Gamepad_A")));
		return InputUTexture2D;
	case EGamepadIcon::GI_FaceButtonLeft:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_B.T_Gamepad_B")));
		return InputUTexture2D;
	case EGamepadIcon::GI_FaceButtonRight:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_X.T_Gamepad_X")));
		return InputUTexture2D;
	case EGamepadIcon::GI_FaceButtonTop:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_Y.T_Gamepad_Y")));
		return InputUTexture2D;
	case EGamepadIcon::GI_DpadLeft:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_DpadLeft.T_Gamepad_DpadLeft")));
		return InputUTexture2D;
	case EGamepadIcon::GI_DpadRight:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_DpadRight._Gamepad_DpadRight")));
		return InputUTexture2D;
	case EGamepadIcon::GI_DpadUp:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_DpadUp.T_Gamepad_DpadUp")));
		return InputUTexture2D;
	case EGamepadIcon::GI_DpadDown:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_DpadDown.T_Gamepad_DpadDown")));
		return InputUTexture2D;
	case EGamepadIcon::GI_RightTrigger:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_RT.T_Gamepad_RT")));
		return InputUTexture2D;
	case EGamepadIcon::GI_LeftTrigger:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_LT.T_Gamepad_LT")));
		return InputUTexture2D;
	case EGamepadIcon::GI_RightShoulder:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(),nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_RB.T_Gamepad_RB")));
		return InputUTexture2D;
	case EGamepadIcon::GI_LeftShoulder:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_LB.T_Gamepad_LB")));
		return InputUTexture2D;
	case EGamepadIcon::GI_SpecialRight:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_ViewButton.T_Gamepad_ViewButton")));
		return InputUTexture2D;
	case EGamepadIcon::GI_SpecialLeft:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Gamepad/T_Gamepad_MenuButton.T_Gamepad_MenuButton")));
		return InputUTexture2D;
	default:
		return nullptr;
	}
	
}

UTexture2D* UFL_InteractUtility::GetKeyboardIcon(const EKeyboardIcon Input)
{
	UTexture2D* InputUTexture2D = nullptr;
	switch (Input)
	{
	case EKeyboardIcon::None:
		return InputUTexture2D;
	case EKeyboardIcon::Key_A:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_A.T_Keyboard_A")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_B:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_B.T_Keyboard_B")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_C:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_C.T_Keyboard_C")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_D:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_D.T_Keyboard_D")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_E:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_E.T_Keyboard_E")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_F:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_F.T_Keyboard_F")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_G:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_G.T_Keyboard_G")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_H:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_H.T_Keyboard_H")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_I:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_I.T_Keyboard_I")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_J:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_J.T_Keyboard_J")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_K:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_K.T_Keyboard_K")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_L:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_L.T_Keyboard_L")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_M:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_M.T_Keyboard_M")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_N:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_N.T_Keyboard_N'")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_O:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_O.T_Keyboard_O")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_P:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_P.T_Keyboard_P")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_Q:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_Q.T_Keyboard_Q")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_R:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_R.T_Keyboard_R")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_S:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_S.T_Keyboard_S")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_T:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_T.T_Keyboard_T")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_U:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_U.T_Keyboard_U")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_V:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_V.T_Keyboard_V")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_W:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_W.T_Keyboard_W")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_X:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_X.T_Keyboard_X")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_Y:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_Y.T_Keyboard_Y")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_Z:
		InputUTexture2D = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, TEXT("/Game/Images/Buttons/Keyboard/T_Keyboard_Z.T_Keyboard_Z")));
		return InputUTexture2D;
	case EKeyboardIcon::Key_Enter:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Space:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Tab:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Up:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Down:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Left:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Right:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Shift:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Ctrl:
		return InputUTexture2D;
	case EKeyboardIcon::Key_Alt:
		return InputUTexture2D;
	default:
		return nullptr;

	}
}

bool UFL_InteractUtility::AreRequirementsMet(UBaseItem* InItem, AActor* OwnerPlayer)
{
	if (!InItem) // Check if the Item pointer is valid
	{
		return false;
	}

	// Loop through each weapon requirement
	for (const auto& Elem : InItem->GetItemInfo()->Equip.RequirementStats)
	{
		const EItemRequiredStatsCategory RequirementCategory = Elem.Key;

		// Check if the requirement is met, if not, return false immediately
		if (const float RequiredValue = Elem.Value; !CheckBasedOnCompare(RequirementCategory, RequiredValue, OwnerPlayer))
		{
			return false; // Requirement not met, return false
		}
	}

	return true; // If loop completes, all requirements are met
}

bool UFL_InteractUtility::CheckBasedOnCompare(EItemRequiredStatsCategory RequiredStats, float RequiredValue,
        AActor* OwnerPlayer)
{
        if (APHBaseCharacter* Owner = Cast<APHBaseCharacter>(OwnerPlayer))
        {
                // Attempt to access the attribute set on the owner
                const UPHAttributeSet* AttributeSet = Cast<UPHAttributeSet>(Owner->GetAttributeSet());

                switch (RequiredStats)
                {
                case EItemRequiredStatsCategory::ISC_None:
                        return true;
                case EItemRequiredStatsCategory::ISC_RequiredLevel:
                        return Owner->GetPlayerLevel() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredStrength:
                        return AttributeSet && AttributeSet->GetStrength() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredDexterity:
                        return AttributeSet && AttributeSet->GetDexterity() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredIntelligence:
                        return AttributeSet && AttributeSet->GetIntelligence() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredEndurance:
                        return AttributeSet && AttributeSet->GetEndurance() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredAffliction:
                        return AttributeSet && AttributeSet->GetAffliction() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredLuck:
                        return AttributeSet && AttributeSet->GetLuck() >= RequiredValue;
                case EItemRequiredStatsCategory::ISC_RequiredCovenant:
                        return AttributeSet && AttributeSet->GetCovenant() >= RequiredValue;
                default:
                        return false;
                }
        }

        return false;
}

void UFL_InteractUtility::GetInteractableScores(const TArray<UInteractableManager*>& Interactables,
	const FVector& PlayerLocation,const FVector& PlayerForward, const float MaxDistance, const float MinDot,
	TArray<float>& OutScores,TArray<UInteractableManager*>& OutValidElements, const bool bDrawDebug)
{
	OutScores.Empty();
	OutValidElements.Empty();

	for (UInteractableManager* Element : Interactables)
	{
		if (!Element || !Element->IsInteractable || !Element->InteractableArea) continue;

		const FVector InteractableLocation = Element->InteractableArea->GetComponentLocation();
		const FVector Direction = (InteractableLocation - PlayerLocation).GetSafeNormal();
		const float Distance = FVector::Dist(PlayerLocation, InteractableLocation);
		const float Dot = FVector::DotProduct(Direction, PlayerForward);

		if (Distance > MaxDistance || Dot < MinDot) continue;

		const float DotWeight = 0.9f;
		const float DistanceWeight = 0.1f;

		float DistanceNormalized = 1.0f - FMath::Clamp(Distance / MaxDistance, 0.f, 1.f);
		float Score = Dot * DotWeight + DistanceNormalized * DistanceWeight;

		OutScores.Add(Score);
		OutValidElements.Add(Element);

		if (bDrawDebug)
		{
			FColor LineColor = FColor::Blue;
			DrawDebugLine(Element->GetWorld(), PlayerLocation, InteractableLocation, LineColor, false, 2.0f, 0, 2.f);
			DrawDebugString(Element->GetWorld(), InteractableLocation + FVector(0,0,50),
				FString::Printf(TEXT("Score: %.2f"), Score), nullptr, FColor::White, 2.0f);
		}
	}
}


UInteractableManager* UFL_InteractUtility::GetCurrentInteractableObject(const APHBaseCharacter* OwningPlayer)
{
	if (UInteractionManager* ComponentClass = OwningPlayer->FindComponentByClass<UInteractionManager>();
		IsValid(ComponentClass))
	{
		if (ComponentClass->GetCurrentInteractable())
		{
			return ComponentClass->GetCurrentInteractable();
		}
	}
	else
	{
		return nullptr;
	}
	return nullptr;
}

EGamepadIcon UFL_InteractUtility::TranslateToGamepadIcon(const FKey& Key)
{
    if (Key == EKeys::Gamepad_FaceButton_Bottom) // 'A' on Xbox, 'X' on PlayStation
    {
        return EGamepadIcon::GI_FaceButtonBottom;
    }
    else if (Key == EKeys::Gamepad_FaceButton_Right) // 'B' on Xbox, 'Circle' on PlayStation
    {
        return EGamepadIcon::GI_FaceButtonRight;
    }
    else if (Key == EKeys::Gamepad_FaceButton_Left) // 'X' on Xbox, 'Square' on PlayStation
    {
        return EGamepadIcon::GI_FaceButtonLeft;
    }
    else if (Key == EKeys::Gamepad_FaceButton_Top) // 'Y' on Xbox, 'Triangle' on PlayStation
    {
        return EGamepadIcon::GI_FaceButtonTop;
    }
    else if (Key == EKeys::Gamepad_DPad_Up)
    {
        return EGamepadIcon::GI_DpadUp;
    }
    else if (Key == EKeys::Gamepad_DPad_Down)
    {
        return EGamepadIcon::GI_DpadDown;
    }
    else if (Key == EKeys::Gamepad_DPad_Left)
    {
        return EGamepadIcon::GI_DpadLeft;
    }
    else if (Key == EKeys::Gamepad_DPad_Right)
    {
        return EGamepadIcon::GI_DpadRight;
    }
    else if (Key == EKeys::Gamepad_LeftShoulder)
    {
        return EGamepadIcon::GI_LeftShoulder;
    }
    else if (Key == EKeys::Gamepad_RightShoulder)
    {
        return EGamepadIcon::GI_RightShoulder;
    }
    else if (Key == EKeys::Gamepad_LeftTrigger)
    {
        return EGamepadIcon::GI_LeftTrigger;
    }
    else if (Key == EKeys::Gamepad_RightTrigger)
    {
        return EGamepadIcon::GI_RightTrigger;
    }
    else if (Key == EKeys::Gamepad_Special_Left) // Often the 'View' button on Xbox, 'Share' on PlayStation
    {
        return EGamepadIcon::GI_SpecialLeft;
    }
    else if (Key == EKeys::Gamepad_Special_Right) // 'Menu' on Xbox, 'Options' on PlayStation
    {
        return EGamepadIcon::GI_SpecialRight;
    }
    else if (Key == EKeys::Gamepad_LeftThumbstick)
    {
        return EGamepadIcon::GI_LeftThumbstick;
    }
    else if (Key == EKeys::Gamepad_RightThumbstick)
    {
        return EGamepadIcon::GI_RightThumbstick;
    }
    else
    {
        return EGamepadIcon::GI_None; // No valid mapping found
    }
}


EKeyboardIcon UFL_InteractUtility::TranslateToKeyboardIcon(const FKey& Key)
{
    if (Key == EKeys::A)
    {
        return EKeyboardIcon::Key_A;
    }
    else if (Key == EKeys::B)
    {
        return EKeyboardIcon::Key_B;
    }
    else if (Key == EKeys::C)
    {
        return EKeyboardIcon::Key_C;
    }
    else if (Key == EKeys::D)
    {
        return EKeyboardIcon::Key_D;
    }
    else if (Key == EKeys::E)
    {
        return EKeyboardIcon::Key_E;
    }
    else if (Key == EKeys::F)
    {
        return EKeyboardIcon::Key_F;
    }
    else if (Key == EKeys::G)
    {
        return EKeyboardIcon::Key_G;
    }
    else if (Key == EKeys::H)
    {
        return EKeyboardIcon::Key_H;
    }
    else if (Key == EKeys::I)
    {
        return EKeyboardIcon::Key_I;
    }
    else if (Key == EKeys::J)
    {
        return EKeyboardIcon::Key_J;
    }
    else if (Key == EKeys::K)
    {
        return EKeyboardIcon::Key_K;
    }
    else if (Key == EKeys::L)
    {
        return EKeyboardIcon::Key_L;
    }
    else if (Key == EKeys::M)
    {
        return EKeyboardIcon::Key_M;
    }
    else if (Key == EKeys::N)
    {
        return EKeyboardIcon::Key_N;
    }
    else if (Key == EKeys::O)
    {
        return EKeyboardIcon::Key_O;
    }
    else if (Key == EKeys::P)
    {
        return EKeyboardIcon::Key_P;
    }
    else if (Key == EKeys::Q)
    {
        return EKeyboardIcon::Key_Q;
    }
    else if (Key == EKeys::R)
    {
        return EKeyboardIcon::Key_R;
    }
    else if (Key == EKeys::S)
    {
        return EKeyboardIcon::Key_S;
    }
    else if (Key == EKeys::T)
    {
        return EKeyboardIcon::Key_T;
    }
    else if (Key == EKeys::U)
    {
        return EKeyboardIcon::Key_U;
    }
    else if (Key == EKeys::V)
    {
        return EKeyboardIcon::Key_V;
    }
    else if (Key == EKeys::W)
    {
        return EKeyboardIcon::Key_W;
    }
    else if (Key == EKeys::X)
    {
        return EKeyboardIcon::Key_X;
    }
    else if (Key == EKeys::Y)
    {
        return EKeyboardIcon::Key_Y;
    }
    else if (Key == EKeys::Z)
    {
        return EKeyboardIcon::Key_Z;
    }
    // Add mappings for other keys you have icons for, such as numbers or function keys
    else
    {
        return EKeyboardIcon::None; // No valid mapping found
    }
}




