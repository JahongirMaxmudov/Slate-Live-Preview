# Changelog

## 1.0.0 - 2026-09-30

### Initial Release

#### C++ Studio & Code Editor
- Added multi-tab code editor with split-pane side-by-side editing (view and edit `.h` and `.cpp` simultaneously).
- Added active pane visual focus indicator with accent borders and dedicated split-pane commands.
- Added full Tab context menu: Close, Close Others, Close All, Move Left/Right, Move to Other Split Pane, Reveal in Explorer.
- Added Quick Header/Source switcher (`Alt + O`) for instant toggling between declarations and implementations.
- Added full Undo / Redo support (`Ctrl + Z`, `Ctrl + Y`).
- Added Quick Open file palette (`Ctrl + P`) with fuzzy search, keyboard navigation (`Up`/`Down`/`Enter`), and click-outside backdrop dismissal.
- Added Search & Replace toolbar with match counter, case-sensitivity toggle, and Next/Previous navigation.
- Added Quick Fix / Code Action: "💡 Generate Definition in .cpp" directly from header declarations with automated parameter extraction, include header insertion, and module export macro stripping.

#### Syntax Highlighting & Visual Styling
- Added high-performance VS Code Dark+ C++ syntax highlighter.
- Tokenized and highlighted C++ language keywords, Unreal Engine reflection macros (`UCLASS`, `UPROPERTY`, `UFUNCTION`, `GENERATED_BODY`, `DECLARE_DYNAMIC_MULTICAST_DELEGATE`, etc.), preprocessor directives, engine types, string literals, multiline comments, numeric constants, operators, and variable identifiers.
- Added Consolas / Monospace editor typography with clear line numbering and visual gutter.

#### Smart Ergonomics & IntelliSense Auto-Completion
- Added smart indentation preservation on `Enter` (retains indentation level of the preceding line).
- Added smart indentation handling on `Tab` and `Backspace` (indents and unindents in clean 4-space stops).
- Added interactive auto-completion / IntelliSense popup triggered when typing or accessing members via `->` and `.`.
- Pre-populated auto-completion dictionary with C++ keywords, Unreal Engine macros, common engine types (`AActor`, `APawn`, `ACharacter`, `UActorComponent`, `USceneComponent`, `UUserWidget`, `FVector`, `FRotator`, `FString`, `TArray`, `TMap`, etc.), Slate widget types, and common API methods.

#### Project C++ Tree & File Management
- Added recursive project tree explorer discovering all C++ source files in project and plugin source directories.
- Added tree item right-click context menu: New C++ Class, Rename / Refactor, Delete File, Show in Explorer, Copy Path.
- Added safe symbol-aware Rename Refactoring: renames companion `.h` and `.cpp` files simultaneously, updates `#include` directives, updates class declaration names, and synchronizes open editor tabs.

#### Unreal-Style "Add C++ Class" Wizard Dialog
- Added visual class picker dialog with selectable cards: Character, Pawn, Actor, ActorComponent, SceneComponent, Slate Widget, UObject, UStruct, Empty C++ Class.
- Added dynamic template naming on card selection (e.g., Character defaults to `MyCharacter`, Slate Widget defaults to `SMyCustomWidget`).
- Added smart Unreal Engine prefix validation preventing duplicate prefixes (e.g. `AMyCharacter` generates class `AMyCharacter` in `MyCharacter.h`, eliminating accidental `AAMyCharacter` naming).
- Added automatic target module and API export macro detection (`PROJECT_API` vs `PLUGIN_API`).
- Generates clean, production-ready, compilable C++ boilerplate with all required includes and `GENERATED_BODY()`.

#### Zero-Compilation Slate Live Preview
- Added instant AST tokenizer and recursive parser for declarative Slate C++ syntax.
- Real-time zero-compilation (0ms) instantiation of native Slate widgets directly in the editor viewport.
- Supported container widgets: `SBorder`, `SVerticalBox`, `SHorizontalBox`, `SOverlay`, `SBox`, `SScrollBox`, `SSpacer`, `SSeparator`.
- Supported display widgets: `STextBlock`, `SImage`, `SColorBlock`.
- Supported input widgets: `SButton`, `SCheckBox`, `SEditableTextBox`, `SMultiLineEditableTextBox`, `SSlider`, `SProgressBar`.
- Added interactive widget mocking: live button click events, checkbox state toggling, slider dragging, and real-time event logging.
- Added Viewport background switcher (Dark, Light, Checkerboard) and preset resolution constraints (400x300, 800x600, Auto).
- Added background file watcher for instant live updates when files are edited externally in IDEs (Visual Studio, Rider, VS Code, Cursor).
- Added auto-snapshot export to `[Project]/Saved/SlateLivePreview/preview.png` for rapid design validation and autonomous AI coding agent workflows.

#### Live Coding & Build Integration
- Added one-click Live Coding trigger button (`Ctrl + Alt + F11`) in the studio toolbar.
- Added real-time compilation status badge (`Ready`, `⚡ Compiling...`, `✓ Success`, `✕ Failed`).
- Added embedded build log output console with live progress notifications.

#### Automated Quality Assurance
- Added automated test suite (`SlateLivePreviewTests.cpp`) validating AST parser accuracy, widget instantiation, snapshot export, and syntax highlighter tokenization.
