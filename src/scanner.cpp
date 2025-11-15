#include "scanner.h"

#include "json_utils.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

// Directory base dei documenti reMarkable
static const QString kBase =
    QStringLiteral("/home/root/.local/share/remarkable/xochitl");

bool scanDocuments(QList<DocEntry>& pdfs,
                   QList<DocEntry>& epubs,
                   QList<DocEntry>& notebooks,
                   ScanStats& stats)
{
    QDir d(kBase);
    if (!d.exists()) {
        return false;
    }

    QStringList metas = d.entryList(QStringList() << "*.metadata",
                                    QDir::Files, QDir::Name);

    for (const QString& metaFile : metas) {
        // UUID = nome file senza estensione
        QString uuid = metaFile;
        uuid.chop(QStringLiteral(".metadata").size());

        const QString metaPath    = kBase + "/" + uuid + ".metadata";
        const QString contentPath = kBase + "/" + uuid + ".content";

        ++stats.metaCount;

        // 1) Leggi metadata (visibleName + parent/deleted)
        QJsonObject meta;
        if (!loadJsonObject(metaPath, meta))
            continue;

        const bool isDeleted = meta.value("deleted").toBool(false);
        if (isDeleted) {
            ++stats.deleted;
            continue;
        }

        // parent può essere stringa ("trash"/UUID) o bool(false)
        const QJsonValue parentV = meta.value("parent");
        QString parentStr;
        bool inTrash = false;

        if (parentV.isBool()) {
            // parent:false → root/My Files (nessuna cartella)
            parentStr.clear();
        } else if (parentV.isString()) {
            const QString p = parentV.toString();
            if (p == "trash") {
                inTrash = true;
            } else {
                parentStr = p; // UUID cartella
            }
        } else {
            parentStr.clear();
        }

        if (inTrash) {
            ++stats.trash;
            continue;
        }

        // 2) Leggi content (tipo + tag + page map)
        QJsonObject content;
        if (!loadJsonObject(contentPath, content)) {
            ++stats.contentMissing;
            continue;
        }

        QString fileType = readFileType(content);
        if (fileType.isEmpty()) {
            // Fallback di emergenza: deduci dal file presente sul FS
            const QString pdfPath  = kBase + "/" + uuid + ".pdf";
            const QString epubPath = kBase + "/" + uuid + ".epub";
            if (QFileInfo::exists(pdfPath))
                fileType = "pdf";
            else if (QFileInfo::exists(epubPath))
                fileType = "epub";
            else
                fileType = "notebook";

            ++stats.forcedType;
        }

        // 3) Mappa pageId → numero pagina
        const QHash<QString,int> pageMap = buildPageMap(content);

        // 4) Page tags (name + pageId + pageNumber)
        QList<TagRef> tags;
        const QJsonArray pageTags = readPageTags(content);
        tags.reserve(pageTags.size());
        QSet<QString> seenPairs; // dedup per (name|pageId)

        for (const auto& v : pageTags) {
            const QJsonObject t = v.toObject();
            const QString name = t.value("name").toString().trimmed();
            const QString pid  = t.value("pageId").toString().trimmed();
            if (name.isEmpty() || pid.isEmpty())
                continue;

            const QString key = name + "|" + pid;
            if (seenPairs.contains(key))
                continue;
            seenPairs.insert(key);

            TagRef r;
            r.name = name;
            r.pageId = pid;
            r.pageNumber = pageMap.value(pid, -1); // -1 se non trovato
            tags.append(r);
        }

        // 5) Costruisci entry
        DocEntry e;
        e.uuid         = uuid;
        e.visibleName  = meta.value("visibleName").toString(uuid).trimmed();
        e.parentUuid   = parentStr;
        e.hasParent    = !e.parentUuid.isEmpty();
        e.kind         = fileType;      // "pdf" | "epub" | "notebook"
        e.tags         = tags;
        e.hasTags      = !e.tags.isEmpty();
       // Numero di pagine dal .content (se presente)
        int pageCount = content.value("pageCount").toInt(0);
        e.pages = pageCount;

        // 6) Smista
        if (fileType == "pdf")            pdfs.append(e);
        else if (fileType == "epub")      epubs.append(e);
        else if (fileType == "notebook")  notebooks.append(e);
        else /* ignora altri tipi */      (void)0;
    }

    return true;
}