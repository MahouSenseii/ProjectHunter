#include "Loot/Library/FunctionLibraries/LootRarityFunctionLibrary.h"

FText ULootRarityFunctionLibrary::GetDropRarityDisplayName(const EDropRarity Rarity)
{
	switch (Rarity)
	{
	case EDropRarity::DR_Common: return FText::FromString(TEXT("Common"));
	case EDropRarity::DR_Uncommon: return FText::FromString(TEXT("Uncommon"));
	case EDropRarity::DR_Rare: return FText::FromString(TEXT("Rare"));
	case EDropRarity::DR_Epic: return FText::FromString(TEXT("Epic"));
	case EDropRarity::DR_Legendary: return FText::FromString(TEXT("Legendary"));
	case EDropRarity::DR_Mythical: return FText::FromString(TEXT("Mythical"));
	default: return FText::FromString(TEXT("Unknown"));
	}
}

FLinearColor ULootRarityFunctionLibrary::GetDropRarityColor(const EDropRarity Rarity)
{
	switch (Rarity)
	{
	case EDropRarity::DR_Common: return FLinearColor(0.8f, 0.8f, 0.8f);
	case EDropRarity::DR_Uncommon: return FLinearColor(0.2f, 0.8f, 0.2f);
	case EDropRarity::DR_Rare: return FLinearColor(0.2f, 0.4f, 1.0f);
	case EDropRarity::DR_Epic: return FLinearColor(0.6f, 0.2f, 0.9f);
	case EDropRarity::DR_Legendary: return FLinearColor(1.0f, 0.6f, 0.0f);
	case EDropRarity::DR_Mythical: return FLinearColor(1.0f, 0.2f, 0.2f);
	default: return FLinearColor::White;
	}
}

EItemRarity ULootRarityFunctionLibrary::DropRarityToItemRarity(const EDropRarity DropRarity)
{
	switch (DropRarity)
	{
	case EDropRarity::DR_Common: return EItemRarity::IR_GradeF;
	case EDropRarity::DR_Uncommon: return EItemRarity::IR_GradeE;
	case EDropRarity::DR_Rare: return EItemRarity::IR_GradeD;
	case EDropRarity::DR_Epic: return EItemRarity::IR_GradeC;
	case EDropRarity::DR_Legendary: return EItemRarity::IR_GradeB;
	case EDropRarity::DR_Mythical: return EItemRarity::IR_GradeA;
	default: return EItemRarity::IR_None;
	}
}

float ULootRarityFunctionLibrary::GetRarityMultiplier(const EDropRarity Rarity)
{
	switch (Rarity)
	{
	case EDropRarity::DR_Common: return 1.0f;
	case EDropRarity::DR_Uncommon: return 1.5f;
	case EDropRarity::DR_Rare: return 2.0f;
	case EDropRarity::DR_Epic: return 2.5f;
	case EDropRarity::DR_Legendary: return 3.5f;
	case EDropRarity::DR_Mythical: return 5.0f;
	default: return 1.0f;
	}
}
