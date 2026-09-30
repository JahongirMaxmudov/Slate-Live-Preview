// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SlateLivePreviewModule.h"
#include "SSlateLivePreviewTab.h"
#include "SCppStudioTab.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#include "SlateLivePreviewStyle.h"

#define LOCTEXT_NAMESPACE "SlateLivePreviewModule"

const FName FSlateLivePreviewModule::TabName(TEXT("SlateLivePreviewTab"));
const FName FSlateLivePreviewModule::CppStudioTabName(TEXT("CppStudioTab"));

void FSlateLivePreviewModule::StartupModule()
{
	// 0. Initialize Custom Slate Style Set
	FSlateLivePreviewStyle::Initialize();

	// 1. Register Slate Live Preview Tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		TabName,
		FOnSpawnTab::CreateRaw(this, &FSlateLivePreviewModule::SpawnLivePreviewTab)
	)
	.SetDisplayName(LOCTEXT("SlateLivePreviewTabTitle", "Slate Live Preview"))
	.SetTooltipText(LOCTEXT("SlateLivePreviewTabTooltip", "Open Live Slate UI Preview Window."))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
	.SetIcon(FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), "SlateLivePreview.SlatePreview"));

	// 2. Register In-Engine C++ Studio Tab
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		CppStudioTabName,
		FOnSpawnTab::CreateRaw(this, &FSlateLivePreviewModule::SpawnCppStudioTab)
	)
	.SetDisplayName(LOCTEXT("CppStudioTabTitle", "In-Engine C++ Studio"))
	.SetTooltipText(LOCTEXT("CppStudioTabTooltip", "Lightweight in-editor C++ code editor and Live Coding interface."))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
	.SetIcon(FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), "SlateLivePreview.SplitView"));

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSlateLivePreviewModule::RegisterMenus));

	UE_LOG(LogTemp, Log, TEXT("SlateLivePreview & CppStudio module loaded successfully."));
}

void FSlateLivePreviewModule::ShutdownModule()
{
	if (UToolMenus::TryGet() != nullptr)
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(CppStudioTabName);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);

	FSlateLivePreviewStyle::Shutdown();

	UE_LOG(LogTemp, Log, TEXT("SlateLivePreview & CppStudio module shut down."));
}

TSharedRef<SDockTab> FSlateLivePreviewModule::SpawnLivePreviewTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SSlateLivePreviewTab)
		];
}

TSharedRef<SDockTab> FSlateLivePreviewModule::SpawnCppStudioTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SCppStudioTab)
		];
}

void FSlateLivePreviewModule::OpenLivePreviewTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

void FSlateLivePreviewModule::OpenCppStudioTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(CppStudioTabName);
}

void FSlateLivePreviewModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
	FToolMenuSection& Section = ToolsMenu->FindOrAddSection(TEXT("DeveloperTools"), LOCTEXT("DevToolsSection", "Developer Tools"));

	// 1. C++ Studio
	Section.AddMenuEntry(
		TEXT("CppStudio"),
		LOCTEXT("CppStudioEntry", "In-Engine C++ Studio"),
		LOCTEXT("CppStudioEntryTooltip", "Lightweight in-editor C++ code editor with Live Coding hot-reloading."),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), "SlateLivePreview.SplitView"),
		FUIAction(FExecuteAction::CreateRaw(this, &FSlateLivePreviewModule::OpenCppStudioTab))
	);

	// 2. Slate Live Preview
	Section.AddMenuEntry(
		TEXT("SlateLivePreview"),
		LOCTEXT("SlateLivePreviewEntry", "Slate Live Preview"),
		LOCTEXT("SlateLivePreviewEntryTooltip", "Instant live Slate UI previewer with zero-compilation AST interpreter."),
		FSlateIcon(FSlateLivePreviewStyle::GetStyleSetName(), "SlateLivePreview.SlatePreview"),
		FUIAction(FExecuteAction::CreateRaw(this, &FSlateLivePreviewModule::OpenLivePreviewTab))
	);
}

IMPLEMENT_MODULE(FSlateLivePreviewModule, SlateLivePreview)

#undef LOCTEXT_NAMESPACE
