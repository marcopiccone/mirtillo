# mirtillo  
*A metadata-driven extension for the reMarkable Paper Pro — generating per-document tag indexes, extracting highlights/notes, and exporting structured summaries.*

--- 

## 📘 Overview

**mirtillo** is a command-line tool designed to operate alongside the reMarkable  
Paper Pro, providing capabilities that the official interface does not expose.

The purpose of mirtillo is **not** to replace `xochitl`, but to complement it with  
powerful data-extraction features — entirely from user space, without touching  
the read-only rootfs introduced in firmware **3.23+**.

### 🎯 Final Goal

**mirtillo aims to become an external extension to the reMarkable**, capable of:

- Building a **per-document index of tags**
- Extracting:
  - tagged pages  
  - underlined passages  
  - handwritten notes (where possible)
- Generating a **local PDF summary** that groups:
  - tag → page number → page UUID  
  - text highlights  
  - user annotations  
  - (optional) page thumbnails
- Exporting the summary back to your laptop automatically

This transforms scattered markings inside a notebook or book into a  
**coherent, searchable, portable document**.

---

## 🌟 Features (current)

- Parses `.metadata` and `.content` files from:

  ```
  /home/root/.local/share/remarkable/xochitl/
  ```

- Classifies documents as:
  - PDF  
  - EPUB  
  - Notebook  

- Extracts:
  - `visibleName`
  - folder relationships (`parent` UUID)
  - deletion status
  - pageTags (local tags per page)
  - page numbers from cPages

- Outputs structured listings directly on the device via SSH  
- Fully compatible with firmware **3.23+** (read-only root filesystem)  
- Entirely user-space: installed in `~/.local/bin` via `post-update` template  

---

## 🔮 Planned Features (Roadmap)

### Phase 1 — Core CLI (done / in progress)

- [x] Document discovery  
- [x] PDF/EPUB/Notebook classification  
- [x] Parsing of metadata + content  
- [x] Extraction of pageTags  
- [x] Mapping tags → page numbers → UUIDs  

### Phase 2 — Summary Export

- [ ] Extract highlighted passages  
- [ ] Extract text selections (where available)  
- [ ] Extract handwritten notes per page  
- [ ] Gather thumbnails from `.thumbnails/`  
- [ ] Generate structured JSON summaries  
- [ ] Export combined summary as **PDF report**

### Phase 3 — Integration Layer

- [ ] Auto-sync summaries via SSH  
- [ ] GUI frontend (Qt Quick on Paper Pro)  
- [ ] Optional macOS companion app  

### Phase 4 — Document Extensions

- [ ] Embed tag index directly inside a duplicate PDF  
- [ ] Generate an on-device “Tag Index Viewer”  
- [ ] Allow per-document virtual bookmarking  

---

## 📦 Repository Layout

```
mirtillo/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── src/
│   └── main.cpp
├── scripts/
│   ├── post-update-mirtillo-setup.sh
│   └── deploy-to-paperpro.sh
└── cmake/
    └── toolchain-remarkable.cmake
```

---

## 🛠️ Requirements (host side)

You need:

- macOS (Apple Silicon) **or** any Linux host  
- QEMU (only if you build inside a Debian VM on macOS)  
- Debian 12 x86_64 as build environment  
- Official reMarkable SDK/toolchain for aarch64 installed inside the VM  

---

## 🏗️ Building inside the VM

```bash
git clone https://github.com/<your-user>/mirtillo.git
cd mirtillo
mkdir -p build
cd build

cmake ..   -DCMAKE_BUILD_TYPE=Release   -DCMAKE_TOOLCHAIN_FILE="$REMARKABLE_SDK/toolchain.cmake"

cmake --build . -j
```

The resulting binary is:

```
build/mirtillo
```

---

## 📤 Deploy to reMarkable Paper Pro

### 1. Ensure SSH over USB is enabled.

### 2. From the VM:

```bash
ssh root@10.11.99.1 'sh ~/post-update-mirtillo-setup.sh || true'
scp build/mirtillo root@10.11.99.1:/home/root/.local/bin/
```

### 3. On the device:

```bash
ssh root@10.11.99.1
mirtillo
```

---

## 📄 Usage Example

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
Tags  :
   1. Simon Mago — page 38 (UUID: bad5d945-fc7c-4a68-a522-290817f2413c)
   2. Ofiti — page 41 (UUID: 8bb36620-4b0f-4355-8d3b-6f4ac68a61b3)
   …
```

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

MIT License (recommended for open development)

---

## ❤️ Contributions

Pull requests are welcome.  
If you build features involving thumbnails, annotation parsing, or PDF generation,  
feel free to open an issue to discuss design choices.
