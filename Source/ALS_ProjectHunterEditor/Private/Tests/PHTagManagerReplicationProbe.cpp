#include "Tests/PHTagManagerReplicationProbe.h"

#include "AbilitySystemComponent.h"
#include "Tags/Components/TagManager.h"

APHTagManagerReplicationProbe::APHTagManagerReplicationProbe()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(30.0f);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	TagManager = CreateDefaultSubobject<UTagManager>(TEXT("TagManager"));
}

void APHTagManagerReplicationProbe::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	TagManager->Initialize(AbilitySystemComponent);
}
