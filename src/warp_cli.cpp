#include "warp_cli.h"

#include <QProcess>

WarpCli::WarpCli(QObject *parent) : QObject(parent) {}

bool WarpCli::isRunning(const QString &requestId) const {
    auto *proc = m_running.value(requestId, nullptr);
    return proc != nullptr && proc->state() != QProcess::NotRunning;
}

void WarpCli::run(const QString &requestId, const QStringList &args) {
    startProcess(requestId, QStringList{QStringLiteral("--accept-tos"), QStringLiteral("--no-paginate")} + args);
}

void WarpCli::runJson(const QString &requestId, const QStringList &args) {
    run(requestId, QStringList{QStringLiteral("-j")} + args);
}

void WarpCli::startProcess(const QString &requestId, const QStringList &args) {
    if (isRunning(requestId)) {
        return;
    }

    auto *proc = new QProcess(this);
    m_running.insert(requestId, proc);

    proc->setProgram(QStringLiteral("warp-cli"));
    proc->setArguments(args);

    connect(proc, &QProcess::finished, this, [this, requestId, proc](int exitCode, QProcess::ExitStatus) {
        WarpResult result;
        result.exitCode = exitCode;
        result.stdoutText = QString::fromUtf8(proc->readAllStandardOutput());
        result.stderrText = QString::fromUtf8(proc->readAllStandardError());

        m_running.remove(requestId);
        emit finished(requestId, result);

        proc->deleteLater();
    });

    proc->start();
}
