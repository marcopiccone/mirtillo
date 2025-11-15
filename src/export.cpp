#include "export.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QMap>

// Percorso base per i file di mirtillo sul Paper Pro
static const QString kShareBase =
    QStringLiteral("/home/root/.local/share/mirtillo");

// -------------------------
//  Summary a video
// -------------------------
void printDocumentSummary(const DocEntry &doc, QTextStream &out)
{
    out << "=== Document summary =====================================\n";
    out << "Title     : " << doc.visibleName << "\n";
    out << "UUID      : " << doc.uuid << "\n";
    out << "Type      : " << doc.kind << "\n";

    if (doc.pages > 0)
        out << "Pages     : " << doc.pages << "\n";
    else
        out << "Pages     : (unknown)\n";

    out << "Tag count : " << doc.tags.size() << "\n";
    out << "Tags by name:\n";

    if (doc.tags.isEmpty()) {
        out << "  (none)\n";
        out << "==========================================================\n";
        return;
    }

    // Raggruppa per nome tag (QMap = ordinato per chiave)
    QMap<QString, QList<const TagRef*>> byName;
    for (const TagRef &t : doc.tags) {
        byName[t.name].append(&t);
    }

    for (auto it = byName.cbegin(); it != byName.cend(); ++it) {
        const QString &name = it.key();
        const QList<const TagRef*> &entries = it.value();

        out << "  - \"" << name << "\"\n";

        for (const TagRef *tr : entries) {
            QString pageStr = (tr->pageNumber >= 0)
                              ? QString::number(tr->pageNumber + 1)
                              : QStringLiteral("?");
            out << "      • Page " << pageStr
                << " (" << tr->pageId << ")\n";
        }
    }

    out << "==========================================================\n";
}

// -------------------------
//  Export su file
// -------------------------
void exportDocument(const DocEntry &doc, QTextStream &out)
{
    QDir dir(kShareBase);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            out << "Error: cannot create directory: " << kShareBase << "\n";
            return;
        }
    }

    const QString fileName =
        QStringLiteral("summary_%1.txt").arg(doc.uuid);

    const QString fullPath = dir.filePath(fileName);

    QFile f(fullPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        out << "Error: cannot write summary file: " << fullPath << "\n";
        return;
    }

    QTextStream s(&f);
    // riusa esattamente lo stesso layout del summary a video
    printDocumentSummary(doc, s);
    s.flush();

    out << "Summary exported to: " << fullPath << "\n";
}