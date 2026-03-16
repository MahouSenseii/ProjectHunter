#include "Modules/ModuleManager.h"

#include "PropertyEditorModule.h"
#include "Data/BaseStatsDataCustomization.h"

class FALS_ProjectHunterEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyEditorModule.RegisterCustomClassLayout(
			TEXT("BaseStatsData"),
			FOnGetDetailCustomizationInstance::CreateStatic(&FBaseStatsDataCustomization::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
			PropertyEditorModule.UnregisterCustomClassLayout(TEXT("BaseStatsData"));
			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}
};

IMPLEMENT_MODULE(FALS_ProjectHunterEditorModule, ALS_ProjectHunterEditor)
