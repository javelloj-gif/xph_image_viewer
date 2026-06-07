# Plan: Bug de Crash en Versión Portable - Session.txt

## Problema

La versión portable crashea con `0xC0000005` (ACCESS_VIOLATION) cuando se ejecuta mediante asociación de archivos de Windows ("Abrir siempre con"). El crash ocurre entre `"FolderDataManager initialized"` y `"Session loaded"`.

**Eliminando `user_data/session.txt` manualmente, la app vuelve a funcionar.**

---

## Causa Raíz Identificada

### 1. Diferencia clave: Release vs Portable

| Versión | `session.txt` existe? | Resultado |
|---------|------------------------|-----------|
| Release (`native\build\bin\Release\`) | **NO** | `LoadSession()` retorna `false` inmediatamente. No crash. |
| Portable | **SÍ** (de instalación previa o `user_data/` en el proyecto) | `LoadSession()` intenta procesar archivo malformed. Crash. |

### 2. El archivo `session.txt` del repositorio tiene formato inesperado

```
Ruta1
(blank)
(blank)
(blank)
Ruta2
```

El formato esperado es 4 líneas por entrada (ruta, MD5, modifiers, blank). Pero el archivo tiene **3 líneas en blanco** entre entradas. Esto causa que `LoadSession()` lea líneas incorrectas como rutas.

### 3. `GetSessionFilePath()` busca hacia arriba 6 niveles

```cpp
for (int i = 0; i < 6; ++i) {
    p = p.parent_path();
    if (std::filesystem::exists(p / "user_data")) {
        return (p / "user_data" / "session.txt").wstring();
    }
}
```

Si existe `user_data/` en cualquier nivel superior (ej: del proyecto, de instalación anterior), el portable lo usa.

### 4. Bug potencial en `LoadSession()` línea 163

```cpp
for (size_t i = 0; i < lines.size(); i += 4) {
    if (i >= lines.size()) break;  // <-- Check DESPUÉS del acceso potential
    std::wstring fPath = toWString(lines[i]);
```

El check `i >= lines.size()` está **después** del incremento pero **antes** del siguiente acceso. Esto es correcto para el loop, pero si `lines` es modificado externamente o tiene datos corruptos, podría causar problemas.

---

## Plan de Solución

### Fase 1: Corrección Inmediata (create_portable_zip.ps1)

**Problema**: El script actual NO crea `user_data/session.txt` vacío. El portable hereda `session.txt` de instalaciones previas o del proyecto.

**Solución**:
```powershell
# Crear directorio user_data y session.txt vacío
New-Item -ItemType Directory -Path (Join-Path $TempDir "user_data") | Out-Null
"" | Out-File -FilePath (Join-Path $TempDir "user_data" "session.txt") -Encoding UTF8
```

### Fase 2: Corrección de LoadSession() (folder_data_manager.cpp)

**Problema**: El parser no valida el formato del archivo antes de procesar.

**Solución** - Hacer el parser más robusto:

```cpp
bool FolderDataManager::LoadSession() {
    std::wstring path = GetSessionFilePath();
    if (!std::filesystem::exists(path)) return false;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) return false;

    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }

    // Validar que hay suficientes líneas para al menos una entrada
    if (lines.size() < 4) return false;

    auto toWString = [](const std::string& str) -> std::wstring {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::vector<wchar_t> w(len);
        if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, w.data(), len) == 0) {
            return L"";
        }
        return std::wstring(w.data());
    };

    bool loadedAny = false;
    for (size_t i = 0; i + 3 < lines.size(); i += 4) {  // <-- CORREGIDO: check +3 antes de acceder
        std::wstring fPath = toWString(lines[i]);
        if (fPath.empty()) continue;  // Skip empty paths
        if (!std::filesystem::exists(fPath)) continue;  // Skip non-existent paths
        try {
            AddFolder(fPath, true);
            loadedAny = true;
        } catch (...) {
            LogError(std::format(L"Failed to add folder from session: {}", fPath));
        }
    }

    if (loadedAny && !m_folders.empty()) {
        SetCurrentFolderIndex(0);
    }
    return loadedAny;
}
```

### Fase 3: Verificación de LoadFavorites()

`LoadFavorites()` tiene la misma estructura. Debe tener el mismo fix.

### Fase 4: Verificación

1. Compilar el proyecto
2. Ejecutar `create_portable_zip.ps1`
3. Verificar que el ZIP contiene `user_data/session.txt` vacío
4. Extraer en nueva carpeta
5. Asociar tipo de archivo
6. Abrir imagen → cerrar → reabrir → **no crash**

---

## Archivos a Modificar

| Archivo | Cambio |
|----------|--------|
| `create_portable_zip.ps1` | Agregar paso para crear `user_data/session.txt` vacío |
| `native/src/library/folder_data_manager.cpp` | Fix `LoadSession()` y `LoadFavorites()` con validación robusta |

---

## Pregunta Clave Respondida

**¿Por qué la versión de Release no sufre el error, pero la portable sí?**

Porque en Release **no existe** `session.txt` (ni `user_data/`). La función `GetSessionFilePath()` busca hacia arriba 6 niveles; si no encuentra `user_data`, usa `session.txt` en el directorio del ejecutable. En Release, ese archivo no existe, entonces `LoadSession()` retorna `false` inmediatamente.

En el portable, si existe un `user_data/session.txt` en algún nivel superior (del proyecto, de instalación previa), el portable lo encuentra y usa. Si ese archivo tiene formato inesperado (como el del repositorio con 3 líneas en blanco entre entradas), `LoadSession()` intenta procesar líneas incorrectas como rutas, causando el crash `0xC0000005`.

La solución es asegurar que el portable **comience limpio** con `user_data/session.txt` vacío, y que `LoadSession()` sea robusto contra archivos malformed.