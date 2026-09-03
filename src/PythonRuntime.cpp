#include "PythonRuntime.h"
#include "PythonBuildConfig.h"

#include <QFileInfo>

#include <stdexcept>

#undef slots
#include <MVData.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#define slots Q_SLOTS

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(mvstudio_core, module)
{
    module.doc() = "Low-level ManiVault Studio data-model bindings";
    module.attr("__version__") = "0.1.0";
    mvstudio_core::register_mv_data_items(module);
    mvstudio_core::register_mv_core_module(module);
}

namespace
{
    py::dict toPythonDictionary(const QVariantMap& values)
    {
        py::dict result;

        for (auto iterator = values.cbegin(); iterator != values.cend(); ++iterator) {
            const auto key = iterator.key().toStdString();
            const auto& value = iterator.value();

            if (!value.isValid() || value.isNull())
                result[py::str(key)] = py::none();
            else if (value.metaType().id() == QMetaType::QString)
                result[py::str(key)] = py::str(value.toString().toStdString());
            else if (value.metaType().id() == QMetaType::Bool)
                result[py::str(key)] = py::bool_(value.toBool());
            else if (value.metaType().id() == QMetaType::Int ||
                     value.metaType().id() == QMetaType::LongLong ||
                     value.metaType().id() == QMetaType::UInt ||
                     value.metaType().id() == QMetaType::ULongLong)
                result[py::str(key)] = py::int_(value.toLongLong());
            else if (value.metaType().id() == QMetaType::Double ||
                     value.metaType().id() == QMetaType::Float)
                result[py::str(key)] = py::float_(value.toDouble());
            else
                result[py::str(key)] = py::str(value.toString().toStdString());
        }

        return result;
    }
}

PythonRuntime& PythonRuntime::instance()
{
    static PythonRuntime runtime;
    return runtime;
}

PythonRuntime::~PythonRuntime() = default;

bool PythonRuntime::initialize(QString* error)
{
    std::lock_guard lock(_mutex);

    try {
        if (!Py_IsInitialized()) {
            PyConfig config;
            PyConfig_InitPythonConfig(&config);

            auto status = PyConfig_SetString(&config, &config.home, PYTHON_VIEW_PYTHON_HOME);
            if (!PyStatus_Exception(status))
                status = PyConfig_SetString(&config, &config.program_name, PYTHON_VIEW_PYTHON_EXECUTABLE);

            if (PyStatus_Exception(status)) {
                const QString message = status.err_msg
                    ? QString::fromUtf8(status.err_msg)
                    : QStringLiteral("Unable to configure the embedded Python interpreter");
                PyConfig_Clear(&config);
                throw std::runtime_error(message.toStdString());
            }

            _ownedInterpreter = std::make_unique<py::scoped_interpreter>(&config);
        }

        py::gil_scoped_acquire acquire;
        py::module_::import("mvstudio_core");
        return true;
    }
    catch (const py::error_already_set& exception) {
        if (error)
            *error = QString::fromUtf8(exception.what());
    }
    catch (const std::exception& exception) {
        if (error)
            *error = QString::fromUtf8(exception.what());
    }

    return false;
}

PythonRenderResult PythonRuntime::render(const QString& scriptPath, const QVariantMap& context)
{
    std::lock_guard lock(_mutex);
    PythonRenderResult result;

    QString initializationError;
    if (!initialize(&initializationError)) {
        result.traceback = initializationError;
        return result;
    }

    try {
        py::gil_scoped_acquire acquire;

        const auto io = py::module_::import("io");
        const auto stdoutCapture = io.attr("StringIO")();
        const auto stderrCapture = io.attr("StringIO")();
        const auto contextManager = py::module_::import("contextlib");

        py::dict globals;
        globals["__builtins__"] = py::module_::import("builtins");
        globals["__file__"] = scriptPath.toStdString();
        globals["__name__"] = "manivault_python_view";

        {
            auto redirectStdout = contextManager.attr("redirect_stdout")(stdoutCapture);
            auto redirectStderr = contextManager.attr("redirect_stderr")(stderrCapture);
            redirectStdout.attr("__enter__")();
            redirectStderr.attr("__enter__")();

            try {
                py::eval_file(scriptPath.toStdString(), globals, globals);

                if (!globals.contains("render"))
                    throw std::runtime_error("Python view script must define render(context)");

                const auto rendered = globals["render"](toPythonDictionary(context));
                if (!py::isinstance<py::str>(rendered))
                    throw std::runtime_error("render(context) must return an HTML string");

                result.html = QString::fromStdString(rendered.cast<std::string>());
                result.success = true;
            }
            catch (...) {
                redirectStderr.attr("__exit__")(py::none(), py::none(), py::none());
                redirectStdout.attr("__exit__")(py::none(), py::none(), py::none());
                throw;
            }

            redirectStderr.attr("__exit__")(py::none(), py::none(), py::none());
            redirectStdout.attr("__exit__")(py::none(), py::none(), py::none());
        }

        result.standardOutput = QString::fromStdString(stdoutCapture.attr("getvalue")().cast<std::string>());
        result.standardError = QString::fromStdString(stderrCapture.attr("getvalue")().cast<std::string>());
    }
    catch (const py::error_already_set& exception) {
        result.traceback = QString::fromUtf8(exception.what());
    }
    catch (const std::exception& exception) {
        result.traceback = QString::fromUtf8(exception.what());
    }

    return result;
}
