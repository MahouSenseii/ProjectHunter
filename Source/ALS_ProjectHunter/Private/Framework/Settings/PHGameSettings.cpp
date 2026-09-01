// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Framework/Settings/PHGameSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"

UPHGameSettings* UPHGameSettings::Get()
{
	// Rooted rather than held by a subsystem: settings outlive any world, and
	// the menu can be built before one exists.
	static TStrongObjectPtr<UPHGameSettings> Instance;
	if (!Instance.IsValid())
	{
		UPHGameSettings* Created = NewObject<UPHGameSettings>(
			GetTransientPackage(), UPHGameSettings::StaticClass(), TEXT("PHGameSettings"));
		Created->LoadConfig();
		Created->ApplyAudioSettings();
		Instance.Reset(Created);
	}
	return Instance.Get();
}

void UPHGameSettings::SetMasterVolume(const float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
}

void UPHGameSettings::RestoreDefaults()
{
	MasterVolume = 1.0f;
	bShowDamageNumbers = true;
	bShowFloorBanner = true;
	bShowEnemyHealthBars = true;
}

void UPHGameSettings::ApplyAudioSettings()
{
	if (!GEngine)
	{
		return;
	}

	// Transient primary volume scales everything the device plays and needs no
	// Sound Class or Mix. It is not itself persisted - MasterVolume is the saved
	// value, and this reapplies it on load and on every change.
	// Non-const handle: SetTransientPrimaryVolume is a non-const member.
	if (FAudioDeviceHandle DeviceHandle = GEngine->GetMainAudioDevice())
	{
		DeviceHandle->SetTransientPrimaryVolume(MasterVolume);
	}
}

void UPHGameSettings::Save()
{
	SaveConfig();
	ApplyAudioSettings();
}
