#pragma once

#include <Dataset.h>
#include <ViewPlugin.h>

#include <QString>

class QWebEngineView;

class PythonViewPlugin final : public mv::plugin::ViewPlugin
{
    Q_OBJECT

public:
    explicit PythonViewPlugin(const mv::plugin::PluginFactory* factory);

    void init() override;
    void loadData(const mv::Datasets& datasets) override;

private:
    void render();
    QString defaultScriptPath() const;
    void showError(const QString& title, const QString& details);

    QWebEngineView* _webView = nullptr;
    mv::Dataset<mv::DatasetImpl> _dataset;
    QString _scriptPath;
};

class PythonViewPluginFactory final : public mv::plugin::ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "studio.manivault.PythonViewPlugin" FILE "PluginInfo.json")

public:
    PythonViewPluginFactory();

    mv::plugin::ViewPlugin* produce() override;
    mv::DataTypes supportedDataTypes() const override;
    QUrl getRepositoryUrl() const override;
};
