#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalSubsystem.generated.h"

class APortalActor;

DECLARE_LOG_CATEGORY_EXTERN(LogPortalSubsystem, Log, All);

UCLASS()
class ALS_PROJECTHUNTER_API UPortalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void RegisterPortal(FName PortalID, APortalActor* Portal);

	UFUNCTION(BlueprintCallable, Category = "Portal")
	void UnregisterPortal(FName PortalID);

	UFUNCTION(BlueprintPure, Category = "Portal")
	APortalActor* FindPortal(FName PortalID) const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	TArray<APortalActor*> GetAllPortals() const;

	UFUNCTION(BlueprintPure, Category = "Portal")
	int32 GetPortalCount() const { return PortalRegistry.Num(); }

private:
	UPROPERTY()
	TMap<FName, TWeakObjectPtr<APortalActor>> PortalRegistry;
};
