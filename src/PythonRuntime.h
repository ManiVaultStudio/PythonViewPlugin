#pragma once

#include <QString>
#include <QVariantMap>

#include <memory>
#include <mutex>

namespace pybind11 {
    class scoped_interpreter;
}

/** Result of one isolated Python view invocation. */
struct PythonRenderResult
{
    bool success = false;
    QString html;
    QString standardOutput;
    QString standardError;
    QString traceback;
};

/**
 * Process-wide access point for the embedded Python interpreter.
 *
 * The runtime only owns/finalizes Python when it initialized Python itself.
 * This lets an already-loaded JupyterPlugin remain the interpreter owner.
 */
class PythonRuntime final
{
public:
    static PythonRuntime& instance();

    PythonRuntime(const PythonRuntime&) = delete;
    PythonRuntime& operator=(const PythonRuntime&) = delete;

    bool initialize(QString* error = nullptr);
    PythonRenderResult render(const QString& scriptPath, const QVariantMap& context);

private:
    PythonRuntime() = default;
    ~PythonRuntime();

    std::unique_ptr<pybind11::scoped_interpreter> _ownedInterpreter;
    std::recursive_mutex _mutex;
};
