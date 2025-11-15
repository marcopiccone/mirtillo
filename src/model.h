#pragma once

#include <QString>
#include <QList>

// Rappresenta un singolo tag su una pagina specifica
struct TagRef {
    QString name;        // Nome del tag (es. "Ippolito, Confut.")
    QString pageId;      // UUID della pagina
    int     pageNumber = -1; // Numero pagina (0-based); -1 se non disponibile
};

// Rappresenta un documento (PDF / EPUB / notebook) nella libreria reMarkable
struct DocEntry {
    QString uuid;         // UUID del documento (basename dei file)
    QString visibleName;  // Titolo mostrato nell'interfaccia
    QString parentUuid;   // "" = root / My Files; altrimenti UUID cartella
    bool    hasParent = false;
    QString kind;         // "pdf" | "epub" | "notebook"
    QList<TagRef> tags;   // Elenco dei tag per-pagina
    int     pages = 0;    // Placeholder per futuro conteggio pagine
    bool    hasTags = false;
};

// Statistiche di scansione (utili in modalità --debug)
struct ScanStats {
    int metaCount       = 0; // Quanti *.metadata letti
    int contentMissing  = 0; // Quanti .content mancanti
    int forcedType      = 0; // Quanti fileType determinati via fallback
    int trash           = 0; // Quanti elementi in "trash"
    int deleted         = 0; // Quanti marcati "deleted"
};