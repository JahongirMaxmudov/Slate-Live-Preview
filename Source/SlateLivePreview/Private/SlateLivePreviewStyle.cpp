// Copyright (c) 2026 JMPingvin. All Rights Reserved.

#include "SlateLivePreviewStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FSlateLivePreviewStyle::StyleInstance = nullptr;

void FSlateLivePreviewStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSlateLivePreviewStyle::Shutdown()
{
	if (StyleInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
		ensure(StyleInstance.IsUnique());
		StyleInstance.Reset();
	}
}

const ISlateStyle& FSlateLivePreviewStyle::Get()
{
	if (!StyleInstance.IsValid())
	{
		Initialize();
	}
	return *StyleInstance;
}

FName FSlateLivePreviewStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("SlateLivePreviewStyle"));
	return StyleSetName;
}

const FSlateBrush* FSlateLivePreviewStyle::GetBrush(FName PropertyName, const ANSICHAR* Specifier)
{
	return Get().GetBrush(PropertyName, Specifier);
}

#define IMAGE_BRUSH(RelativePath, ...) new FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

TSharedRef<FSlateStyleSet> FSlateLivePreviewStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SlateLivePreview"));
	if (Plugin.IsValid())
	{
		Style->SetContentRoot(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));
	}
	else
	{
		Style->SetContentRoot(FPaths::ProjectPluginsDir() / TEXT("SlateLivePreview/Resources"));
	}

	const FVector2D Icon14x14(14.0f, 14.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon24x24(24.0f, 24.0f);

	// --- Toolbar Icons (14x14 / 16x16) ---
	Style->Set("SlateLivePreview.NewClass", IMAGE_BRUSH(TEXT("Icons/NewClass"), Icon14x14));
	Style->Set("SlateLivePreview.QuickOpen", IMAGE_BRUSH(TEXT("Icons/QuickOpen"), Icon14x14));
	Style->Set("SlateLivePreview.Save", IMAGE_BRUSH(TEXT("Icons/Save"), Icon14x14));
	Style->Set("SlateLivePreview.SaveAll", IMAGE_BRUSH(TEXT("Icons/SaveAll"), Icon14x14));
	Style->Set("SlateLivePreview.LiveCoding", IMAGE_BRUSH(TEXT("Icons/LiveCoding"), Icon14x14));
	Style->Set("SlateLivePreview.Revert", IMAGE_BRUSH(TEXT("Icons/Revert"), Icon14x14));
	Style->Set("SlateLivePreview.SplitView", IMAGE_BRUSH(TEXT("Icons/SplitView"), Icon14x14));
	Style->Set("SlateLivePreview.SlatePreview", IMAGE_BRUSH(TEXT("Icons/SlatePreview"), Icon14x14));
	Style->Set("SlateLivePreview.AIAssistant", IMAGE_BRUSH(TEXT("Icons/AIAssistant"), Icon14x14));

	// --- Actions & Context Menus (16x16) ---
	Style->Set("SlateLivePreview.QuickFix", IMAGE_BRUSH(TEXT("Icons/QuickFix"), Icon16x16));
	Style->Set("SlateLivePreview.SwitchHeader", IMAGE_BRUSH(TEXT("Icons/SwitchHeader"), Icon16x16));
	Style->Set("SlateLivePreview.Find", IMAGE_BRUSH(TEXT("Icons/Find"), Icon16x16));
	Style->Set("SlateLivePreview.Rename", IMAGE_BRUSH(TEXT("Icons/Rename"), Icon16x16));
	Style->Set("SlateLivePreview.Delete", IMAGE_BRUSH(TEXT("Icons/Delete"), Icon16x16));
	Style->Set("SlateLivePreview.ShowInExplorer", IMAGE_BRUSH(TEXT("Icons/ShowInExplorer"), Icon16x16));
	Style->Set("SlateLivePreview.CopyPath", IMAGE_BRUSH(TEXT("Icons/CopyPath"), Icon16x16));
	Style->Set("SlateLivePreview.CloseTab", IMAGE_BRUSH(TEXT("Icons/CloseTab"), Icon14x14));
	Style->Set("SlateLivePreview.Undo", IMAGE_BRUSH(TEXT("Icons/Undo"), Icon14x14));
	Style->Set("SlateLivePreview.Redo", IMAGE_BRUSH(TEXT("Icons/Redo"), Icon14x14));
	Style->Set("SlateLivePreview.ArrowUp", IMAGE_BRUSH(TEXT("Icons/ArrowUp"), Icon14x14));
	Style->Set("SlateLivePreview.ArrowDown", IMAGE_BRUSH(TEXT("Icons/ArrowDown"), Icon14x14));
	Style->Set("SlateLivePreview.ArrowLeft", IMAGE_BRUSH(TEXT("Icons/ArrowLeft"), Icon14x14));
	Style->Set("SlateLivePreview.ArrowRight", IMAGE_BRUSH(TEXT("Icons/ArrowRight"), Icon14x14));
	Style->Set("SlateLivePreview.Cut", IMAGE_BRUSH(TEXT("Icons/Cut"), Icon16x16));
	Style->Set("SlateLivePreview.Paste", IMAGE_BRUSH(TEXT("Icons/Paste"), Icon16x16));
	Style->Set("SlateLivePreview.Comment", IMAGE_BRUSH(TEXT("Icons/Comment"), Icon16x16));
	Style->Set("SlateLivePreview.Duplicate", IMAGE_BRUSH(TEXT("Icons/Duplicate"), Icon16x16));
	Style->Set("SlateLivePreview.Refresh", IMAGE_BRUSH(TEXT("Icons/Refresh"), Icon14x14));
	Style->Set("SlateLivePreview.Snapshot", IMAGE_BRUSH(TEXT("Icons/Snapshot"), Icon14x14));
	Style->Set("SlateLivePreview.Settings", IMAGE_BRUSH(TEXT("Icons/Settings"), Icon14x14));

	// --- Class Wizard Cards (24x24) ---
	Style->Set("SlateLivePreview.Class.Character", IMAGE_BRUSH(TEXT("Icons/Class_Character"), Icon24x24));
	Style->Set("SlateLivePreview.Class.Pawn", IMAGE_BRUSH(TEXT("Icons/Class_Pawn"), Icon24x24));
	Style->Set("SlateLivePreview.Class.Actor", IMAGE_BRUSH(TEXT("Icons/Class_Actor"), Icon24x24));
	Style->Set("SlateLivePreview.Class.Component", IMAGE_BRUSH(TEXT("Icons/Class_Component"), Icon24x24));
	Style->Set("SlateLivePreview.Class.SceneComponent", IMAGE_BRUSH(TEXT("Icons/Class_SceneComponent"), Icon24x24));
	Style->Set("SlateLivePreview.Class.SlateWidget", IMAGE_BRUSH(TEXT("Icons/Class_SlateWidget"), Icon24x24));
	Style->Set("SlateLivePreview.Class.UObject", IMAGE_BRUSH(TEXT("Icons/Class_UObject"), Icon24x24));
	Style->Set("SlateLivePreview.Class.UStruct", IMAGE_BRUSH(TEXT("Icons/Class_UStruct"), Icon24x24));
	Style->Set("SlateLivePreview.Class.EmptyCpp", IMAGE_BRUSH(TEXT("Icons/Class_EmptyCpp"), Icon24x24));

	// --- Status Indicators (14x14 / 16x16) ---
	Style->Set("SlateLivePreview.Status.Ready", IMAGE_BRUSH(TEXT("Icons/Status_Ready"), Icon14x14));
	Style->Set("SlateLivePreview.Status.Compiling", IMAGE_BRUSH(TEXT("Icons/Status_Compiling"), Icon14x14));
	Style->Set("SlateLivePreview.Status.Success", IMAGE_BRUSH(TEXT("Icons/Status_Success"), Icon14x14));
	Style->Set("SlateLivePreview.Status.Failed", IMAGE_BRUSH(TEXT("Icons/Status_Failed"), Icon14x14));
	Style->Set("SlateLivePreview.Status.ErrorJump", IMAGE_BRUSH(TEXT("Icons/Status_ErrorJump"), Icon14x14));

	// --- Tree Explorer (16x16) ---
	Style->Set("SlateLivePreview.Tree.FolderClosed", IMAGE_BRUSH(TEXT("Icons/Tree_FolderClosed"), Icon16x16));
	Style->Set("SlateLivePreview.Tree.FolderOpen", IMAGE_BRUSH(TEXT("Icons/Tree_FolderOpen"), Icon16x16));
	Style->Set("SlateLivePreview.Tree.HeaderFile", IMAGE_BRUSH(TEXT("Icons/Tree_HeaderFile"), Icon16x16));
	Style->Set("SlateLivePreview.Tree.SourceFile", IMAGE_BRUSH(TEXT("Icons/Tree_SourceFile"), Icon16x16));
	Style->Set("SlateLivePreview.Tree.BuildFile", IMAGE_BRUSH(TEXT("Icons/Tree_BuildFile"), Icon16x16));

	return Style;
}

#undef IMAGE_BRUSH
