#include "PythonViewPlugin.h"

#include "PythonRuntime.h"

#include <Application.h>
#include <DatasetsMimeData.h>
#include <PointData/PointData.h>
#include <widgets/DropWidget.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMimeData>
#include <QVBoxLayout>
#include <QWidget>

Q_PLUGIN_METADATA(IID "studio.manivault.PythonViewPlugin")

using namespace mv;
using namespace mv::plugin;

PythonViewPlugin::PythonViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _viewContainer(new QWidget(&getWidget())),
    _dropWidget(new gui::DropWidget(&getWidget()))
{
    setObjectName("PythonView");

    _viewContainer->hide();
    getWidget().setAcceptDrops(true);

    auto* viewLayout = new QVBoxLayout(_viewContainer);
    viewLayout->setContentsMargins(0, 0, 0, 0);

    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_viewContainer);
    getWidget().setLayout(layout);

    auto* dropIndicatorWidget = new gui::DropWidget::DropIndicatorWidget(
        &getWidget(),
        "No data loaded",
        "Drag a points dataset from the data hierarchy and drop it here"
    );
    _dropWidget->setDropIndicatorWidget(dropIndicatorWidget);
    dropIndicatorWidget->show();
    dropIndicatorWidget->raise();
    _dropWidget->show();
    _dropWidget->raise();

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

PythonViewPlugin::~PythonViewPlugin()
{
    if (_pythonWidget) {
        delete _pythonWidget;
        PythonRuntime::instance().releaseWidget(_pythonWidget);
    }
}

void PythonViewPlugin::init()
{
    _scriptPath = defaultScriptPath();

    QString error;
    if (!PythonRuntime::instance().initialize(&error)) {
        showError("Unable to initialize Python", error);
        return;
    }

}

void PythonViewPlugin::loadData(const Datasets& datasets)
{
    if (datasets.empty())
        return;

    _dataset = datasets.front();
    _dropWidget->setShowDropIndicator(false);
    _viewContainer->show();
    _dropWidget->raise();
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

    if (_pythonWidget) {
        _viewContainer->layout()->removeWidget(_pythonWidget);
        delete _pythonWidget;
        PythonRuntime::instance().releaseWidget(_pythonWidget);
        _pythonWidget = nullptr;
    }

    QString error;
    _pythonWidget = PythonRuntime::instance().createWidget(_scriptPath, context, &error);
    if (!_pythonWidget) {
        showError("Python rendering failed", error);
        return;
    }

    _pythonWidget->setParent(_viewContainer);
    _viewContainer->layout()->addWidget(_pythonWidget);
    _pythonWidget->show();
}

QString PythonViewPlugin::defaultScriptPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("examples/PythonViewPlugin/basic_view.py");
}

void PythonViewPlugin::showError(const QString& title, const QString& details)
{
    const auto escapedTitle = title.toHtmlEscaped();
    const auto escapedDetails = details.toHtmlEscaped();
    auto* errorWidget = new QWidget(_viewContainer);
    auto* errorLayout = new QVBoxLayout(errorWidget);
    auto* errorLabel = new QLabel(QString("<h2>%1</h2><pre>%2</pre>").arg(escapedTitle, escapedDetails));
    errorLabel->setTextFormat(Qt::RichText);
    errorLabel->setWordWrap(true);
    errorLayout->addWidget(errorLabel);
    _viewContainer->layout()->addWidget(errorWidget);
    _viewContainer->show();
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
