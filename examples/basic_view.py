from html import escape


def render(context):
    """Return self-contained HTML for the ManiVault view host."""
    dataset_name = context.get("dataset_name") or "No dataset loaded"
    dataset_id = context.get("dataset_id") or "—"

    return f"""<!doctype html>
<html>
  <head>
    <meta charset="utf-8">
    <style>
      body {{
        background: #202124;
        color: #f1f3f4;
        font: 14px system-ui, sans-serif;
        margin: 0;
        padding: 32px;
      }}
      .card {{
        border: 1px solid #5f6368;
        border-radius: 10px;
        padding: 24px;
      }}
      code {{ color: #8ab4f8; }}
    </style>
  </head>
  <body>
    <div class="card">
      <h1>Hello from Python</h1>
      <p>Dataset: <strong>{escape(dataset_name)}</strong></p>
      <p>GUID: <code>{escape(dataset_id)}</code></p>
    </div>
  </body>
</html>"""
