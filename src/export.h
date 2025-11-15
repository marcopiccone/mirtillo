#pragma once

#include "model.h"
#include <QTextStream>

// Stampa il summary a video
void printDocumentSummary(const DocEntry &doc, QTextStream &out);

// Esporta il summary in un file di testo sul Paper Pro
void exportDocument(const DocEntry &doc, QTextStream &out);