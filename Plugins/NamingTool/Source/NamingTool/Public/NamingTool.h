// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Math/IntVector.h"

class FNamingToolModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Hook for asset import event
	void OnAssetPostImport(class UFactory* Factory, class UObject* ImportedObject);

	// Popup for asset naming and folder selection
	void ShowAssetNamingPopup(const TArray<class UObject*>& ImportedAssets);

	// --- ADDED MEMBERS FOR BATCHING ---
	TArray<UObject*> PendingImportedAssets;
	FTimerHandle ImportBatchTimer;
	void HandleImportBatch();
};
