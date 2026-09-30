// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SCppProjectTree.h"
#include "SCppEditorPane.h"
#include "SSlateLivePreviewViewport.h"
#include "CppAiAssistant.h"

class SMultiLineEditableTextBox;
class STextBlock;
class SBorder;
class SSplitter;
class SSearchBox;
class SCppAiAssistantDrawer;
template<typename ItemType> class SListView;
class ITableRow;
class STableViewBase;

class SLATELIVEPREVIEW_API SCppStudioTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppStudioTab) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCppStudioTab() override;

	void OpenFile(const FString& InFilePath);
	void OpenFileAtLine(const FString& InFilePath, int32 LineNumber, int32 ColumnNumber = 1);
	void OpenNewClassWizard();
	void ToggleQuickOpen(bool bShow);

	void SaveCurrentFile();
	void SaveAllFiles();
	void TriggerLiveCoding();
	void ToggleSlatePreview(bool bShow);
	void ToggleSplitView();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TSharedPtr<SCppProjectTree> ProjectTree;
	TSharedPtr<SCppEditorPane> LeftEditorPane;
	TSharedPtr<SCppEditorPane> RightEditorPane;
	TWeakPtr<SCppEditorPane> ActiveEditorPane;

	TSharedPtr<STextBlock> FileHeaderTextBlock;
	TSharedPtr<SImage> StatusBadgeIcon;
	TSharedPtr<STextBlock> StatusBadgeTextBlock;
	TSharedPtr<SMultiLineEditableTextBox> OutputConsoleTextBox;
	TSharedPtr<SSlateLivePreviewViewport> SlatePreviewViewport;
	TSharedPtr<SBorder> PreviewPanelBorder;
	TSharedPtr<SSplitter> EditorAreaSplitter;
	TSharedPtr<SSplitter> MainHorizontalSplitter;

	// Quick Open (Ctrl+P)
	TSharedPtr<SBorder> QuickOpenOverlayWidget;
	TSharedPtr<SSearchBox> QuickOpenSearchBox;
	TSharedPtr<SListView<TSharedPtr<FString>>> QuickOpenListView;
	TArray<TSharedPtr<FString>> AllProjectFiles;
	TArray<TSharedPtr<FString>> FilteredQuickOpenFiles;
	bool bQuickOpenVisible = false;

	// Compiler Error Navigation
	TSharedPtr<SButton> ErrorJumpButton;
	TSharedPtr<SImage> ErrorJumpIcon;
	TSharedPtr<STextBlock> ErrorJumpTextBlock;
	FString LastParsedErrorFile;
	int32 LastParsedErrorLine = 0;
	int32 LastParsedErrorCol = 0;

	TSharedPtr<SCppAiAssistantDrawer> AiAssistantDrawer;
	bool bShowAiDrawer = false;

	bool bIsSplitView = false;
	bool bShowSlatePreview = true;
	FString OutputConsoleAccumulator;
	FDelegateHandle PatchCompleteDelegateHandle;
	bool bWasLiveCodingCompiling = false;
	bool bPatchSucceeded = false;

	void OnFileSelectedFromTree(const FString& FilePath);
	void OnPaneActiveDocumentChanged(const FString& FilePath, TSharedPtr<SCppEditorPane> SourcePane);
	void OnPaneDocumentContentChanged(const FString& FilePath, const FString& Content);
	void OnPaneFocusedChanged(TSharedPtr<SCppEditorPane> FocusedPane);
	void OnMoveDocumentBetweenPanes(const FString& FilePath, TSharedPtr<SCppEditorPane> SourcePane);
	void OnFilesRenamedInTree(const FString& OldPath, const FString& NewPath);
	void OnFileDeletedInTree(const FString& DeletedPath);

	void RefreshProjectFileList();
	void OnQuickOpenSearchTextChanged(const FText& SearchText);
	void CommitQuickOpenSelection();
	void QuickOpenNavigate(int32 Direction);
	TSharedRef<ITableRow> OnGenerateQuickOpenRow(TSharedPtr<FString> FileItem, const TSharedRef<STableViewBase>& OwnerTable);

	void CheckAndParseErrorLine(const FString& LogLine);
	FReply OnErrorJumpClicked();
	void OnClassCreated(const FString& HeaderPath, const FString& CppPath);

	FReply OnSaveClicked();
	FReply OnSaveAllClicked();
	FReply OnLiveCodingClicked();
	FReply OnRevertClicked();
	FReply OnToggleSplitClicked();
	FReply OnTogglePreviewClicked();
	FReply OnToggleAiDrawerClicked();
	void ToggleAiDrawer(bool bShow);
	void InsertCodeFromAi(const FString& Code);
	void ApplyCodeFromAi(const FString& Code);
	FAiEditorContext GetActiveEditorContext() const;
	FString GetRecentCompilerErrors() const;
	FReply OnNewClassClicked();
	FReply OnQuickOpenClicked();
	FReply OnSettingsClicked();
	void OpenSettingsDialog();

	void UpdateFileHeader();
	void SetStatus(const FString& StatusText, const FLinearColor& StatusColor);
	void AppendLog(const FString& LogLine);
	void UpdateSlatePreviewIfApplicable();
	void OnPatchComplete();
};
