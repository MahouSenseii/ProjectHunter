#include "Modules/ModuleManager.h"

#include "EdGraphUtilities.h"
#include "PropertyEditorModule.h"
#include "Data/BaseStatsDataCustomization.h"
#include "Item/ItemBaseCustomization.h"
#include "Passive/PHPassiveTreeGraphFactories.h"

class FALS_ProjectHunterEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		PassiveNodeFactory = MakeShared<FPHPassiveTreeNodeFactory>();
		FEdGraphUtilities::RegisterVisualNodeFactory(PassiveNodeFactory);
		PassiveConnectionFactory = MakeShared<FPHPassiveTreeConnectionFactory>();
		FEdGraphUtilities::RegisterVisualPinConnectionFactory(PassiveConnectionFactory);

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditorModule.RegisterCustomClassLayout(
			TEXT("BaseStatsData"),
			FOnGetDetailCustomizationInstance::CreateStatic(&FBaseStatsDataCustomization::MakeInstance));
		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			TEXT("ItemBase"),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FItemBaseCustomization::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (PassiveNodeFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(PassiveNodeFactory);
			PassiveNodeFactory.Reset();
		}
		if (PassiveConnectionFactory.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualPinConnectionFactory(PassiveConnectionFactory);
			PassiveConnectionFactory.Reset();
		}

		if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyEditorModule.UnregisterCustomClassLayout(TEXT("BaseStatsData"));
			PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TEXT("ItemBase"));
			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}

private:
	TSharedPtr<FPHPassiveTreeNodeFactory> PassiveNodeFactory;
	TSharedPtr<FPHPassiveTreeConnectionFactory> PassiveConnectionFactory;
};

IMPLEMENT_MODULE(FALS_ProjectHunterEditorModule, ALS_ProjectHunterEditor)
