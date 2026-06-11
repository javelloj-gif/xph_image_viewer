# Plan: Add Self-Contained Build Note to NATIVE_USAGE_GUIDE.md

## Goal
Add a note documenting the requirements and implications of a self-contained (statically linked) build vs dynamic build.

## Location
New subsection in **Section 10 (Distribution and Packaging)**, after section 10.7, or as a new section 10.8.

## Content to Add

```markdown
### 10.8 Self-Contained Build (Static Linking)

By default, LenteUV uses **dynamic linking** and requires the MSVC Runtime to be installed on the target system. A self-contained build embeds the runtime into the executable, eliminating external dependencies (except Windows system DLLs).

| Aspect | Dynamic (Default) | Static (Self-Contained) |
|--------|-------------------|-------------------------|
| **Exe size** | ~927 KB | ~5-15 MB (depends on CRT/STL usage) |
| **Dependencies** | Requires VC++ Runtime installer | No installer needed |
| **Startup** | DLL loading overhead | Slightly faster |
| **Security patches** | OS updates VC++ Runtime | Must recompile to patch CRT |
| **Compatibility** | Works with older runtimes | Fixed to compile-time CRT version |

#### Implementation

To enable static linking, modify `native/CMakeLists.txt`:

```cmake
# Find and replace the runtime library flag for Release builds
if(MSVC)
    # For static CRT (self-contained):
    add_compile_options(/MT)
    # For dynamic CRT (default):
    # add_compile_options(/MD)
endif()
```

> [!NOTE]
> The VC++ Runtime is pre-installed on most Windows 10/11 systems via Windows Update. The default dynamic build works for ~99% of users without additional installation.

> [!TIP]
> If you need a truly single-file executable with no external dependencies at all, consider using a third-party tool like **Enigma Virtual Box** or **BoxedApp Packer** to package the exe with its DLL dependencies.
```

## Steps
1. Open `NATIVE_USAGE_GUIDE.md`
2. Navigate to end of Section 10.7
3. Insert new subsection 10.8 with the content above