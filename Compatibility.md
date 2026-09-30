# Compatibility

| Engine version | Status | Notes |
| --- | --- | --- |
| UE 5.5 | Supported | Targets standard public Slate, Core, DirectoryWatcher, and LiveCoding APIs. |
| UE 5.6 | Supported | Verified across editor Slate architecture. |
| UE 5.7 | Supported | Compatible with modern UE5 reflection and Slate constructs. |
| UE 5.8 | Primary target | Workspace target; built and verified with clean test suite. |

| Platform | Type | Status |
| --- | --- | --- |
| Windows (Win64) | Editor | Fully supported and tested |
| macOS | Editor | Supported (Standard Slate and C++ file system operations) |
| Linux | Editor | Supported (Standard Slate and C++ file system operations) |

## Safe Architecture

- **Editor-Only Scope**: The plugin contains only an `Editor` module (`SlateLivePreview`). It has **zero footprint** in shipping builds and packaged standalone game binaries.
- **Zero Asset Dirtying**: Inspecting files, parsing Slate ASTs, previewing widgets, and navigating code never touches `.uasset` files or marks assets dirty.
- **Pure Native Slate**: Widget live preview relies entirely on Unreal Engine's native Slate rendering pipeline without external dependencies, webviews, or third-party runtimes.
- **Telemetry & Privacy**: 100% offline, zero network requests, zero telemetry, zero background data collection.
- **Live Coding Integration**: Seamlessly interfaces with Unreal Engine's built-in Live Coding system (`ILiveCodingModule`) without modifying core engine source code.
