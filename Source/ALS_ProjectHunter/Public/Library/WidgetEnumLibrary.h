// Copyright@2024 Quentin Davis 

#pragma once

#include "WidgetEnumLibrary.generated.h"


UENUM(BlueprintType)
enum class EWidgets : uint8
{
	AW_None UMETA(DisplayName = "None"),
	AW_Settings UMETA(DisplayName = "Settings"),
	AW_Stats UMETA(DisplayName = "Stats"),
	AW_Map UMETA(DisplayName = "Map"),
	AW_Titles UMETA(DisplayName = "Titles"),
	AW_Craft UMETA(DisplayName = "Craft"),
	AW_Gallery UMETA(DisplayName = "Gallery"),
	AW_Equipment UMETA(DisplayName = "Equipment"),
	AW_Passive UMETA(DisplayName = "Passive"),
	AW_Vendor UMETA(DisplayName = "Vendor"),
	AW_LoadGame UMETA(DisplayName = "Load Game"),
	AW_VendorEquipment UMETA(DisplayName = "Inventory"),
	AW_Storage UMETA(Displayname = "Storage"),
	AW_StorageEquipment UMETA(Displayname = "Storage Equipment "),
	// Add more Widgets as needed 
};


UENUM(BlueprintType)
enum class EWidgetsSwitcher : uint8
{
	WS_None UMETA(DisplayName = "None"),

	WS_MainTab UMETA(DisplayName = "Main"),
	WS_VendorTab UMETA(DisplayName = "Vendor"),
	WS_StorageTab UMETA(DisplayName = "Storage"),
	WS_LoadTab UMETA(DisplayName = "Load"),
};