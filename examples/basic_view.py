from html import escape

import mvstudio_core


def _message(title, description):
    """Create a small self-contained status page."""
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
      <h2>{escape(title)}</h2>
      <p>{escape(description)}</p>
    </div>
  </body>
</html>"""


def render(context):
    """Render the first two dimensions of a ManiVault points dataset."""
    dataset_id = context.get("dataset_id")
    dataset_name = context.get("dataset_name") or "Points"

    if not dataset_id:
        return _message(
            "No data loaded",
            "Drop a points dataset from the data hierarchy onto this view.",
        )

    try:
        import plotly.express as px
    except ImportError as error:
        raise RuntimeError(
            "Plotly is required by this example. Install it into the Python "
            "environment used to build PythonViewPlugin with: "
            "python -m pip install plotly"
        ) from error

    points = mvstudio_core.get_data_for_item(dataset_id)

    if points.ndim != 2:
        return _message(
            "Unsupported point data",
            f"Expected a two-dimensional array, received shape {points.shape}.",
        )

    if points.shape[1] < 2:
        return _message(
            "Not enough dimensions",
            "The points dataset must contain at least two dimensions.",
        )

    figure = px.scatter(
        x=points[:, 0],
        y=points[:, 1],
        labels={"x": "Dimension 1", "y": "Dimension 2"},
        title=dataset_name,
        render_mode="webgl" if points.shape[0] > 5_000 else "auto",
    )
    figure.update_layout(
        autosize=True,
        margin={"l": 55, "r": 20, "t": 55, "b": 50},
        template="plotly_dark",
    )

    return figure.to_html(
        full_html=True,
        include_plotlyjs="inline",
        config={"responsive": True, "displaylogo": False},
    )
