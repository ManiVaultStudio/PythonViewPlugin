import numpy as np

import mvstudio_core
from PySide6.QtCore import QRectF
from PySide6.QtGui import QColor, QPainter, QPainterPath, QPen
from PySide6.QtWidgets import QWidget


class PointView(QWidget):
    """Small native Qt scatterplot implemented entirely in Python."""

    def __init__(self, points, title):
        super().__init__()
        self._points = np.asarray(points[:, :2], dtype=np.float64)
        self._title = title
        self.setMinimumSize(200, 160)

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.setRenderHint(QPainter.Antialiasing, self._points.shape[0] <= 20_000)
        painter.fillRect(self.rect(), self.palette().window())

        plot = self.rect().adjusted(48, 42, -20, -42)
        if plot.width() <= 0 or plot.height() <= 0:
            return

        finite = np.isfinite(self._points).all(axis=1)
        points = self._points[finite]
        if points.size == 0:
            return

        minimum = points.min(axis=0)
        maximum = points.max(axis=0)
        span = maximum - minimum
        span[span == 0.0] = 1.0

        painter.setPen(self.palette().text().color())
        painter.drawText(16, 24, self._title)
        painter.drawText(plot.left(), self.height() - 14, "Dimension 1")
        painter.save()
        painter.translate(16, plot.bottom())
        painter.rotate(-90)
        painter.drawText(0, 0, "Dimension 2")
        painter.restore()

        painter.setPen(QPen(self.palette().mid().color(), 1.0))
        painter.drawRect(plot)

        normalized = (points - minimum) / span
        xs = plot.left() + normalized[:, 0] * plot.width()
        ys = plot.bottom() - normalized[:, 1] * plot.height()
        radius = 2.5
        path = QPainterPath()
        for x, y in zip(xs, ys):
            path.addEllipse(QRectF(float(x - radius), float(y - radius), 2 * radius, 2 * radius))

        painter.setPen(QPen(QColor("#445ee9"), 0.8))
        painter.setBrush(QColor("#5b6eea"))
        painter.drawPath(path)


def create_view(context):
    """Create and return the native Qt widget hosted by ManiVault."""
    dataset_id = context.get("dataset_id")
    if not dataset_id:
        raise RuntimeError("A points dataset is required")

    points = mvstudio_core.get_data_for_item(dataset_id)
    if points.ndim != 2 or points.shape[1] < 2:
        raise RuntimeError("The points dataset must contain at least two dimensions")

    return PointView(points, context.get("dataset_name") or "Points")

