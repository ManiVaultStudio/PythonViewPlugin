# Python View Plugin

Experimental ManiVault Studio view plugin that executes a Python
`render(context)` function and displays its returned HTML in `QWebEngineView`.

The repository uses JupyterPlugin as a Git submodule so that ManiVault's
pybind11 data-model bindings are reused instead of copied.

## Clone

```console
git clone --recursive https://github.com/ManiVaultStudio/PythonViewPlugin.git
```

For an existing checkout:

```console
git submodule update --init --recursive
```

## Python view contract

A view script defines:

```python
def render(context):
    return "<html>...</html>"
```

The initial context contains `dataset_id` and `dataset_name`. Scripts may
import `mvstudio_core` to access the same low-level bindings used by the
JupyterPlugin.

## Prototype limitations

- The bundled example path is currently fixed.
- Rendering is synchronous and serialized.
- JavaScript-to-ManiVault selection bridging is not implemented yet.
- Interpreter coordination with a simultaneously loaded JupyterPlugin still
  needs to move into a shared runtime library before production use.
- The prototype records the configured Python installation as its runtime home;
  a distributable build still needs to package a relocatable Python standard
  library and third-party environment.
