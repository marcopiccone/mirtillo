#pragma once

#include <QList>
#include "model.h"

// Scansiona la libreria reMarkable in kBase e riempie le liste PDF/EPUB/notebook.
// Aggiorna anche le statistiche di scansione.
// Restituisce false solo se la directory base non esiste.
bool scanDocuments(QList<DocEntry>& pdfs,
                   QList<DocEntry>& epubs,
                   QList<DocEntry>& notebooks,
                   ScanStats& stats);