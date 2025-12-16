#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>

class QProcess;

struct WarpResult {
    int exitCode;
    QString stdoutText;
    QString stderrText;
};

class WarpCli : public QObject {
    Q_OBJECT

public:
    explicit WarpCli(QObject *parent = nullptr);

    bool isRunning(const QString &requestId) const;

    void run(const QString &requestId, const QStringList &args);
    void runJson(const QString &requestId, const QStringList &args);

signals:
    void finished(const QString &requestId, const WarpResult &result);

private:
    void startProcess(const QString &requestId, const QStringList &args);

    QHash<QString, QProcess *> m_running;
};
