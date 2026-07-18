#include "Loot/Library/FunctionLibraries/LootCorruptionFunctionLibrary.h"

FText ULootCorruptionFunctionLibrary::GetCorruptionTypeName(const ECorruptionType Type)
{
	switch (Type)
	{
	case ECorruptionType::CT_None: return FText::FromString(TEXT("None"));
	case ECorruptionType::CT_Minor: return FText::FromString(TEXT("Minor Corruption"));
	case ECorruptionType::CT_Major: return FText::FromString(TEXT("Major Corruption"));
	case ECorruptionType::CT_Abyssal: return FText::FromString(TEXT("Abyssal Corruption"));
	default: return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor ULootCorruptionFunctionLibrary::GetCorruptionTypeColor(const ECorruptionType Type)
{
	switch (Type)
	{
	case ECorruptionType::CT_None: return FLinearColor::White;
	case ECorruptionType::CT_Minor: return FLinearColor(0.6f, 0.3f, 0.6f);
	case ECorruptionType::CT_Major: return FLinearColor(0.4f, 0.0f, 0.4f);
	case ECorruptionType::CT_Abyssal: return FLinearColor(0.1f, 0.0f, 0.1f);
	default: return FLinearColor::White;
	}
}

float ULootCorruptionFunctionLibrary::GetCorruptionSeverity(const ECorruptionType Type)
{
	switch (Type)
	{
	case ECorruptionType::CT_None: return 0.0f;
	case ECorruptionType::CT_Minor: return 0.25f;
	case ECorruptionType::CT_Major: return 0.5f;
	case ECorruptionType::CT_Abyssal: return 1.0f;
	default: return 0.0f;
	}
}
