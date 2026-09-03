#include "PythonViewPlugin.h"

#include "PythonRuntime.h"

#include <Application.h>
#include <DatasetsMimeData.h>
#include <PointData/PointData.h>
#include <widgets/DropWidget.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QVBoxLayout>
#include <QWebEngineView>

Q_PLUGIN_METADATA(IID "studio.manivault.PythonViewPlugin")

using namespace mv;
using namespace mv::plugin;

PythonViewPlugin::PythonViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _webView(new QWebEngineView(&getWidget())),
    _dropWidget(new gui::DropWidget(_webView))
{
    setObjectName("PythonView");

    _webView->setAcceptDrops(true);

    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_webView);
    getWidget().setLayout(layout);

    _dropWidget->setDropIndicatorWidget(new gui::DropWidget::DropIndicatorWidget(
        _webView,
        "No data loaded",
        "Drag a points dataset from the data hierarchy and drop it here"
    ));

    _dropWidget->initialize([this](const QMimeData* mimeData) -> gui::DropWidget::DropRegions {
        gui::DropWidget::DropRegions dropRegions;
        const auto* datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (!datasetsMimeData)
            return dropRegions;

        if (datasetsMimeData->getDatasetsCount() != 1) {
            dropRegions << new gui::DropWidget::DropRegion(
                this,
                "Unsupported drop",
                "Drop exactly one points dataset",
                "exclamation-circle",
                false
            );
            return dropRegions;
        }

        const auto& candidateDataset = datasetsMimeData->getDatasetsRef().first();
        if (!candidateDataset.isValid() || candidateDataset->getDataType() != PointType) {
            dropRegions << new gui::DropWidget::DropRegion(
                this,
                "Incompatible data",
                "Only points datasets are supported",
                "exclamation-circle",
                false
            );
            return dropRegions;
        }

        const auto datasetName = candidateDataset->getGuiName();
        dropRegions << new gui::DropWidget::DropRegion(
            this,
            "Points",
            QString("Visualize %1 with Python").arg(datasetName),
            "map-marker-alt",
            true,
            [this, candidateDataset]() {
                loadData({ candidateDataset });
            }
        );

        return dropRegions;
    });
}

void PythonViewPlugin::init()
{
    _scriptPath = defaultScriptPath();

    QString error;
    if (!PythonRuntime::instance().initialize(&error)) {
        showError("Unable to initialize Python", error);
        return;
    }

    render();
}

void PythonViewPlugin::loadData(const Datasets& datasets)
{
    if (datasets.empty())
        return;

    _dataset = datasets.front();
    _dropWidget->setShowDropIndicator(false);
    render();
}

void PythonViewPlugin::render()
{
    if (_scriptPath.isEmpty() || !QFileInfo::exists(_scriptPath)) {
        showError("Python view script not found", _scriptPath);
        return;
    }

    QVariantMap context;
    context.insert("dataset_id", _dataset.isValid() ? _dataset.getDatasetId() : QString());
    context.insert("dataset_name", _dataset.isValid() ? _dataset->getGuiName() : QString());

    const auto result = PythonRuntime::instance().render(_scriptPath, context);
    if (!result.success) {
        showError("Python rendering failed", result.traceback + "\n" + result.standardError);
        return;
    }

    _webView->setHtml(result.html, QUrl::fromLocalFile(QFileInfo(_scriptPath).absolutePath() + QDir::separator()));
}

QString PythonViewPlugin::defaultScriptPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("examples/PythonViewPlugin/basic_view.py");
}

void PythonViewPlugin::showError(const QString& title, const QString& details)
{
    const auto escapedTitle = title.toHtmlEscaped();
    const auto escapedDetails = details.toHtmlEscaped();
    _webView->setHtml(QStringLiteral(
        "<!doctype html><html><body style='font-family:sans-serif;padding:24px'>"
        "<h2>%1</h2><pre style='white-space:pre-wrap'>%2</pre></body></html>")
        .arg(escapedTitle, escapedDetails));
}

PythonViewPluginFactory::PythonViewPluginFactory() :
    ViewPluginFactory(false, false)
{
    setIconByName("python");
    getPluginMetadata().setDescription("Renders ManiVault data with Python-generated HTML.");
    getPluginMetadata().setSummary("A lightweight Python-backed view plugin prototype.");
    getPluginMetadata().setLicenseText("This plugin is distributed under the LGPL v3.0 license.");
}

ViewPlugin* PythonViewPluginFactory::produce()
{
    return new PythonViewPlugin(this);
}

DataTypes PythonViewPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

QUrl PythonViewPluginFactory::getRepositoryUrl() const
{
    return { "https://github.com/ManiVaultStudio/PythonViewPlugin" };
}
