// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

DECLARE_DELEGATE_OneParam(FOnCppSourceFileSelected, const FString& /* FilePath */);
DECLARE_DELEGATE_TwoParams(FOnSourceFilesRenamed, const FString& /* OldPath */, const FString& /* NewPath */);
DECLARE_DELEGATE_OneParam(FOnSourceFileDeleted, const FString& /* FilePath */);

struct SLATELIVEPREVIEW_API FCppSourceNode : public TSharedFromThis<FCppSourceNode>
{
	FString Name;
	FString FullPath;
	bool bIsDirectory = false;
	TArray<TSharedPtr<FCppSourceNode>> Children;
	TWeakPtr<FCppSourceNode> Parent;

	bool MatchesFilter(const FString& Filter) const
	{
		if (Filter.IsEmpty()) return true;
		if (Name.Contains(Filter, ESearchCase::IgnoreCase)) return true;

		for (const auto& Child : Children)
		{
			if (Child->MatchesFilter(Filter)) return true;
		}
		return false;
	}
};

class SLATELIVEPREVIEW_API SCppProjectTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppProjectTree) {}
		SLATE_EVENT(FOnCppSourceFileSelected, OnFileSelected)
		SLATE_EVENT(FSimpleDelegate, OnNewClassRequested)
		SLATE_EVENT(FOnSourceFilesRenamed, OnFilesRenamed)
		SLATE_EVENT(FOnSourceFileDeleted, OnFileDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void RefreshTree();
	void Refresh() { RefreshTree(); }
	void SelectFile(const FString& InFilePath);
	void GetAllFiles(TArray<FString>& OutFiles) const;
	FString GetSelectedDirectory() const;
	TSharedPtr<FCppSourceNode> GetSelectedNode() const;

	void OpenRenameDialog(TSharedPtr<FCppSourceNode> Node);
	void DeleteNode(TSharedPtr<FCppSourceNode> Node);

private:
	FOnCppSourceFileSelected OnFileSelected;
	FSimpleDelegate OnNewClassRequested;
	FOnSourceFilesRenamed OnFilesRenamed;
	FOnSourceFileDeleted OnFileDeleted;

	TArray<TSharedPtr<FCppSourceNode>> RootNodes;
	TArray<TSharedPtr<FCppSourceNode>> FilteredNodes;
	TSharedPtr<STreeView<TSharedPtr<FCppSourceNode>>> TreeView;
	FString CurrentFilterText;

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FCppSourceNode> InItem, const TSharedRef<STableViewBase>& OwnerTable);
	void OnGetChildren(TSharedPtr<FCppSourceNode> InItem, TArray<TSharedPtr<FCppSourceNode>>& OutChildren);
	void OnSelectionChanged(TSharedPtr<FCppSourceNode> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnFilterTextChanged(const FText& InFilterText);
	TSharedPtr<SWidget> OnContextMenuOpening();

	void ScanDirectoryRecursive(const FString& InDirPath, const TSharedPtr<FCppSourceNode>& ParentNode);
	void ApplyFilter();
};
