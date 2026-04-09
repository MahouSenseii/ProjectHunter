#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"

#define PH_LOG_SOURCE_FILE *FPaths::GetCleanFilename(ANSI_TO_TCHAR(__FILE__))

#define PH_LOG(Category, Verbosity, Format, ...) \
	UE_LOG(Category, Verbosity, TEXT("[%s | %s:%d] " Format), PH_LOG_SOURCE_FILE, TEXT(__FUNCTION__), __LINE__, ##__VA_ARGS__)

#define PH_LOG_WARNING(Category, Format, ...) \
	PH_LOG(Category, Warning, Format, ##__VA_ARGS__)

#define PH_LOG_ERROR(Category, Format, ...) \
	PH_LOG(Category, Error, Format, ##__VA_ARGS__)
