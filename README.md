# mirtillo  
*A metadata-driven extension for the reMarkable Paper Pro — generating per-document tag indexes, extracting highlights/notes, and exporting structured summaries.*

---

## 📘 Overview

**mirtillo** is a command-line tool that runs directly on the reMarkable Paper Pro.  
It parses document metadata, page UUIDs, tags, and page mappings from:

```text
/home/root/.local/share/remarkable/xochitl/
```

The goal is **not to replace `xochitl`**, but to complement it with advanced  
metadata-based features the official UI does not expose.

---

## 🎯 Project Goal

mirtillo aims to eventually provide:

- a **complete tag index per document**
- extraction of:
  - tagged pages
  - underlined passages
  - handwritten notes (when possible)
- generation of a **summary PDF** containing:
  - tag → page number → UUID mappings
  - extracted annotations
  - optional thumbnails
- automatic export to the host laptop

All without touching the rootfs (read-only on firmware 3.23+).

---

## 🌟 Current Features

- Lists all documents (PDF / EPUB / Notebook)
- Parses:
  - `.metadata`
  - `.content`
  - `cPages.pages` mappings
  - per-page tags (`pageTags`)
  - folder parent relationships (`parent` UUIDs)
- Shows tags with page numbers and UUIDs
- Works fully under `/home/root/.local/bin`
- Survives firmware 3.23+ updates using a post-update script

---

## 🔧 Required SDK Setup (IMPORTANT)

Before building mirtillo, you **must** load the reMarkable SDK environment.

From inside your Debian 12 VM:

```bash
export QEMU_LD_PREFIX="$HOME/rm-sdk/sysroots/cortexa53-crypto-remarkable-linux"
source "$HOME/rm-sdk/environment-setup-cortexa53-crypto-remarkable-linux"
```

This step:

- configures the cross-compiler  
- places the SDK version of `cmake` at the front of your `PATH`  
- configures the sysroot and linker  
- makes sure all builds are compiled for **aarch64-remarkable-linux**

You do **not** need to modify your `.bashrc`: just run these two lines in every new shell before building.

---

## 🛠️ Requirements (host side)

- macOS (Apple Silicon) or Linux  
- Debian 12 VM (if on macOS)  
- reMarkable official SDK installed in `~/rm-sdk`  
- SSH access to the Paper Pro over USB (`root@10.11.99.1`)

---

## 🏗️ Building mirtillo

```bash
# 1) Load SDK environment
export QEMU_LD_PREFIX="$HOME/rm-sdk/sysroots/cortexa53-crypto-remarkable-linux"
source "$HOME/rm-sdk/environment-setup-cortexa53-crypto-remarkable-linux"

# 2) Build
git clone https://github.com/<your-user>/mirtillo.git
cd mirtillo
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

The binary will be created as:

```text
build/mirtillo
```

---

## 🚀 Deploy to Paper Pro

### 1. First-time setup on the device

On the device, mirtillo installs and runs entirely inside the home directory (`/home/root`).  
A helper script can be used after each firmware update to recreate the local bin/lib dirs.

Copy the script and run it (once per firmware update):

```bash
scp scripts/post_setup_update.sh root@10.11.99.1:/home/root/post-update-mirtillo-setup.sh
ssh root@10.11.99.1 'sh ~/post-update-mirtillo-setup.sh || true'
```

This will:

- create `~/.local/bin` and `~/.local/lib` if missing  
- prepend `~/.local/bin` to `PATH`  
- set a minimal locale environment (`LANG=C`, `LC_ALL=C`)  
- silence noisy Qt locale warnings

### 2. Deploy the binary

From the VM, after building:

```bash
scp build/mirtillo root@10.11.99.1:/home/root/.local/bin/
```

### 3. Run on the device

```bash
ssh root@10.11.99.1
mirtillo
```

If `~/.profile` was updated by the post-update script, you may need to source it once:

```bash
. ~/.profile
mirtillo
```

---

## 📝 Example Output

```text
Document type to list? [p] PDF  [e] EPUB  [n] notebook:
p

Found 42 PDF document(s):

  1) Plato — Phaedrus                     2) Gospel of Thomas
  3) Confutatio Omnium Haeresium         4) The Complete Tolkien Companion
  …

Select a number: 3

Title : Confutatio Omnium Haeresium
UUID  : 6667662b-ff4a-4e25-bf9d-b719a36c26f3
Parent: <root>
Tags:
  1. Simon Mago, pag. 38 (UUID: bad5d945-fc7c-4a68-a522-290817f2413c)
  2. Ofiti, pag. 41 (UUID: 8bb36620-4b0f-4355-8d3b-6f4ac68a61b3)
```

---

## 📦 Repository Layout

```text
mirtillo/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── src/
│   └── main.cpp
├── scripts/
│   ├── post_setup_update.sh
│   └── deploy_to_paper_pro.sh
└── cmake/
    └── (reserved for future toolchain files, not required)
```

---

## 🧩 Development notes

This section documents the internal structure of the project for developers who want
to extend or modify `mirtillo`.

### Build system (CMake)

`mirtillo` uses a simple Qt 6 + CMake setup:

```cmake
cmake_minimum_required(VERSION 3.16)
project(mirtillo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(mirtillo
  src/main.cpp
)

target_link_libraries(mirtillo PRIVATE Qt6::Core)

target_compile_options(mirtillo PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(mirtillo PRIVATE -Os)
target_link_options(mirtillo PRIVATE -Wl,--as-needed)

set_target_properties(mirtillo PROPERTIES
  INSTALL_RPATH "\$ORIGIN/../lib"
)

include(GNUInstallDirs)
install(TARGETS mirtillo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
```

Key points:

- Only **QtCore** is required (console application, no GUI).
- The target is a single executable named `mirtillo`.
- Warnings are enabled (`-Wall -Wextra -Wpedantic`) and size is optimized (`-Os`).
- `INSTALL_RPATH` points to `../lib` relative to the binary, in case Qt libraries
  are bundled in the future.
- Deployment to the device is handled by shell scripts, not by CMake directly.

### Helper scripts

#### `scripts/deploy_to_paper_pro.sh`

```bash
#!/usr/bin/env bash
# Build and deploy mirtillo to reMarkable Paper Pro

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TARGET_BIN="mirtillo"
RM_HOST="root@10.11.99.1"
RM_BIN_DIR="/home/root/.local/bin"

echo "== Building =="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j

echo "== Deploying to ${RM_HOST}:${RM_BIN_DIR} =="
scp "$TARGET_BIN" "${RM_HOST}:${RM_BIN_DIR}/"

echo "Done."
```

This script:

- configures the project in `build/` (using the SDK `cmake` already in `PATH`),
- builds a **Release** binary,
- copies `mirtillo` to the device under `/home/root/.local/bin`.

It assumes:

- the SDK environment is loaded (see “Required SDK Setup”),
- SSH over USB at `root@10.11.99.1` is available.

#### `scripts/post_setup_update.sh`

```bash
#!/usr/bin/env sh
# Post-update setup for user-local filesystem on reMarkable Paper Pro 3.x

set -eu

BIN_DIR="$HOME/.local/bin"
LIB_DIR="$HOME/.local/lib"
PROFILE="$HOME/.profile"

say() { printf '%s
' "$*"; }

ensure_dir() {
  [ -d "$1" ] || { mkdir -p "$1"; say "Created: $1"; }
}

ensure_line() {
  # $1=file, $2=exact line
  [ -f "$1" ] || touch "$1"
  if ! grep -Fqx -- "$2" "$1"; then
    printf '%s
' "$2" >> "$1"
    say "Appended to $(basename "$1"): $2"
  fi
}

say "== post-update setup (mirtillo) =="

ensure_dir "$BIN_DIR"
ensure_dir "$LIB_DIR"

ensure_line "$PROFILE" 'export PATH="$HOME/.local/bin:$PATH"'
ensure_line "$PROFILE" 'export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"'

# reMarkable firmwares typically only provide "C" / "POSIX" locales.
ensure_line "$PROFILE" "export LANG=C"
ensure_line "$PROFILE" "export LC_ALL=C"
ensure_line("$PROFILE" "export QT_LOGGING_RULES='qt.core.locale.warning=false'"

say "Setup complete. Run: . $PROFILE"
```

This script is meant to be copied and executed **once per firmware update** on the device:

- ensures `~/.local/bin` and `~/.local/lib` exist,
- adds them to `PATH` and `LD_LIBRARY_PATH` in `~/.profile`,
- sets `LANG`/`LC_ALL` to `"C"` and disables noisy Qt locale warnings.

### Code structure (`src/main.cpp`)

`main.cpp` contains all the CLI logic. It is organized into:

1. **Data structures**
2. **JSON helpers**
3. **Main scan loop**
4. **Interactive menu and tag listing**

#### 1. Data structures

```cpp
struct TagRef {
    QString name;
    QString pageId;
    int     pageNumber = -1; // -1 if unknown
};

struct DocEntry {
    QString uuid;
    QString visibleName;
    QString parentUuid;   // "" = root / My Files
    bool    hasParent = false;
    QString kind;         // "pdf" | "epub" | "notebook"
    QList<TagRef> tags;   // per-page tags
    int     pages = 0;    // placeholder (not used yet)
    bool    hasTags = false;
};
```

- `TagRef` represents a single tag on a specific page (`name`, `pageId`, `pageNumber`).
- `DocEntry` represents a document (PDF/EPUB/notebook) with its metadata and tags.

The base directory of the reMarkable document store is:

```cpp
static const QString kBase =
    QStringLiteral("/home/root/.local/share/remarkable/xochitl");
```

#### 2. JSON helpers

```cpp
static bool loadJsonObject(const QString& path, QJsonObject& out);
static QString readFileType(const QJsonObject& content);
static QJsonArray readPageTags(const QJsonObject& content);
static QHash<QString,int> buildPageMap(const QJsonObject& content);
```

- `loadJsonObject`  
  Opens a JSON file and returns its contents as a `QJsonObject`. Used for both
  `*.metadata` and `*.content`.

- `readFileType`  
  Reads the document type from:
  - `extraMetaData.fileType`
  - or fallback `fileType` at the root level

  Possible values: `"pdf"`, `"epub"`, `"notebook"`.

- `readPageTags`  
  Reads `pageTags` from:
  - `extraMetaData.pageTags`
  - or fallback `pageTags` at the root level.

- `buildPageMap`  
  Builds a map `pageId -> pageNumber` using:

  - `cPages.pages[].redir.value`  
  - or fallback root-level `pages` if `cPages` is missing.

#### 3. Main scan loop

In `main()`:

- Locale and environment are initialized **before** constructing `QCoreApplication`.

- A `--debug` flag is parsed from `argv` (optional).

- The program enumerates all `*.metadata` files under `kBase`, and for each:

  - loads `.metadata`
  - skips deleted or trashed documents
  - loads `.content`
  - determines `fileType` (`pdf`/`epub`/`notebook`)
  - builds the `pageId -> pageNumber` map
  - reads all `pageTags` and converts them to `TagRef` entries

Documents are stored into three lists:

```cpp
QList<DocEntry> pdfs, epubs, notebooks;
```

and sorted alphabetically by `visibleName`.

#### 4. Interactive menu & tag listing

At runtime, `mirtillo`:

1. Asks which document type to list (`p`/`e`/`n`).  
2. Prints a two-column index of documents.  
3. Asks the user to select one by number.  
4. Prints:
   - Title  
   - UUID  
   - Parent UUID (or `false` if root)  
5. Prints all tags for that document, with page numbers and page UUIDs.  
6. If `--debug` is used, prints counters about scanned/deleted/trashed entries.

This CLI core is the foundation for future features such as:

- JSON export of the tag index,
- PDF summary generation,
- and a graphical tag/index viewer on the device.

---

## ⚠️ Disclaimer

mirtillo:

- does **not** patch, replace, or modify `xochitl`  
- does **not** remount or alter the root filesystem  
- operates fully within user space (`/home/root`)  
- is compatible with official update policies  

This project is **not affiliated with reMarkable AS**.

---

## 📜 License

MIT License

---

## ❤️ Contributions

Pull requests are welcome.  
If you build features involving thumbnails, annotation parsing, or PDF generation,  
feel free to open an issue to discuss design choices.
