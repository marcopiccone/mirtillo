#include "logging.h"

#include <QLoggingCategory>
#include <QByteArray>
#include <cstdio>

// Silenzia i warning specifici sul locale ANSI_X3.4-1968 / UTF-8 mancanti
void mirtilloMessageHandler(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg)
{
    Q_UNUSED(context);

    if (type == QtWarningMsg) {
        if (msg.contains("ANSI_X3.4-1968", Qt::CaseInsensitive) ||
            msg.contains("Qt depends on a UTF-8 locale", Qt::CaseInsensitive)) {
            // Ignora completamente questo warning
            return;
        }
    }

    // Tutto il resto: stampa normale su stderr
    QByteArray local = msg.toLocal8Bit();
    std::fprintf(stderr, "%s\n", local.constData());
}

// Piccolo header con gatto ASCII + versione
QString mirtilloHeader(const QString &version)
{
    QString line1 = " /\\_/\\    mirtillo v" + version;
    QString line2 = "( o.o )   Paper Pro CLI tool";
    QString line3 = " > ^ <    meta/tag extractor";
    return line1 + "\n" + line2 + "\n" + line3;
}