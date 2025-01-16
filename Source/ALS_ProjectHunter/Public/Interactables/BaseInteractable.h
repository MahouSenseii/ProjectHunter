// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableObjectInterface.h"
#include "BaseInteractable.generated.h"

class APHBaseCharacter;
class UInteractableManager;
class UWidgetComponent;


UCLASS(Blueprintable)
class ALS_PROJECTHUNTER_API ABaseInteractable : public AActor, public IInteractableObjectInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseInteractable();
	void InitializeComponent();
	virtual void OnConstruction(const FTransform& Transform) override;
	
	void CheckForActors();

	virtual bool InteractionHandle(AActor* Actor, bool WasHeld) const;
	virtual void BPIInitialize_Implementation() override;
	virtual void BPIClientStartInteraction_Implementation(AActor* Interactor, bool bIsHeld) override;
	virtual void BPIClientEndInteraction_Implementation(AActor* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Action")
	void BPStartOverride(AActor* Interactor, bool Held);
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Action")
	void BPEndInteractionOverride(AActor* Interactor);

	UFUNCTION(BlueprintCallable)
	void SetNewMesh(UStaticMesh* InMesh) { NewMesh = InMesh; }

	UFUNCTION(BlueprintCallable)
	void SetSkeletalMesh(USkeletalMesh* InMesh) { SkeletalMesh->SetSkinnedAssetAndUpdate(InMesh); }
	

	UInteractableManager* GetInteractableManager(){return InteractableManager;}
	
	// Initialization Functions
	void SetupMeshVisibility() const;
	void SetupOverlapEvents();
	void SetupTrigger() const;
	void SetupUI() const;
	void SetupMesh() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	

	// Overlap Event Handlers
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	
	// Validation Functions
	void ValidateComponents() const;
	
protected: // Property Members
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	TObjectPtr<UInteractableManager> InteractableManager;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	FVector UILocation {0.0f, 0.0f, 140.0f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	FTransform MeshTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trigger")
	float TriggerRadius{ 300.0f };

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Trigger")
	FVector TriggerLocation {0.0f, 0.0f, 0.0f};

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMesh> NewMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractableArea;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> Scene;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	UWidgetComponent* InteractionWidget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	TSet<UPrimitiveComponent*> MeshSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	TObjectPtr<APHBaseCharacter> InteractorOwner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Mesh")
	bool bShouldStaticMeshBeSet = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Interaction")
	bool bIsInteractable = true;

	FTimerHandle TimerHandle_DelayedStart;

};
