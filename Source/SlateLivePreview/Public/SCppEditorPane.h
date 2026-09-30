// Copyright (c) 2026 Antigravity & User. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

class SMultiLineEditableTextBox;
class SSearchBox;
class STextBlock;
class SBorder;
class SHorizontalBox;
class FCppSyntaxHighlighterMarshaller;

DECLARE_DELEGATE_OneParam(FOnActiveDocumentChanged, const FString& /* FilePath */);
DECLARE_DELEGATE_TwoParams(FOnDocumentContentChanged, const FString& /* FilePath */, const FString& /* Content */);
DECLARE_DELEGATE_OneParam(FOnEditorPaneLog, const FString& /* LogMessage */);
DECLARE_DELEGATE_OneParam(FOnPaneFocused, TSharedPtr<class SCppEditorPane>);
DECLARE_DELEGATE_TwoParams(FOnMoveDocumentRequested, const FString& /* FilePath */, TSharedPtr<class SCppEditorPane> /* SourcePane */);

struct SLATELIVEPREVIEW_API FEditorDocument : public TSharedFromThis<FEditorDocument>
{
	FString FilePath;
	FString Filename;
	FString SavedContent;
	FString CurrentContent;
	bool bIsDirty = false;

	TArray<FString> UndoHistory;
	TArray<FString> RedoHistory;
	bool bIsInternalTextChange = false;
	double LastUndoSnapshotTime = 0.0;
};

enum class EIntelliSenseCategory : uint8
{
	Keyword,
	Type,
	Method,
	Field,
	Macro,
	Delegate,
	Enum,
	Variable
};

struct FIntelliSenseItem
{
	FString DisplayText;
	FString InsertText;
	EIntelliSenseCategory Category = EIntelliSenseCategory::Keyword;
	FString Signature;
	FString Description;
	FString DocUrl;
	FString Scope; // "slot", "string", "array", "macro", "delegate", "general"
};

class SLATELIVEPREVIEW_API SCppEditorPane : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCppEditorPane) {}
		SLATE_EVENT(FOnActiveDocumentChanged, OnActiveDocumentChanged)
		SLATE_EVENT(FOnDocumentContentChanged, OnDocumentContentChanged)
		SLATE_EVENT(FOnEditorPaneLog, OnPaneLog)
		SLATE_EVENT(FSimpleDelegate, OnQuickOpenRequested)
		SLATE_EVENT(FSimpleDelegate, OnNewClassRequested)
		SLATE_EVENT(FOnPaneFocused, OnPaneFocused)
		SLATE_EVENT(FOnMoveDocumentRequested, OnMoveDocumentRequested)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SCppEditorPane() override;

	bool OpenFile(const FString& InFilePath);
	bool CloseFile(const FString& InFilePath);
	void CloseOtherFiles(const FString& KeepFilePath);
	void CloseAllFiles();
	void CloseActiveFile();
	bool SaveCurrentFile();
	bool SaveAll();
	void RevertCurrentFile();

	void Undo();
	void Redo();

	void ToggleFindBar(bool bShow);
	void FindNext();
	void FindPrevious();
	void GoToLine(int32 LineNumber, int32 ColumnNumber = 1);

	FString GetActiveFilePath() const;
	FString GetActiveContent() const;
	bool HasOpenFiles() const { return OpenDocuments.Num() > 0; }
	bool IsCurrentDirty() const;

	void FocusEditor();
	void SetIsActivePane(bool bInActive);
	bool IsActivePane() const { return bIsActivePane; }
	void MoveDocumentTab(int32 OldIndex, int32 NewIndex);

	// Gutter & Status Bar Accessors
	int32 GetTotalLineCount() const;
	float GetEditorLineHeight() const;
	int32 GetCurrentLineIndex() const;
	int32 GetCurrentColumnIndex() const;

	void InsertCodeAtCursor(const FString& InCode);
	bool HasGhostText() const { return bGhostTextVisible && !ActiveGhostText.IsEmpty(); }
	FString GetActiveGhostText() const { return ActiveGhostText; }
	FTextLocation GetGhostTextLocation() const { return GhostTextLocation; }

	friend class SCppEditorGutter;
	friend class SCppEditorGhostOverlay;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FOnActiveDocumentChanged OnActiveDocumentChanged;
	FOnDocumentContentChanged OnDocumentContentChanged;
	FOnEditorPaneLog OnPaneLog;
	FSimpleDelegate OnQuickOpenRequested;
	FSimpleDelegate OnNewClassRequested;
	FOnPaneFocused OnPaneFocused;
	FOnMoveDocumentRequested OnMoveDocumentRequested;

	bool bIsActivePane = false;
	TSharedPtr<SBorder> PaneContainerBorder;

	TSharedPtr<SWidget> OnEditorContextMenuOpening();
	void ShowTabContextMenu(const FPointerEvent& MouseEvent, TSharedPtr<FEditorDocument> Doc, int32 DocIndex);

	TArray<TSharedPtr<FEditorDocument>> OpenDocuments;
	int32 ActiveDocumentIndex = INDEX_NONE;

	// Widgets
	TSharedPtr<SHorizontalBox> TabStripBox;
	TSharedPtr<SHorizontalBox> BreadcrumbsBox;
	TSharedPtr<SMultiLineEditableTextBox> CodeTextBox;
	TSharedPtr<class SCppEditorGutter> GutterWidget;
	TSharedPtr<class SCppEditorGhostOverlay> GhostOverlayWidget;
	TSharedPtr<FCppSyntaxHighlighterMarshaller> SyntaxMarshaller;
	FDelegateHandle SettingsChangedHandle;

	// AI Copilot Ghost Text State
	FString ActiveGhostText;
	FTextLocation GhostTextLocation;
	bool bGhostTextVisible = false;
	double LastKeyStrokeTime = 0.0;
	bool bPendingAiRequest = false;

	void TriggerAiInlineCompletion();
	void OnAiCompletionReceived(const FString& CompletionText, bool bSuccess, FTextLocation OriginalCursorLoc);
	void CommitGhostText();
	void DismissGhostText();
	void UpdateBreadcrumbs();

	void ApplySettings();

	// Find Bar Widgets
	TSharedPtr<SBorder> FindBarBorder;
	TSharedPtr<SSearchBox> FindSearchBox;
	TSharedPtr<STextBlock> MatchCountTextBlock;
	bool bFindBarVisible = false;
	bool bMatchCase = false;
	FString CurrentSearchQuery;
	struct FTextMatch
	{
		int32 LineIndex = 0;
		int32 ColumnIndex = 0;
	};
	TArray<FTextMatch> CurrentMatches;
	int32 CurrentMatchIndex = INDEX_NONE;

	// IntelliSense Widgets & State
	TSharedPtr<SBorder> IntelliSenseBox;
	TSharedPtr<SListView<TSharedPtr<FIntelliSenseItem>>> IntelliSenseListView;
	TSharedPtr<SBorder> IntelliSenseDocBox;
	TSharedPtr<STextBlock> DocSignatureTextBlock;
	TSharedPtr<STextBlock> DocDescriptionTextBlock;
	TSharedPtr<STextBlock> DocCategoryTextBlock;
	TSharedPtr<SButton> DocUrlButton;
	FString ActiveDocUrl;

	TArray<TSharedPtr<FIntelliSenseItem>> FilteredIntelliSenseItems;
	bool bIntelliSenseActive = false;

	static void EnsureIntelliSenseDatabaseLoaded();
	void UpdateIntelliSense();
	void DismissIntelliSense();
	void CommitIntelliSense();
	void IntelliSenseNavigate(int32 Direction);
	void UpdateDocPanel(TSharedPtr<FIntelliSenseItem> Item);
	void OnIntelliSenseSelectionChanged(TSharedPtr<FIntelliSenseItem> SelectedItem, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> OnGenerateIntelliSenseRow(TSharedPtr<FIntelliSenseItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnIntelliSenseItemDoubleClicked(TSharedPtr<FIntelliSenseItem> Item);
	FMargin GetIntelliSenseMargin() const;

	// Hover Documentation Card (Quick Info on Hover)
	TSharedPtr<SBorder> HoverDocCard;
	TSharedPtr<STextBlock> HoverDocCategoryText;
	TSharedPtr<STextBlock> HoverDocSignatureText;
	TSharedPtr<STextBlock> HoverDocDescriptionText;
	TSharedPtr<SButton> HoverDocUrlButton;
	FString ActiveHoverDocUrl;
	FVector2D HoverDocScreenPosition = FVector2D::ZeroVector;
	FVector2D LastMouseScreenPosition = FVector2D::ZeroVector;
	double LastMouseMoveTime = 0.0;
	FString LastHoveredWord;
	bool bHoverDocVisible = false;

	FMargin GetHoverDocMargin() const;
	FString GetWordAtScreenPosition(const FVector2D& ScreenPos);
	void ShowHoverDoc(const FString& Word, const FVector2D& ScreenPos);
	void DismissHoverDoc();
	TSharedPtr<FIntelliSenseItem> FindDocItemForWord(const FString& Word) const;

	bool bEnterKeyHandledInKeyDown = false;
	FReply HandleCodeTextBoxKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	FReply HandleCodeTextBoxKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent);
	void HarvestDocumentSymbols(TArray<TSharedPtr<FIntelliSenseItem>>& OutSymbols) const;

	void RebuildTabStrip();
	void SetActiveDocumentIndex(int32 NewIndex);
	void OnCodeTextChanged(const FText& NewText);

	// Productivity Actions
	void ToggleLineComment();
	void DuplicateLine();
	void MoveLine(int32 Direction);
	void DeleteLine();
	void ToggleHeaderSource();
	void QuickActionGenerateDefinition();

	void PerformSearch(bool bJumpToFirst = false);
	void JumpToMatch(int32 MatchIndex);
	void Log(const FString& Message);

	// Status Bar Widgets
	TSharedPtr<STextBlock> StatusBarCursorText;
	TSharedPtr<STextBlock> StatusBarDocStatsText;
};
