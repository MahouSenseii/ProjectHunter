// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PHDeadStateProbe.generated.h"

/**
 * Counts OnDeadStateChanged broadcasts so a test can assert on them without a live character.
 * A dynamic delegate needs a UObject to bind to, which is why this exists at all.
 */
UCLASS()
class UPHDeadStateProbe : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void OnDeadStateChanged(bool bDead)
	{
		++Calls;
		LastValue = bDead;
	}

	int32 Calls = 0;
	bool LastValue = false;
};
