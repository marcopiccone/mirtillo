# mirtillo — Developer Guide
*A technical guide to the internal architecture of mirtillo.*

---

## 1. Introduction

This document explains the **internal structure**, **architecture**, and **design decisions** of `mirtillo`, a metadata-driven CLI tool for the reMarkable Paper Pro.

It is intended for developers who want to:

- extend the CLI,
- add new commands (e.g. `--export-json`, `--export-pdf`),
- integrate GUI modules (Qt/QML),
- or contribute to the roadmap.

---

## 2. Code Architecture Overview

The core program currently lives in a single translation unit:

```text
src/main.cpp
```

It is organized into these logical layers:

1. **Environment setup & logging**
2. **Data structures**
3. **JSON parsing utilities**
4. **Main document-scanning and user interaction loop**

The design is constrained by reMarkable OS:

- read-only root filesystem
- limited memory / storage
- QtCore-only (console app, no GUI)
- Qt 6.5 runtime
- BusyBox shell and reduced POSIX userspace

---

## 3. Data Structures

### 3.1 `TagRef`

Represents a single tag associated with a specific page:

```cpp
struct TagRef {
    QString name;
    QString pageId;
    int     pageNumber = -1; // -1 if unknown (printed as "?")
};
```

- `name` — user-visible label of the tag.
- `pageId` — UUID of the page in the document.
- `pageNumber` — resolved via the page map (0-based); printed as 1-based in the UI.

---

### 3.2 `DocEntry`

Represents a single document as seen by `mirtillo`:

```cpp
struct DocEntry {
    QString uuid;
    QString visibleName;
    QString parentUuid;        // empty = root / My Files
    bool    hasParent = false;
    QString kind;              // "pdf" | "epub" | "notebook"
    QList<TagRef> tags;        // per-page tags
    int     pages = 0;         // placeholder for future use
    bool    hasTags = false;
};
```

The `kind` field is determined from the corresponding `.content` JSON, with fallback to filesystem extensions.

---

## 4. JSON Parsing Layer

reMarkable stores document-related data as a cluster of files:

```text
UUID.metadata
UUID.content
UUID.pdf      (for PDFs)
UUID.epub     (for EPUBs)
UUID.thumbnails/...
```

`mirtillo` reads:

- `.metadata` → high-level metadata (title, parent, deletion flags)
- `.content`  → rich metadata (file type, page tags, page mappings)

The base directory is:

```cpp
static const QString kBase =
    QStringLiteral("/home/root/.local/share/remarkable/xochitl");
```

---

### 4.1 `loadJsonObject()`

```cpp
static bool loadJsonObject(const QString& path, QJsonObject& out) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    out = doc.object();
    return true;
}
```

A small utility that abstracts away common boilerplate:

- opens a file
- parses JSON
- returns a `QJsonObject` if successful

Used for both `*.metadata` and `*.content`.

---

### 4.2 `readFileType()`

```cpp
static QString readFileType(const QJsonObject& content) {
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QString t = extra.value("fileType").toString().trimmed();  // "pdf"|"epub"|"notebook"
    if (!t.isEmpty()) return t;
    t = content.value("fileType").toString().trimmed();
    return t; // may be empty, handled later
}
```

Priority:

1. `extraMetaData.fileType`
2. fallback `fileType` at the root level
3. final fallback done later via filesystem inspection:
   - `.pdf` → `"pdf"`
   - `.epub` → `"epub"`
   - default → `"notebook"`

This makes the code robust to different firmware versions / formats.

---

### 4.3 `readPageTags()`

```cpp
static QJsonArray readPageTags(const QJsonObject& content) {
    const QJsonObject extra = content.value("extraMetaData").toObject();
    QJsonArray a = extra.value("pageTags").toArray();
    if (!a.isEmpty()) return a;
    return content.value("pageTags").toArray();
}
```

Priority:

1. `extraMetaData.pageTags`
2. fallback `pageTags` at the root level

This keeps compatibility if reMarkable changes where tags are stored inside `.content`.

Each element in the resulting array is expected to be an object like:

```json
{
  "name": "Ippolito, Confut.",
  "pageId": "6667662b-ff4a-4e25-bf9d-b719a36c26f3",
  "timestamp": 1737280945574
}
```

---

### 4.4 `buildPageMap()`

```cpp
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
```

This builds a map:

```text
pageId → pageNumber (0-based)
```

The page number is then converted to **1-based** for display.

Fallbacks ensure that:

- if `cPages` is present, it is used
- otherwise, a simpler root-level `pages` array is used.

---

## 5. Message Handling & Locale Filtering

On firmware 3.23+, Qt emits a warning like:

```text
Detected locale "C" with character encoding "ANSI_X3.4-1968", which is not UTF-8.
Qt depends on a UTF-8 locale, and has switched to "C.UTF-8" instead.
```

This is harmless but noisy. The device cannot load a true UTF-8 locale, so `mirtillo` installs a custom message handler:

```cpp
void mirtilloMessageHandler(QtMsgType type,
                            const QMessageLogContext &context,
                            const QString &msg) {
    Q_UNUSED(context);

    if (type == QtWarningMsg) {
        if (msg.contains("ANSI_X3.4-1968", Qt::CaseInsensitive) ||
            msg.contains("Qt depends on a UTF-8 locale", Qt::CaseInsensitive)) {
            return; // ignore locale warnings entirely
        }
    }

    QByteArray local = msg.toLocal8Bit();
    fprintf(stderr, "%s
", local.constData());
}
```

And in `main()`:

```cpp
qInstallMessageHandler(mirtilloMessageHandler);
```

All other Qt warnings and messages are still printed to `stderr`.

---

## 6. Environment Setup in `main()`

At the top of `main()`:

```cpp
qInstallMessageHandler(mirtilloMessageHandler);

setlocale(LC_ALL, "C.UTF-8");
qputenv("LANG",     QByteArray("C.UTF-8"));
qputenv("LC_ALL",   QByteArray("C.UTF-8"));
qputenv("LC_CTYPE", QByteArray("C.UTF-8"));
QLocale::setDefault(QLocale::c());

QCoreApplication app(argc, argv);
QTextStream out(stdout), in(stdin);
```

Even if `C.UTF-8` is not actually available on the device, these calls are harmless. The message handler ensures that Qt does not flood output with locale warnings.

---

## 7. Command-Line Options

The program currently supports:

- `--version`
- `--about`
- `--debug`

### 7.1 `--version`

```cpp
if (arg == "--version") {
    out << "mirtillo v" << MIRTILLO_VERSION << " (Paper Pro CLI)
";
    return 0;
}
```

`MIRTILLO_VERSION` is injected via CMake:

```cmake
project(mirtillo VERSION 0.1 LANGUAGES CXX)

target_compile_definitions(mirtillo PRIVATE
    MIRTILLO_VERSION="${PROJECT_VERSION}"
)
```

---

### 7.2 `--about`

```cpp
if (arg == "--about") {
    QString path;

    if (qEnvironmentVariableIsSet("MIRTILLO_ABOUT_PATH")) {
        path = qEnvironmentVariable("MIRTILLO_ABOUT_PATH");
    } else {
        path = QStringLiteral("/home/root/.local/share/mirtillo/ABOUT.txt");
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        out << "Error: could not open ABOUT file: " << path << "
";
        return 1;
    }

    out << f.readAll() << "
";
    return 0;
}
```

This allows:

- developer override via `MIRTILLO_ABOUT_PATH` (e.g. inside the SDK/VM),
- runtime path on the device (`/home/root/.local/share/mirtillo/ABOUT.txt`).

---

### 7.3 `--debug`

A simple boolean flag:

```cpp
if (arg == "--debug") {
    debug = true;
}
```

Later, if no documents of a given type are found:

```cpp
if (list->isEmpty()) {
    out << "No documents found for the selected type.
";
    if (debug) {
        out << "[debug] scanned: " << cntMeta
            << " | missing .content: " << cntContentMissing
            << " | forced fileType: " << cntForcedType
            << " | deleted: " << cntDeleted
            << " | trash: " << cntTrash << "
";
    }
    return 0;
}
```

At the end of a successful run, debug information is also printed.

---

## 8. Document Scan and Classification

The main loop enumerates `.metadata` files:

```cpp
QStringList metas = d.entryList(QStringList() << "*.metadata",
                                QDir::Files, QDir::Name);
QList<DocEntry> pdfs, epubs, notebooks;
```

For each:

1. Derive UUID from filename.
2. Load `.metadata`.
3. Skip deleted documents.
4. Check `parent`:
   - `parent: "trash"` → skip
   - `parent: false`   → root / My Files
   - `parent: "<uuid>"` → inside a folder
5. Load `.content`.
6. Determine file type.
7. Build page map.
8. Extract `pageTags` → build `TagRef` list (with deduplication).
9. Fill `DocEntry` and append to `pdfs` / `epubs` / `notebooks`.

Tags are deduplicated by `(name|pageId)`:

```cpp
QSet<QString> seenPairs;
for (const auto& v : pageTags) {
    ...
    const QString key = name + "|" + pid;
    if (seenPairs.contains(key)) continue;
    seenPairs.insert(key);
    ...
}
```

This avoids duplicated tags in case multiple JSON entries map to the same semantic tag/page combination.

---

## 9. Sorting and Listing

Documents are sorted alphabetically:

```cpp
auto byName = [](const DocEntry& a, const DocEntry& b){
    return QString::localeAwareCompare(a.visibleName, b.visibleName) < 0;
};

std::sort(pdfs.begin(),      pdfs.end(),      byName);
std::sort(epubs.begin(),     epubs.end(),     byName);
std::sort(notebooks.begin(), notebooks.end(), byName);
```

The CLI then asks which document type to list:

```cpp
out << "
Document type to list? [p] PDF  [e] EPUB  [n] notebook  [q] quit: ";
...
```

The chosen list is displayed in two columns:

```cpp
auto printList = [&](const QList<DocEntry>& L) {
    const int n = L.size();
    const int colw = 48;
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
        out << left.leftJustified(colw) << "  " << right << "
";
        i += 2;
    }
};
```

The user then selects a document index, and details are shown:

- Title
- UUID
- Parent UUID (or `"false"` for root)
- Tags: name, page number (1-based or `?`), page UUID.

---

## 10. Extending the Codebase

### 10.1 Adding JSON export

A natural next step is:

- Add an option `--export-json <path>` that dumps all collected `DocEntry` structures into a JSON file, including tags and page numbers.

You might:

- Introduce a helper `toJson(const DocEntry&)`,
- Add a small export loop after the scan,
- Use `QJsonArray` / `QJsonObject` / `QJsonDocument` to write to disk.

---

### 10.2 Adding PDF summary generation

For future work:

- Gather annotations, highlights, and thumbnails.
- Generate a summary PDF that collects:
  - document title
  - tag index
  - per-tag/per-page lists
  - (optional) embedded thumbnails

This can be produced:

- on-device (if a PDF library is available),
- or off-device by exporting structured JSON and using tools like `pandoc`, LaTeX, or a custom PDF generator.

---

### 10.3 Adding a GUI (Qt Quick / QML)

Because the reMarkable already ships Qt Quick:

- `mirtillo` could grow a QML-based “Tag Index Viewer”.
- The CLI backend can be refactored into a library (`libmirtillo-core`), then reused in:
  - CLI,
  - on-device GUI,
  - companion tools.

A possible future layout:

```text
src/
 ├── main.cpp              # CLI entry point
 ├── core/
 │    ├── scanner.cpp
 │    ├── scanner.hpp
 │    └── model.hpp
 ├── gui/
 │    ├── main_qml.cpp
 │    └── TagViewer.qml
```

---

## 11. Testing Locally (QEMU)

If your SDK environment is loaded, you can test the binary under QEMU *before* deploying:

```bash
$QEMU_LD_PREFIX/bin/qemu-aarch64   -L "$QEMU_LD_PREFIX"   ./mirtillo --version
```

This checks that:

- the binary is valid for aarch64,
- it links against the correct sysroot,
- it starts without crashing.

For more advanced tests, you can copy a snapshot of the `xochitl` directory into your VM and run `mirtillo` against it.

---

## 12. Troubleshooting

### 12.1 Locale / UTF‑8 warnings

If you still see messages like:

```text
Detected locale "C" with character encoding "ANSI_X3.4-1968"...
Qt depends on a UTF-8 locale...
```

Check that:

- `qInstallMessageHandler(mirtilloMessageHandler);` is at the very top of `main()`.
- No other module in your codebase installs a new message handler after mirtillo’s one.

You can also set:

```bash
export QT_LOGGING_RULES='qt.core.locale.warning=false'
```

in `~/.profile` as a belt-and-suspenders solution.

---

### 12.2 SDK environment not loaded

If CMake cannot find the cross-compiler or sysroot, you’ll see errors like:

```text
CMAKE_CXX_COMPILER not set, after EnableLanguage
Could not find toolchain file
```

Make sure you **always** run:

```bash
export RMSDK="<path-to-your-sdk>"
export QEMU_LD_PREFIX="$RMSDK/sysroots/cortexa53-crypto-remarkable-linux"
source "$RMSDK/environment-setup-cortexa53-crypto-remarkable-linux"
```

before running `cmake ..`.

---

### 12.3 SSH password prompts on every deploy

If `scp` or `ssh` keeps asking for a password, set up SSH keys:

```bash
ssh-keygen -t ed25519 -f ~/.ssh/rm_key
ssh-copy-id -i ~/.ssh/rm_key.pub root@10.11.99.1
```

Then configure your deploy script to use:

```bash
ssh -i "$HOME/.ssh/rm_key" ...
scp -i "$HOME/.ssh/rm_key" ...
```

---

### 12.4 Build directory mismatch

If CMake complains about an existing cache from another path, just reset:

```bash
rm -rf build/
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

---

## 13. Roadmap Summary

- [x] Metadata extraction
- [x] Per-document tag index
- [x] Page number resolution via `cPages`
- [x] Locale warning suppression
- [ ] JSON export of tag/index data
- [ ] PDF summary generator
- [ ] macOS companion app
- [ ] On-device GUI extension (Qt QML)

---

## 14. License and Contributions

`mirtillo` is released under the MIT License.

Contribution guidelines:

- Keep the code compatible with QtCore (no heavy GUI dependencies in the CLI).
- Avoid large external dependencies that would be hard to ship on reMarkable.
- Test on the actual device when possible.
- Try to preserve compatibility with future reMarkable firmware changes (avoid hardcoding assumptions that are likely to break).

Pull requests and design discussions are welcome.
