// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"

class FSlateLivePreviewModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FSlateLivePreviewModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FSlateLivePreviewModule>("SlateLivePreview");
	}

	static const FName TabName;
	static const FName CppStudioTabName;

	/** Open or focus the live preview tab */
	void OpenLivePreviewTab();

	/** Open or focus the In-Engine C++ Studio tab */
	void OpenCppStudioTab();

private:
	TSharedRef<SDockTab> SpawnLivePreviewTab(const FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<SDockTab> SpawnCppStudioTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
};
