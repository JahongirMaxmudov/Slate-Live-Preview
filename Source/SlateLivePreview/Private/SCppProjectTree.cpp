// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#include "SCppProjectTree.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/MessageDialog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "SCreateClassDialog.h"

void SCppProjectTree::Construct(const FArguments& InArgs)
{
	OnFileSelected = InArgs._OnFileSelected;
	OnNewClassRequested = InArgs._OnNewClassRequested;
	OnFilesRenamed = InArgs._OnFilesRenamed;
	OnFileDeleted = InArgs._OnFileDeleted;

	ChildSlot
	[
		SNew(SVerticalBox)

		// Search, New and Refresh Bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Filter C++ files...")))
				.OnTextChanged(this, &SCppProjectTree::OnFilterTextChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(0.12f, 0.52f, 0.95f, 1.0f))
				.Text(FText::FromString(TEXT("+ New")))
				.ToolTipText(FText::FromString(TEXT("Create new C++ Class / Slate Widget (Ctrl+N)")))
				.OnClicked_Lambda([this]() -> FReply
				{
					if (OnNewClassRequested.IsBound())
					{
						OnNewClassRequested.Execute();
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.0f, 0.0f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("🔄")))
				.ToolTipText(FText::FromString(TEXT("Refresh Source Tree")))
				.OnClicked_Lambda([this]() -> FReply
				{
					RefreshTree();
					return FReply::Handled();
				})
			]
		]

		// Tree View
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SAssignNew(TreeView, STreeView<TSharedPtr<FCppSourceNode>>)
			.TreeItemsSource(&FilteredNodes)
			.OnGenerateRow(this, &SCppProjectTree::OnGenerateRow)
			.OnGetChildren(this, &SCppProjectTree::OnGetChildren)
			.OnSelectionChanged(this, &SCppProjectTree::OnSelectionChanged)
			.OnContextMenuOpening(this, &SCppProjectTree::OnContextMenuOpening)
		]
	];

	RefreshTree();
}

void SCppProjectTree::RefreshTree()
{
	RootNodes.Empty();

	// 1. Project Source Directory
	FString ProjectSourceDir = FPaths::GameSourceDir();
	if (IFileManager::Get().DirectoryExists(*ProjectSourceDir))
	{
		TSharedPtr<FCppSourceNode> SourceRoot = MakeShared<FCppSourceNode>();
		SourceRoot->Name = TEXT("Source (Project)");
		SourceRoot->FullPath = ProjectSourceDir;
		SourceRoot->bIsDirectory = true;

		ScanDirectoryRecursive(ProjectSourceDir, SourceRoot);
		RootNodes.Add(SourceRoot);
	}

	// 2. Plugins Source Directories
	FString PluginsDir = FPaths::ProjectPluginsDir();
	TArray<FString> PluginFolders;
	IFileManager::Get().FindFiles(PluginFolders, *(PluginsDir / TEXT("*")), false, true);

	for (const FString& PluginFolder : PluginFolders)
	{
		FString PluginSourceDir = PluginsDir / PluginFolder / TEXT("Source");
		if (IFileManager::Get().DirectoryExists(*PluginSourceDir))
		{
			TSharedPtr<FCppSourceNode> PluginNode = MakeShared<FCppSourceNode>();
			PluginNode->Name = FString::Printf(TEXT("Plugins/%s"), *PluginFolder);
			PluginNode->FullPath = PluginSourceDir;
			PluginNode->bIsDirectory = true;

			ScanDirectoryRecursive(PluginSourceDir, PluginNode);
			RootNodes.Add(PluginNode);
		}
	}

	ApplyFilter();

	// Expand top-level roots by default
	if (TreeView.IsValid())
	{
		for (const auto& Root : FilteredNodes)
		{
			TreeView->SetItemExpansion(Root, true);
		}
	}
}

void SCppProjectTree::ScanDirectoryRecursive(const FString& InDirPath, const TSharedPtr<FCppSourceNode>& ParentNode)
{
	IFileManager& FileManager = IFileManager::Get();

	// Find Subdirectories
	TArray<FString> SubDirs;
	FileManager.FindFiles(SubDirs, *(InDirPath / TEXT("*")), false, true);
	SubDirs.Sort();

	for (const FString& SubDir : SubDirs)
	{
		if (SubDir.Equals(TEXT("Intermediate"), ESearchCase::IgnoreCase) ||
			SubDir.Equals(TEXT("Binaries"), ESearchCase::IgnoreCase) ||
			SubDir.Equals(TEXT(".vs"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		FString FullSubDirPath = InDirPath / SubDir;
		TSharedPtr<FCppSourceNode> DirNode = MakeShared<FCppSourceNode>();
		DirNode->Name = SubDir;
		DirNode->FullPath = FullSubDirPath;
		DirNode->bIsDirectory = true;
		DirNode->Parent = ParentNode;

		ScanDirectoryRecursive(FullSubDirPath, DirNode);

		if (DirNode->Children.Num() > 0)
		{
			ParentNode->Children.Add(DirNode);
		}
	}

	// Find Source Files
	TArray<FString> Files;
	FileManager.FindFiles(Files, *(InDirPath / TEXT("*.*")), true, false);
	Files.Sort();

	for (const FString& File : Files)
	{
		FString Ext = FPaths::GetExtension(File).ToLower();
		if (Ext == TEXT("h") || Ext == TEXT("cpp") || Ext == TEXT("cs") || Ext == TEXT("inl") || Ext == TEXT("slate"))
		{
			TSharedPtr<FCppSourceNode> FileNode = MakeShared<FCppSourceNode>();
			FileNode->Name = File;
			FileNode->FullPath = InDirPath / File;
			FileNode->bIsDirectory = false;
			FileNode->Parent = ParentNode;

			ParentNode->Children.Add(FileNode);
		}
	}
}

void SCppProjectTree::ApplyFilter()
{
	FilteredNodes.Empty();

	if (CurrentFilterText.IsEmpty())
	{
		FilteredNodes = RootNodes;
	}
	else
	{
		for (const auto& Root : RootNodes)
		{
			if (Root->MatchesFilter(CurrentFilterText))
			{
				FilteredNodes.Add(Root);
			}
		}
	}

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
	}
}

void SCppProjectTree::OnFilterTextChanged(const FText& InFilterText)
{
	CurrentFilterText = InFilterText.ToString();
	ApplyFilter();
}

TSharedRef<ITableRow> SCppProjectTree::OnGenerateRow(TSharedPtr<FCppSourceNode> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	FString PrefixIcon = InItem->bIsDirectory ? TEXT("📁 ") : TEXT("📄 ");
	FLinearColor ItemColor = FLinearColor::White;

	if (InItem->bIsDirectory)
	{
		ItemColor = FLinearColor(0.9f, 0.8f, 0.4f, 1.0f);
	}
	else
	{
		FString Ext = FPaths::GetExtension(InItem->Name).ToLower();
		if (Ext == TEXT("h") || Ext == TEXT("inl"))
		{
			PrefixIcon = TEXT("🔷 ");
			ItemColor = FLinearColor(0.4f, 0.8f, 1.0f, 1.0f);
		}
		else if (Ext == TEXT("cpp"))
		{
			PrefixIcon = TEXT("⚡ ");
			ItemColor = FLinearColor(0.5f, 1.0f, 0.5f, 1.0f);
		}
		else if (Ext == TEXT("cs"))
		{
			PrefixIcon = TEXT("⚙ ");
			ItemColor = FLinearColor(0.8f, 0.6f, 1.0f, 1.0f);
		}
	}

	TSharedRef<STableRow<TSharedPtr<FCppSourceNode>>> Row = SNew(STableRow<TSharedPtr<FCppSourceNode>>, OwnerTable);

	Row->SetContent(
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(4.0f, 1.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(PrefixIcon))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(FMargin(2.0f, 1.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(InItem->Name))
			.ColorAndOpacity(ItemColor)
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
		]
	);

	return Row;
}

void SCppProjectTree::OnGetChildren(TSharedPtr<FCppSourceNode> InItem, TArray<TSharedPtr<FCppSourceNode>>& OutChildren)
{
	if (CurrentFilterText.IsEmpty())
	{
		OutChildren = InItem->Children;
	}
	else
	{
		for (const auto& Child : InItem->Children)
		{
			if (Child->MatchesFilter(CurrentFilterText))
			{
				OutChildren.Add(Child);
			}
		}
	}
}

void SCppProjectTree::OnSelectionChanged(TSharedPtr<FCppSourceNode> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid() && !SelectedItem->bIsDirectory)
	{
		if (OnFileSelected.IsBound())
		{
			OnFileSelected.Execute(SelectedItem->FullPath);
		}
	}
}

void SCppProjectTree::SelectFile(const FString& InFilePath)
{
}

void SCppProjectTree::GetAllFiles(TArray<FString>& OutFiles) const
{
	auto CollectFromNode = [&OutFiles](auto& Self, const TSharedPtr<FCppSourceNode>& Node) -> void
	{
		if (!Node.IsValid()) return;
		if (!Node->bIsDirectory)
		{
			OutFiles.Add(Node->FullPath);
		}
		for (const auto& Child : Node->Children)
		{
			Self(Self, Child);
		}
	};

	for (const auto& Root : RootNodes)
	{
		CollectFromNode(CollectFromNode, Root);
	}
}

FString SCppProjectTree::GetSelectedDirectory() const
{
	TSharedPtr<FCppSourceNode> Node = GetSelectedNode();
	if (Node.IsValid())
	{
		if (Node->bIsDirectory)
		{
			return Node->FullPath;
		}
		return FPaths::GetPath(Node->FullPath);
	}
	return FString();
}

TSharedPtr<FCppSourceNode> SCppProjectTree::GetSelectedNode() const
{
	if (TreeView.IsValid())
	{
		TArray<TSharedPtr<FCppSourceNode>> Selected = TreeView->GetSelectedItems();
		if (Selected.Num() > 0)
		{
			return Selected[0];
		}
	}
	return nullptr;
}

TSharedPtr<SWidget> SCppProjectTree::OnContextMenuOpening()
{
	TSharedPtr<FCppSourceNode> SelectedNode = GetSelectedNode();
	if (!SelectedNode.IsValid())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	if (!SelectedNode->bIsDirectory)
	{
		// ✏️ Rename File / Class...
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("✏️ Rename File / Class Refactor...")),
			FText::FromString(TEXT("Rename file, companion header/source, and update symbols")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCppProjectTree::OpenRenameDialog, SelectedNode))
		);

		// 🗑️ Delete File...
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("🗑️ Delete File...")),
			FText::FromString(TEXT("Delete file permanently from disk")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCppProjectTree::DeleteNode, SelectedNode))
		);

		MenuBuilder.AddSeparator();

		// ➕ New C++ Class Here
		FString FolderPath = FPaths::GetPath(SelectedNode->FullPath);
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("➕ New C++ Class in This Folder...")),
			FText::FromString(TEXT("Create a new class in the directory containing this file")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FolderPath, this]()
			{
				SCreateClassDialog::OpenModal(FolderPath, FOnClassCreated::CreateLambda([this](const FString&, const FString&)
				{
					RefreshTree();
				}));
			}))
		);

		MenuBuilder.AddSeparator();

		// 📂 Show in Explorer
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("📂 Show in Explorer")),
			FText::FromString(TEXT("Open containing folder in Windows Explorer")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FolderPath]()
			{
				FPlatformProcess::ExploreFolder(*FolderPath);
			}))
		);

		// 📋 Copy Full Path
		FString FullPath = SelectedNode->FullPath;
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("📋 Copy Full Path")),
			FText::FromString(TEXT("Copy absolute file path to clipboard")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FullPath]()
			{
				FPlatformApplicationMisc::ClipboardCopy(*FullPath);
			}))
		);
	}
	else
	{
		// Directory
		FString FolderPath = SelectedNode->FullPath;

		// ➕ New C++ Class / File Here...
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("➕ New C++ Class in This Folder...")),
			FText::FromString(TEXT("Create a new class in this directory")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FolderPath, this]()
			{
				SCreateClassDialog::OpenModal(FolderPath, FOnClassCreated::CreateLambda([this](const FString&, const FString&)
				{
					RefreshTree();
				}));
			}))
		);

		// ✏️ Rename Folder...
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("✏️ Rename Folder...")),
			FText::FromString(TEXT("Rename this directory")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCppProjectTree::OpenRenameDialog, SelectedNode))
		);

		// 🗑️ Delete Folder...
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("🗑️ Delete Folder...")),
			FText::FromString(TEXT("Delete directory and its contents")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SCppProjectTree::DeleteNode, SelectedNode))
		);

		MenuBuilder.AddSeparator();

		// 📂 Show in Explorer
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("📂 Show in Explorer")),
			FText::FromString(TEXT("Open directory in Windows Explorer")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FolderPath]()
			{
				FPlatformProcess::ExploreFolder(*FolderPath);
			}))
		);

		// 📋 Copy Full Path
		MenuBuilder.AddMenuEntry(
			FText::FromString(TEXT("📋 Copy Full Path")),
			FText::FromString(TEXT("Copy directory path to clipboard")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([FolderPath]()
			{
				FPlatformApplicationMisc::ClipboardCopy(*FolderPath);
			}))
		);
	}

	return MenuBuilder.MakeWidget();
}

void SCppProjectTree::OpenRenameDialog(TSharedPtr<FCppSourceNode> Node)
{
	if (!Node.IsValid()) return;

	const FString OldFullPath = Node->FullPath;
	const FString OldBaseFilename = FPaths::GetBaseFilename(OldFullPath);
	const FString OldExtension = FPaths::GetExtension(OldFullPath);
	const FString FolderPath = FPaths::GetPath(OldFullPath);
	const bool bIsSourceFile = !Node->bIsDirectory && (OldExtension.Equals(TEXT("h"), ESearchCase::IgnoreCase) || OldExtension.Equals(TEXT("cpp"), ESearchCase::IgnoreCase));

	// Detect companion file (e.g. MyClass.cpp for MyClass.h)
	FString CompanionOldPath;
	if (bIsSourceFile)
	{
		if (OldExtension.Equals(TEXT("h"), ESearchCase::IgnoreCase))
		{
			// Check same dir
			FString SameDirCpp = FolderPath / (OldBaseFilename + TEXT(".cpp"));
			if (IFileManager::Get().FileExists(*SameDirCpp))
			{
				CompanionOldPath = SameDirCpp;
			}
			else
			{
				// Check Private folder if currently in Public
				FString PrivateDir = FolderPath;
				PrivateDir.ReplaceInline(TEXT("/Public"), TEXT("/Private"));
				PrivateDir.ReplaceInline(TEXT("\\Public"), TEXT("\\Private"));
				FString PrivCpp = PrivateDir / (OldBaseFilename + TEXT(".cpp"));
				if (IFileManager::Get().FileExists(*PrivCpp))
				{
					CompanionOldPath = PrivCpp;
				}
			}
		}
		else if (OldExtension.Equals(TEXT("cpp"), ESearchCase::IgnoreCase))
		{
			// Check same dir
			FString SameDirH = FolderPath / (OldBaseFilename + TEXT(".h"));
			if (IFileManager::Get().FileExists(*SameDirH))
			{
				CompanionOldPath = SameDirH;
			}
			else
			{
				// Check Public folder if currently in Private
				FString PublicDir = FolderPath;
				PublicDir.ReplaceInline(TEXT("/Private"), TEXT("/Public"));
				PublicDir.ReplaceInline(TEXT("\\Private"), TEXT("\\Public"));
				FString PubH = PublicDir / (OldBaseFilename + TEXT(".h"));
				if (IFileManager::Get().FileExists(*PubH))
				{
					CompanionOldPath = PubH;
				}
			}
		}
	}

	TSharedPtr<SEditableTextBox> NewNameTextBox;
	TSharedPtr<SCheckBox> RenameCompanionCheckBox;
	TSharedPtr<SCheckBox> RefactorSymbolsCheckBox;

	TSharedRef<SWindow> RenameWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Rename File / Class Refactor")))
		.ClientSize(FVector2D(520.0f, 260.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		.SizingRule(ESizingRule::UserSized);

	RenameWindow->SetContent(
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.BorderBackgroundColor(FLinearColor(0.10f, 0.10f, 0.12f, 1.0f))
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 10.0f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::Printf(TEXT("✏️ Rename: %s"), *Node->Name)))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FLinearColor(0.35f, 0.75f, 1.0f, 1.0f))
			]

			// New Name Input
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("New Name:")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9.5f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(NewNameTextBox, SEditableTextBox)
					.Text(FText::FromString(Node->bIsDirectory ? Node->Name : OldBaseFilename))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9.5f))
				]
			]

			// Checkbox for Companion file
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 4.0f)
			[
				SAssignNew(RenameCompanionCheckBox, SCheckBox)
				.Visibility(!CompanionOldPath.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed)
				.IsChecked(ECheckBoxState::Checked)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("Also rename companion file (%s)"), *FPaths::GetCleanFilename(CompanionOldPath))))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
				]
			]

			// Checkbox for Refactor Symbols
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 2.0f, 0.0f, 12.0f)
			[
				SAssignNew(RefactorSymbolsCheckBox, SCheckBox)
				.Visibility(bIsSourceFile ? EVisibility::Visible : EVisibility::Collapsed)
				.IsChecked(ECheckBoxState::Checked)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Update C++ Class Name & #includes inside file content")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 8.5f))
				]
			]

			+ SVerticalBox::Slot().FillHeight(1.0f) [ SNew(SSpacer) ]

			// Buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f) [ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Cancel")))
					.OnClicked_Lambda([RenameWindow]() -> FReply
					{
						RenameWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.12f, 0.52f, 0.95f, 1.0f))
					.Text(FText::FromString(TEXT("✏️ Rename & Refactor")))
					.OnClicked_Lambda([this, RenameWindow, Node, OldFullPath, OldBaseFilename, OldExtension, FolderPath, CompanionOldPath, bIsSourceFile, NewNameTextBox, RenameCompanionCheckBox, RefactorSymbolsCheckBox]() -> FReply
					{
						FString NewNameRaw = NewNameTextBox->GetText().ToString().TrimStartAndEnd();
						if (NewNameRaw.IsEmpty() || NewNameRaw.Equals(OldBaseFilename, ESearchCase::IgnoreCase))
						{
							RenameWindow->RequestDestroyWindow();
							return FReply::Handled();
						}

						const bool bRenameCompanion = RenameCompanionCheckBox.IsValid() && RenameCompanionCheckBox->IsChecked();
						const bool bRefactorSymbols = RefactorSymbolsCheckBox.IsValid() && RefactorSymbolsCheckBox->IsChecked();

						IFileManager& FileManager = IFileManager::Get();

						if (Node->bIsDirectory)
						{
							FString NewDirPath = FPaths::GetPath(OldFullPath) / NewNameRaw;
							FileManager.Move(*NewDirPath, *OldFullPath);
							if (OnFilesRenamed.IsBound())
							{
								OnFilesRenamed.Execute(OldFullPath, NewDirPath);
							}
						}
						else
						{
							// Function to refactor and save file
							auto RefactorAndMoveFile = [&](const FString& SrcPath, const FString& DstPath)
							{
								FString Content;
								if (FFileHelper::LoadFileToString(Content, *SrcPath))
								{
									if (bRefactorSymbols)
									{
										// 1. Replace include directives
										Content.ReplaceInline(*(OldBaseFilename + TEXT(".h")), *(NewNameRaw + TEXT(".h")));
										Content.ReplaceInline(*(OldBaseFilename + TEXT(".generated.h")), *(NewNameRaw + TEXT(".generated.h")));

										// 2. Replace prefixed class names
										Content.ReplaceInline(*(TEXT("A") + OldBaseFilename), *(TEXT("A") + NewNameRaw));
										Content.ReplaceInline(*(TEXT("U") + OldBaseFilename), *(TEXT("U") + NewNameRaw));
										Content.ReplaceInline(*(TEXT("F") + OldBaseFilename), *(TEXT("F") + NewNameRaw));
										Content.ReplaceInline(*(TEXT("S") + OldBaseFilename), *(TEXT("S") + NewNameRaw));

										// 3. Replace constructor / scope qualifiers
										Content.ReplaceInline(*(OldBaseFilename + TEXT("::")), *(NewNameRaw + TEXT("::")));
									}

									FFileHelper::SaveStringToFile(Content, *DstPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
									FileManager.Delete(*SrcPath);

									if (OnFilesRenamed.IsBound())
									{
										OnFilesRenamed.Execute(SrcPath, DstPath);
									}
								}
								else
								{
									FileManager.Move(*DstPath, *SrcPath);
									if (OnFilesRenamed.IsBound())
									{
										OnFilesRenamed.Execute(SrcPath, DstPath);
									}
								}
							};

							// Rename primary file
							FString NewPrimaryPath = FolderPath / (NewNameRaw + (OldExtension.IsEmpty() ? TEXT("") : TEXT(".") + OldExtension));
							RefactorAndMoveFile(OldFullPath, NewPrimaryPath);

							// Rename companion file if requested
							if (bRenameCompanion && !CompanionOldPath.IsEmpty())
							{
								FString CompanionExt = FPaths::GetExtension(CompanionOldPath);
								FString CompanionFolder = FPaths::GetPath(CompanionOldPath);
								FString NewCompanionPath = CompanionFolder / (NewNameRaw + TEXT(".") + CompanionExt);
								RefactorAndMoveFile(CompanionOldPath, NewCompanionPath);
							}
						}

						RefreshTree();
						RenameWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		]
	);

	FSlateApplication::Get().AddModalWindow(RenameWindow, FSlateApplication::Get().GetActiveTopLevelWindow());
}

void SCppProjectTree::DeleteNode(TSharedPtr<FCppSourceNode> Node)
{
	if (!Node.IsValid()) return;

	EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::FromString(FString::Printf(TEXT("Are you sure you want to permanently delete '%s' from disk?"), *Node->Name))
	);

	if (Result == EAppReturnType::Yes)
	{
		IFileManager& FileManager = IFileManager::Get();
		if (Node->bIsDirectory)
		{
			FileManager.DeleteDirectory(*Node->FullPath, false, true);
		}
		else
		{
			FileManager.Delete(*Node->FullPath);
			if (OnFileDeleted.IsBound())
			{
				OnFileDeleted.Execute(Node->FullPath);
			}
		}

		RefreshTree();
	}
}
