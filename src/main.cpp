// ========================
//  mirtillo — main.cpp
//  CLI per Paper Pro: elenca PDF/EPUB/Notebook e mostra i tag per-pagina
// ========================

#include <QCoreApplication>
#include <QTextStream>
#include <QLocale>
#include <QByteArray>
#include <QFile>
#include <QIODevice>

#include <clocale>       // setlocale
#include <algorithm>     // std::sort

#include "logging.h"
#include "model.h"
#include "scanner.h"
#include "export.h"

int main(int argc, char *argv[])
{
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
            out << "\n";
            out << mirtilloHeader(QStringLiteral(MIRTILLO_VERSION)) << "\n\n";
            out.flush();
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

    // 2) Scansione documenti
    QList<DocEntry> pdfs, epubs, notebooks;
    ScanStats stats;

    if (!scanDocuments(pdfs, epubs, notebooks, stats)) {
        out << "Directory not found: "
            << "/home/root/.local/share/remarkable/xochitl"
            << "\n";
        return 1;
    }

    // Ordinamento alfabetico per visibleName
    auto byName = [](const DocEntry& a, const DocEntry& b) {
        return QString::localeAwareCompare(a.visibleName, b.visibleName) < 0;
    };
    std::sort(pdfs.begin(),      pdfs.end(),      byName);
    std::sort(epubs.begin(),     epubs.end(),     byName);
    std::sort(notebooks.begin(), notebooks.end(), byName);

    // -----------------------------
    // Loop principale del menu
    // -----------------------------
    while (true) {
        // 1) Chiedi il tipo di documento
        out << "\nDocument type to list? [p] PDF  [e] EPUB  [n] notebook  [q] quit: ";
        out.flush();

        QString choice = in.readLine().trimmed().toLower();
        if (choice.isEmpty()) {
            // EOF o solo invio → esci
            break;
        }
        if (choice == "q") {
            break;
        }

        const QList<DocEntry>* list = nullptr;
        QString kindLabel;

        if (choice == "p") {
            list = &pdfs;
            kindLabel = "PDF";
        } else if (choice == "e") {
            list = &epubs;
            kindLabel = "EPUB";
        } else if (choice == "n") {
            list = &notebooks;
            kindLabel = "notebook";
        } else {
            out << "Unknown choice. Please use p/e/n/q.\n";
            continue;
        }

        if (list->isEmpty()) {
            out << "No " << kindLabel << " documents found.\n";
            continue;
        }

        // 2) Stampa la lista in 2 colonne
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

        out << "\nFound " << list->size() << " " << kindLabel << " document(s):\n\n";
        printList(*list);

        // 3) Chiedi quale documento aprire
        out << "\nSelect a number (1-" << list->size()
            << ") or 0 to go back to main menu: ";
        out.flush();

        bool ok = false;
        int sel = in.readLine().trimmed().toInt(&ok);
        if (!ok || sel < 0 || sel > list->size()) {
            out << "Invalid selection.\n";
            continue;
        }
        if (sel == 0) {
            // Torna al menu principale
            continue;
        }

        const DocEntry& pick = list->at(sel-1);

        // 4) Mostra i dettagli del documento
        printDocumentSummary(pick, out);

        // 5) Chiedi cosa fare dopo: export / torna al menu / quit
        while (true) {
            out << "\n[e] export  [m] main menu  [q] quit: ";
            out.flush();
            QString action = in.readLine().trimmed().toLower();

            if (action == "e") {
                exportDocument(pick, out);
                // dopo l'export torniamo al menu principale
                break;
            } else if (action == "m" || action.isEmpty()) {
                // torna al menu principale
                break;
            } else if (action == "q") {
                // esci dall'intero programma
                if (debug) {
                    out << "\n[debug] scanned *.metadata: " << stats.metaCount
                        << " | missing .content: " << stats.contentMissing
                        << " | forced fileType: " << stats.forcedType
                        << " | deleted: " << stats.deleted
                        << " | trash: " << stats.trash << "\n";
                }
                return 0;
            } else {
                out << "Unknown choice. Use e/m/q.\n";
            }
        }
    }

    if (debug) {
        out << "\n[debug] scanned *.metadata: " << stats.metaCount
            << " | missing .content: " << stats.contentMissing
            << " | forced fileType: " << stats.forcedType
            << " | deleted: " << stats.deleted
            << " | trash: " << stats.trash << "\n";
    }

    return 0;
}