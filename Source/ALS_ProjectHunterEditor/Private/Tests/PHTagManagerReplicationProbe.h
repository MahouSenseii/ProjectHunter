#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PHTagManagerReplicationProbe.generated.h"

class UAbilitySystemComponent;
class UTagManager;

/** Minimal replicated owner used only by the editor network automation test. */
UCLASS(NotPlaceable, Transient)
class APHTagManagerReplicationProbe : public AActor
{
	GENERATED_BODY()

public:
	APHTagManagerReplicationProbe();

	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	UTagManager* GetTagManager() const { return TagManager; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTagManager> TagManager;
};
