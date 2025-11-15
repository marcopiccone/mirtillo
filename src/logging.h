#pragma once

#include <QtGlobal>
#include <QString>

// Message handler personalizzato: silenzia solo i warning di qt.core.locale
void mirtilloMessageHandler(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg);

// Header ASCII con "logo" e versione
QString mirtilloHeader(const QString &version);