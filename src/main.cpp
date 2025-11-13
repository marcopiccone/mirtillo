// ========================
//  mirtillo — main.cpp
//  CLI per Paper Pro: elenca PDF/EPUB/Notebook e mostra i tag per-pagina
// ========================

// ------------------------
//  Qt Core includes
// ------------------------
// Gestisce il ciclo principale (anche per programmi console)
#include <QCoreApplication>
// I/O testuale su console (Unicode)
#include <QTextStream>
// Filesystem
#include <QDir>
#include <QFileInfo>
#include <QFile>
// JSON
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
// Warning handler
#include <QtGlobal>        // per qInstallMessageHandler
#include <QLoggingCategory>
#include <cstdio>          // per fprintf

// ------------------------
//  Standard / Qt utils
// ------------------------
#include <algorithm>     // std::sort
#include <clocale>       // setlocale()
#include <QLocale>       // QLocale::setDefault()
#include <QByteArray>    // qputenv arg type
#include <QStringList>
#include <QList>
#include <QHash>
#include <QSet>

// ------------------------

struct TagRef {
    QString name;
    QString pageId;
    int     pageNumber = -1; // -1 se non determinabile (verrà stampato "?")
};

struct DocEntry {
    QString uuid;
    QString visibleName;
    QString parentUuid;        // "" = root/My Files
    bool    hasParent = false;
    QString kind;              // "pdf" | "epub" | "notebook"
    QList<TagRef> tags;        // tag per-pagina
    int     pages = 0;         // placeholder
    bool    hasTags = false;
};

static const QString kBase = QStringLiteral("/home/root/.local/share/remarkable/xochitl");

// Carica un file JSON come QJsonObject
static bool loadJsonObject(const QString& path, QJsonObject& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    out = doc.object();
    return true;
}

// Legge il tipo di file con fallback: extraMetaData.fileType → root.fileType → ""
static QString readFileType(const QJsonObject& content) {
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QString t = extra.value("fileType").toString().trimmed();  // "pdf"|"epub"|"notebook"
    if (!t.isEmpty()) return t;
    t = content.value("fileType").toString().trimmed();
    return t; // può essere vuoto: verrà gestito a valle
}

// Restituisce i pageTags: extraMetaData.pageTags → root.pageTags
static QJsonArray readPageTags(const QJsonObject& content) {
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QJsonArray a = extra.value("pageTags").toArray();
    if (!a.isEmpty()) return a;
    return content.value("pageTags").toArray();
}

// Costruisce una mappa pageId → pageNumber da cPages.pages[].redir.value (con fallback root.pages)
static QHash<QString,int> buildPageMap(const QJsonObject& content) {
    QHash<QString,int> map;

    auto parsePagesArray = [&](const QJsonArray& pages){
        for (const auto& v : pages) {
            const QJsonObject p = v.toObject();
            const QString pid = p.value("id").toString().trimmed();
            int pageNo = -1;
            const QJsonObject redir = p.value("redir").toObject();
            if (!redir.isEmpty()) pageNo = redir.value("value").toInt(-1);
            if (!pid.isEmpty() && pageNo >= 0) map.insert(pid, pageNo);
        }
    };

    // 1) cPages.pages
    const QJsonObject cPages = content.value("cPages").toObject();
    QJsonArray pages = cPages.value("pages").toArray();
    if (!pages.isEmpty()) parsePagesArray(pages);

    // 2) fallback: root-level "pages"
    if (map.isEmpty()) {
        pages = content.value("pages").toArray();
        if (!pages.isEmpty()) parsePagesArray(pages);
    }
    return map;
}

// Message handler personalizzato: silenzia solo i warning di qt.core.locale
void mirtilloMessageHandler(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg){
    Q_UNUSED(context);

    // Filtra via i warning sul locale ANSI_X3.4-1968 / UTF-8 mancanti
    if (type == QtWarningMsg) {
        if (msg.contains("ANSI_X3.4-1968", Qt::CaseInsensitive) ||
            msg.contains("Qt depends on a UTF-8 locale", Qt::CaseInsensitive)) {
            return; // ignora completamente questo warning
        }
    }

    // Tutto il resto lo scriviamo su stderr normalmente
    QByteArray local = msg.toLocal8Bit();
    fprintf(stderr, "%s\n", local.constData());
}

int main(int argc, char *argv[]) {
    
    // 0) Installa il message handler per silenziare qt.core.locale
    qInstallMessageHandler(mirtilloMessageHandler);

    // 1) Prova comunque a forzare UTF-8 (innocuo su Paper Pro)
    setlocale(LC_ALL, "C.UTF-8");
    qputenv("LANG",     QByteArray("C.UTF-8"));
    qputenv("LC_ALL",   QByteArray("C.UTF-8"));
    qputenv("LC_CTYPE", QByteArray("C.UTF-8"));
    QLocale::setDefault(QLocale::c());

    QCoreApplication app(argc, argv);
    QTextStream out(stdout), in(stdin);

    // Gestione opzioni --version / --about / --debug
    bool debug = false;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromUtf8(argv[i]).trimmed();

        if (arg == "--version") {
            out << "mirtillo v" << MIRTILLO_VERSION << " (Paper Pro CLI)\n";
            return 0;
        }

        if (arg == "--about") {
            QString path;

            // Override da variabile d'ambiente (utile in VM)
            if (qEnvironmentVariableIsSet("MIRTILLO_ABOUT_PATH")) {
                path = qEnvironmentVariable("MIRTILLO_ABOUT_PATH");
            } else {
                // Path runtime sul dispositivo
                path = QStringLiteral("/home/root/.local/share/mirtillo/ABOUT.txt");
            }

            QFile f(path);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                out << "Error: could not open ABOUT file: " << path << "\n";
                return 1;
            }

            out << f.readAll() << "\n";
            return 0;
        }

        if (arg == "--debug") {
            debug = true;
        }
    }

    out << "mirtillo v" << MIRTILLO_VERSION << " (Paper Pro CLI)\n";
    out << "----------------------------------------------\n";
    out.flush();

    for (int i = 1; i < argc; ++i) {
        if (QString::fromUtf8(argv[i]) == "--debug") { debug = true; break; }
    }
    int cntMeta = 0, cntContentMissing = 0, cntForcedType = 0, cntTrash = 0, cntDeleted = 0;

    QDir d(kBase);
    if (!d.exists()) {
        out << "Directory not found: " << kBase << "\n";
        return 1;
    }

    QStringList metas = d.entryList(QStringList() << "*.metadata", QDir::Files, QDir::Name);
    QList<DocEntry> pdfs, epubs, notebooks;

    for (const QString& metaFile : metas) {
        // UUID
        QString uuid = metaFile;
        uuid.chop(QStringLiteral(".metadata").size());
        const QString metaPath    = kBase + "/" + uuid + ".metadata";
        const QString contentPath = kBase + "/" + uuid + ".content";

        ++cntMeta;

        // Leggi metadata (visibleName + parent/deleted)
        QJsonObject meta;
        if (!loadJsonObject(metaPath, meta)) continue;

        const bool isDeleted = meta.value("deleted").toBool(false);
        if (isDeleted) { ++cntDeleted; continue; }

        // parent può essere stringa ("trash"/UUID) o bool(false)
        const QJsonValue parentV = meta.value("parent");
        QString parentStr;
        bool inTrash = false;
        if (parentV.isBool()) {
            // parent:false → root/My Files (nessuna cartella)
            parentStr.clear();
        } else if (parentV.isString()) {
            const QString p = parentV.toString();
            if (p == "trash") inTrash = true;
            else parentStr = p; // UUID cartella
        } else {
            parentStr.clear();
        }
        if (inTrash) { ++cntTrash; continue; }

        // Leggi content (tipo + tag + page map)
        QJsonObject content;
        if (!loadJsonObject(contentPath, content)) { ++cntContentMissing; continue; }

        QString fileType = readFileType(content);
        if (fileType.isEmpty()) {
            // Fallback di emergenza: deduci dal file presente sul FS
            const QString pdfPath  = kBase + "/" + uuid + ".pdf";
            const QString epubPath = kBase + "/" + uuid + ".epub";
            if (QFileInfo::exists(pdfPath))       fileType = "pdf";
            else if (QFileInfo::exists(epubPath)) fileType = "epub";
            else                                   fileType = "notebook";
            ++cntForcedType;
        }

        // Mappa pageId → numero pagina
        const QHash<QString,int> pageMap = buildPageMap(content);

        // Page tags (name + pageId + pageNumber)
        QList<TagRef> tags;
        const QJsonArray pageTags = readPageTags(content);
        tags.reserve(pageTags.size());
        QSet<QString> seenPairs; // dedup per (name|pageId)
        for (const auto& v : pageTags) {
            const QJsonObject t = v.toObject();
            const QString name = t.value("name").toString().trimmed();
            const QString pid  = t.value("pageId").toString().trimmed();
            if (name.isEmpty() || pid.isEmpty()) continue;

            const QString key = name + "|" + pid;
            if (seenPairs.contains(key)) continue;
            seenPairs.insert(key);

            TagRef r;
            r.name = name;
            r.pageId = pid;
            r.pageNumber = pageMap.value(pid, -1); // -1 se non trovato
            tags.append(r);
        }

        // Costruisci entry
        DocEntry e;
        e.uuid = uuid;
        e.visibleName = meta.value("visibleName").toString(uuid).trimmed();
        e.parentUuid = parentStr;
        e.hasParent  = !e.parentUuid.isEmpty();
        e.kind = fileType;      // "pdf" | "epub" | "notebook"
        e.tags = tags;
        e.hasTags = !e.tags.isEmpty();
        e.pages   = 0;

        // Smista
        if (fileType == "pdf")            pdfs.append(e);
        else if (fileType == "epub")      epubs.append(e);
        else if (fileType == "notebook")  notebooks.append(e);
        else /* ignora altri tipi */      (void)0;
    }

    // Ordinamento alfabetico per visibleName
    auto byName = [](const DocEntry& a, const DocEntry& b){
        return QString::localeAwareCompare(a.visibleName, b.visibleName) < 0;
    };
    std::sort(pdfs.begin(),      pdfs.end(),      byName);
    std::sort(epubs.begin(),     epubs.end(),     byName);
    std::sort(notebooks.begin(), notebooks.end(), byName);

    // Menu tipo documento
    out << "\nDocument type to list? [p] PDF  [e] EPUB  [n] notebook  [q] quit: ";
    out.flush();
    QString choice = in.readLine().trimmed().toLower();
    const QList<DocEntry>* list = nullptr;
    if (choice == "p")      list = &pdfs;
    else if (choice == "e") list = &epubs;
    else if (choice == "n") list = &notebooks;
    else return 0;

    if (list->isEmpty()) {
        out << "No documents found for the selected type.\n";
        if (debug) {
            out << "[debug] scanned: " << cntMeta
                << " | missing .content: " << cntContentMissing
                << " | forced fileType: " << cntForcedType
                << " | deleted: " << cntDeleted
                << " | trash: " << cntTrash << "\n";
        }
        return 0;
    }

    // Stampa 2 colonne con indice
    auto printList = [&](const QList<DocEntry>& L) {
        const int n = L.size();
        const int colw = 48; // larghezza colonna
        for (int i = 0; i < n; ) {
            QString left = QString("%1) %2")
                               .arg(i+1, 3, 10, QChar(' '))
                               .arg(L[i].visibleName.left(colw-7));
            QString right;
            if (i+1 < n) {
                right = QString("%1) %2")
                            .arg(i+2, 3, 10, QChar(' '))
                            .arg(L[i+1].visibleName.left(colw-7));
            }
            out << left.leftJustified(colw) << "  " << right << "\n";
            i += 2;
        }
    };

    out << "\nFound " << list->size() << " "
        << (list==&pdfs ? "PDF" : list==&epubs ? "EPUB" : "notebook")
        << " document(s):\n\n";
    printList(*list);

    out << "\nSelect a number (1-" << list->size() << ") or 0 to exit: ";
    out.flush();
    bool ok = false;
    int sel = in.readLine().trimmed().toInt(&ok);
    if (!ok || sel <= 0 || sel > list->size()) return 0;

    const DocEntry& pick = list->at(sel-1);
    out << "\nTitle : " << pick.visibleName << "\n";
    out << "UUID  : " << pick.uuid << "\n";
    out << "Parent: " << (pick.hasParent ? pick.parentUuid : "false") << "\n";

    // Stampa dettagli tag (name, pagina, pageId)
    if (!pick.hasTags) {
        out << "Tags  : (none)\n";
    } else {
        out << "Tags:\n";
        int idx = 1;
        for (const TagRef& t : pick.tags) {
            // Mostra pagina in 1-based se disponibile
            const QString pageStr = (t.pageNumber >= 0)
                                    ? QString::number(t.pageNumber + 1)
                                    : QStringLiteral("?");
            out << "  " << idx++ << ". " << t.name
                << ", pag. " << pageStr
                << " (UUID: " << t.pageId << ")\n";
        }
    }

    if (debug) {
        out << "\n[debug] scanned *.metadata: " << cntMeta
            << " | missing .content: " << cntContentMissing
            << " | forced fileType: " << cntForcedType
            << " | deleted: " << cntDeleted
            << " | trash: " << cntTrash << "\n";
    }
    return 0;
}