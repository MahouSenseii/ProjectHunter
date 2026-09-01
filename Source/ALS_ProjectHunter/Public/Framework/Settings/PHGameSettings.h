// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PHGameSettings.generated.h"

/**
 * Project Hunter's own player settings: audio and gameplay.
 *
 * Deliberately NOT a UGameUserSettings subclass. The engine builds its settings
 * object during UEngine::Init, before Default-phase game modules are loaded, so
 * pointing GameUserSettingsClass at a class in this module silently falls back
 * to plain UGameUserSettings - the symptom is every project setting reading as
 * unavailable. Moving the module to an earlier loading phase would fix that but
 * changes load order for everything else, which is not worth it for a settings
 * object.
 *
 * So ownership splits cleanly instead: UGameUserSettings owns display and
 * scalability, this owns audio and gameplay. Both persist to GameUserSettings.ini
 * through the Config specifier, so there is still one settings file.
 *
 * Audio note: master volume is applied as the audio device's transient primary
 * volume, which needs no Sound Class or Mix - the project has none yet. When a
 * real mix exists, ApplyAudioSettings is the single place to change.
 */
UCLASS(Config = GameUserSettings, BlueprintType)
class ALS_PROJECTHUNTER_API UPHGameSettings : public UObject
{
	GENERATED_BODY()

public:
	/** The settings singleton, loaded from config on first access. */
	UFUNCTION(BlueprintPure, Category = "Settings", meta = (DisplayName = "Get Project Hunter Settings"))
	static UPHGameSettings* Get();

	/** Writes to GameUserSettings.ini and pushes audio at the device. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void Save();

	/** Project defaults. Does not save until Save() is called. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void RestoreDefaults();

	/** Pushes the current volume at the audio device. */
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void ApplyAudioSettings();

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewVolume);

	// GAMEPLAY
	//
	// Only settings with a real consumer live here. A toggle nothing reads is
	// worse than a missing one, because it looks like it works.

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool GetShowDamageNumbers() const { return bShowDamageNumbers; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetShowDamageNumbers(bool bShow) { bShowDamageNumbers = bShow; }

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool GetShowFloorBanner() const { return bShowFloorBanner; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetShowFloorBanner(bool bShow) { bShowFloorBanner = bShow; }

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool GetShowEnemyHealthBars() const { return bShowEnemyHealthBars; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void SetShowEnemyHealthBars(bool bShow) { bShowEnemyHealthBars = bShow; }

private:
	/** 0 silences, 1 is unattenuated. */
	UPROPERTY(Config, BlueprintReadOnly, Category = "Settings|Audio",
		meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess = "true"))
	float MasterVolume = 1.0f;

	UPROPERTY(Config, BlueprintReadOnly, Category = "Settings|Gameplay",
		meta = (AllowPrivateAccess = "true"))
	bool bShowDamageNumbers = true;

	UPROPERTY(Config, BlueprintReadOnly, Category = "Settings|Gameplay",
		meta = (AllowPrivateAccess = "true"))
	bool bShowFloorBanner = true;

	UPROPERTY(Config, BlueprintReadOnly, Category = "Settings|Gameplay",
		meta = (AllowPrivateAccess = "true"))
	bool bShowEnemyHealthBars = true;
};
