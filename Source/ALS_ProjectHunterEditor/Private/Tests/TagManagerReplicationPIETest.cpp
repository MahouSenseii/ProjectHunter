#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "LevelEditor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tags/Components/TagManager.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/PHTagManagerReplicationProbe.h"
#include "UnrealEdGlobals.h"

namespace PHTagManagerReplicationPIETest
{
	constexpr double WorldTimeoutSeconds = 30.0;
	constexpr double ReplicationTimeoutSeconds = 15.0;

	enum class EStage : uint8
	{
		StartPIE,
		WaitForWorlds,
		WaitForClientActor,
		WaitForTagAdd,
		WaitForTagRemove,
		WaitForPIEEnd
	};

	class FRunNetworkPIECommand final : public IAutomationLatentCommand
	{
	public:
		explicit FRunNetworkPIECommand(FAutomationTestBase* InTest)
			: Test(InTest)
		{
		}

		virtual bool Update() override
		{
			switch (Stage)
			{
			case EStage::StartPIE:
				return StartPIE();
			case EStage::WaitForWorlds:
				return WaitForWorlds();
			case EStage::WaitForClientActor:
				return WaitForClientActor();
			case EStage::WaitForTagAdd:
				return WaitForTagAdd();
			case EStage::WaitForTagRemove:
				return WaitForTagRemove();
			case EStage::WaitForPIEEnd:
				return WaitForPIEEnd();
			default:
				return true;
			}
		}

	private:
		bool StartPIE()
		{
			if (!GUnrealEd || !GEngine)
			{
				Test->AddError(TEXT("The editor engine is unavailable; network PIE cannot start."));
				return true;
			}

			if (HasPIEWorld())
			{
				Test->AddError(TEXT("A PIE session is already running; the network replication test requires an isolated session."));
				return true;
			}

			FAutomationEditorCommonUtils::CreateNewMap();

			PlaySettings.Reset(NewObject<ULevelEditorPlaySettings>(GetTransientPackage()));
			PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);
			PlaySettings->SetPlayNumberOfClients(2);
			PlaySettings->SetRunUnderOneProcess(true);
			PlaySettings->bLaunchSeparateServer = false;
			PlaySettings->GameGetsMouseControl = false;

			FLevelEditorModule& LevelEditorModule =
				FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

			FRequestPlaySessionParams SessionParams;
			SessionParams.WorldType = EPlaySessionWorldType::PlayInEditor;
			SessionParams.DestinationSlateViewport = LevelEditorModule.GetFirstActiveViewport();
			SessionParams.EditorPlaySettings = PlaySettings.Get();
			SessionParams.GameModeOverride = AGameModeBase::StaticClass();

			GUnrealEd->RequestPlaySession(SessionParams);
			GUnrealEd->StartQueuedPlaySessionRequest();
			BeginWait(EStage::WaitForWorlds, WorldTimeoutSeconds);
			return false;
		}

		bool WaitForWorlds()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (Context.WorldType != EWorldType::PIE || !IsValid(World) || !World->GetNetDriver())
				{
					continue;
				}

				if (World->GetNetMode() == NM_ListenServer)
				{
					ServerWorld = World;
				}
				else if (World->GetNetMode() == NM_Client)
				{
					ClientWorld = World;
				}
			}

			if (ServerWorld.IsValid() && ClientWorld.IsValid())
			{
				Test->TestEqual(TEXT("PIE server uses listen-server net mode"), ServerWorld->GetNetMode(), NM_ListenServer);
				Test->TestEqual(TEXT("PIE client uses client net mode"), ClientWorld->GetNetMode(), NM_Client);

				FActorSpawnParameters SpawnParameters;
				SpawnParameters.Name = TEXT("PH_TagManagerReplicationProbe");
				SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				ServerProbe = ServerWorld->SpawnActor<APHTagManagerReplicationProbe>(
					APHTagManagerReplicationProbe::StaticClass(),
					FTransform::Identity,
					SpawnParameters);

				if (!ServerProbe.IsValid())
				{
					return FailAndEnd(TEXT("The replicated TagManager probe could not be spawned on the PIE server."));
				}

				ServerProbe->ForceNetUpdate();
				BeginWait(EStage::WaitForClientActor, ReplicationTimeoutSeconds);
				return false;
			}

			return ContinueOrTimeout(TEXT("Timed out waiting for both listen-server and client PIE worlds."));
		}

		bool WaitForClientActor()
		{
			if (ClientWorld.IsValid())
			{
				for (TActorIterator<APHTagManagerReplicationProbe> It(ClientWorld.Get()); It; ++It)
				{
					ClientProbe = *It;
					break;
				}
			}

			if (ClientProbe.IsValid())
			{
				UTagManager* ServerTagManager = ServerProbe.IsValid() ? ServerProbe->GetTagManager() : nullptr;
				if (!ServerTagManager || !ServerTagManager->IsInitialized())
				{
					return FailAndEnd(TEXT("The server TagManager probe was not initialized with its AbilitySystemComponent."));
				}

				ReplicatedTag = FGameplayTag::RequestGameplayTag(TEXT("Condition.State.TakingDamage"));
				ServerTagManager->AddTag(ReplicatedTag);
				ServerProbe->ForceNetUpdate();
				BeginWait(EStage::WaitForTagAdd, ReplicationTimeoutSeconds);
				return false;
			}

			return ContinueOrTimeout(TEXT("Timed out waiting for the server-spawned TagManager probe to replicate to the client."));
		}

		bool WaitForTagAdd()
		{
			UAbilitySystemComponent* ClientASC = ClientProbe.IsValid()
				? ClientProbe->GetAbilitySystemComponent()
				: nullptr;
			if (ClientASC && ClientASC->HasMatchingGameplayTag(ReplicatedTag))
			{
				Test->TestTrue(TEXT("Server-managed condition tag reaches the client ASC"), true);

				UTagManager* ServerTagManager = ServerProbe.IsValid() ? ServerProbe->GetTagManager() : nullptr;
				if (!ServerTagManager)
				{
					return FailAndEnd(TEXT("The server TagManager probe disappeared before the removal phase."));
				}

				ServerTagManager->RemoveTag(ReplicatedTag);
				ServerProbe->ForceNetUpdate();
				BeginWait(EStage::WaitForTagRemove, ReplicationTimeoutSeconds);
				return false;
			}

			return ContinueOrTimeout(TEXT("Timed out waiting for a TagOnly condition add to reach the client ASC."));
		}

		bool WaitForTagRemove()
		{
			UAbilitySystemComponent* ClientASC = ClientProbe.IsValid()
				? ClientProbe->GetAbilitySystemComponent()
				: nullptr;
			if (ClientASC && !ClientASC->HasMatchingGameplayTag(ReplicatedTag))
			{
				Test->TestFalse(TEXT("Removing the server-managed condition clears it on the client ASC"), false);
				RequestPIEEnd();
				BeginWait(EStage::WaitForPIEEnd, WorldTimeoutSeconds);
				return false;
			}

			return ContinueOrTimeout(TEXT("Timed out waiting for a TagOnly condition removal to reach the client ASC."));
		}

		bool WaitForPIEEnd()
		{
			if (!HasPIEWorld())
			{
				return true;
			}

			if (HasTimedOut())
			{
				Test->AddError(TEXT("Timed out while shutting down the network PIE session."));
				return true;
			}

			return false;
		}

		bool ContinueOrTimeout(const TCHAR* TimeoutMessage)
		{
			return HasTimedOut() ? FailAndEnd(TimeoutMessage) : false;
		}

		bool FailAndEnd(const TCHAR* Message)
		{
			Test->AddError(Message);
			RequestPIEEnd();
			BeginWait(EStage::WaitForPIEEnd, WorldTimeoutSeconds);
			return false;
		}

		void BeginWait(const EStage NewStage, const double TimeoutSeconds)
		{
			Stage = NewStage;
			Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
		}

		bool HasTimedOut() const
		{
			return FPlatformTime::Seconds() >= Deadline;
		}

		static bool HasPIEWorld()
		{
			if (!GEngine)
			{
				return false;
			}

			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE && IsValid(Context.World()))
				{
					return true;
				}
			}

			return false;
		}

		static void RequestPIEEnd()
		{
			if (GUnrealEd)
			{
				GUnrealEd->RequestEndPlayMap();
			}
		}

		FAutomationTestBase* Test = nullptr;
		EStage Stage = EStage::StartPIE;
		double Deadline = 0.0;
		TStrongObjectPtr<ULevelEditorPlaySettings> PlaySettings;
		TWeakObjectPtr<UWorld> ServerWorld;
		TWeakObjectPtr<UWorld> ClientWorld;
		TWeakObjectPtr<APHTagManagerReplicationProbe> ServerProbe;
		TWeakObjectPtr<APHTagManagerReplicationProbe> ClientProbe;
		FGameplayTag ReplicatedTag;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHTagManagerReplicationPIETest,
	"ProjectHunter.Tags.Manager.ReplicatesManagedConditionTagsInPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPHTagManagerReplicationPIETest::RunTest(const FString&)
{
	// The CommonUI viewport-client notice this used to suppress is an incidental
	// engine log: CommonUI is not a project plugin and the only reference to it in
	// this repository was the suppression itself. It is emitted at Verbose from
	// LogUIActionRouter, so it can never fail a test, and requiring it made this
	// case fail the moment PIE package load order shifted - which is exactly what
	// TASK-PH-20260831-37's content reorganization did, with nothing else broken.
	// Pin assertions to this project's behaviour, not to an unrelated plugin's log.
	ADD_LATENT_AUTOMATION_COMMAND(PHTagManagerReplicationPIETest::FRunNetworkPIECommand(this));
	return true;
}

#endif
