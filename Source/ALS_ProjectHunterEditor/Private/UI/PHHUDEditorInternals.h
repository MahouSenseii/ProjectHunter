// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"

class FJsonObject;
class UHunterHUDResourceWidget;
class UTextBlock;
class UWidgetBlueprint;

namespace PHHUDEditor
{
	bool Fail(const FString& Message);
	FString EvidenceDirectory();
	UWidgetBlueprint* LoadHUD();
	bool BackupHUD(UWidgetBlueprint* Blueprint);
	bool SaveNamedAsset(UObject* Asset);
	FString JSONText(const TSharedRef<FJsonObject>& Object);
	TSharedRef<FJsonObject> ContractInventory(UWidgetBlueprint* Blueprint);
	TSharedRef<FJsonObject> ResourceSnapshot(UHunterHUDResourceWidget* Resource);
	void SetTextStyle(UTextBlock* Text, int32 Size);
}
