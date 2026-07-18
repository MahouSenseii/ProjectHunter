// Item/Library/Structs/ItemAttachmentStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Item/Library/Enums/ItemEnums.h"
#include "ItemAttachmentStructs.generated.h"

USTRUCT(BlueprintType)
struct FItemAttachmentRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule LocationRule = EPHAttachmentRule::AR_SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule RotationRule = EPHAttachmentRule::AR_SnapToTarget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	EPHAttachmentRule ScaleRule = EPHAttachmentRule::AR_KeepRelative;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	bool bWeldSimulatedBodies = false;

	FItemAttachmentRules() = default;
};
