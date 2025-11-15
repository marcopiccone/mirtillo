# mirtillo
*A metadata‑driven extension for the reMarkable Paper Pro — generating per‑document tag indexes, extracting highlights/notes, and exporting structured summaries.*

---

## 📘 Overview

**mirtillo** is a command‑line tool designed to operate entirely in user space on the  
**reMarkable Paper Pro**, providing advanced document‑introspection features not exposed  
by the official `xochitl` reader.

mirtillo **does not modify system files**, does not remount the rootfs, and remains fully  
compatible with firmware **3.23+**, where the root filesystem is read‑only.

Its long‑term goal is to become a companion utility that:

- Builds **per‑document tag indexes**
- Extracts:
  - tagged pages  
  - highlights  
  - handwritten notes (when available)  
- Generates a **summary PDF** containing:
  - tag → page → page UUID mapping  
  - text excerpts  
  - notes  
  - optional thumbnails  
- Copies the summary automatically to your computer

---

## 🌟 Features (current)

- Parses `.metadata` and `.content` for all documents under:

```
/home/root/.local/share/remarkable/xochitl/
```

- Detects document type via `extraMetaData.fileType`
- Classifies documents:
  - PDF
  - EPUB
  - Notebook
- Extracts:
  - visible name
  - folder hierarchy (`parent` UUID)
  - deletion status
  - per‑page tags (`pageTags`)
  - page numbers (`cPages.pages[].redir.value`)
- Displays structured document previews via CLI
- Installs cleanly into:
```
/home/root/.local/bin/mirtillo
/home/root/.local/share/mirtillo/ABOUT.txt
```
- Includes:
  - `--version`
  - `--about`
  - `--debug`
- Fully independent from `xochitl`

---

## 🔮 Roadmap

### Phase 1 — Core CLI (complete)
- Document discovery
- Parsing metadata + content
- Tag extraction
- Page mapping

### Phase 2 — Export Engine (planned)
- Export tagged excerpts
- Extract highlights and annotations
- Gather thumbnails
- Produce JSON summaries
- Generate PDF summaries

### Phase 3 — Integration Layer
- Auto‑sync
- On‑device Qt‑based viewer
- macOS companion app

---

## 📦 Repository Structure

```
mirtillo/
├── src/
│   └── main.cpp
├── scripts/
│   ├── post-update-mirtillo-setup.sh
│   └── deploy_to_paperpro.sh        # unified build+deploy helper
├── CMakeLists.txt
├── ABOUT.txt
└── README.md
```

---

# 🧰 Development Setup

mirtillo is cross‑compiled using the **official reMarkable Paper Pro SDK**.

## Install the SDK

Follow the official instructions from reMarkable.  
Assume your SDK is installed under:

```
$HOME/rm-sdk
```

Then load it into your shell:

```bash
export RMSDK="$HOME/rm-sdk"
export QEMU_LD_PREFIX="$RMSDK/sysroots/cortexa53-crypto-remarkable-linux"
source "$RMSDK/environment-setup-cortexa53-crypto-remarkable-linux"
```

✔️ **This must be done before running CMake.**

---

# 🔧 Building mirtillo

Inside your Debian x86_64 VM:

```bash
git clone https://github.com/<your-username>/mirtillo.git
cd mirtillo
mkdir -p build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

Build output:

```
build/mirtillo
```

---

### SDK environment not loaded
If CMake reports missing compilers or cannot find Qt:

```
export RMSDK="$HOME/rm-sdk"
export QEMU_LD_PREFIX="$RMSDK/sysroots/cortexa53-crypto-remarkable-linux"
source "$RMSDK/environment-setup-cortexa53-crypto-remarkable-linux"
```

Then re-run CMake.

---

### Mismatch between build/ folders
If you move the project directory manually, CMake will complain:

```
The current CMakeCache.txt directory ... is different from ...
```

Fix by removing the old build:

```
rm -rf build/
mkdir build
cd build
cmake ..
```

---

# 🧪 Testing the build locally (QEMU‑ld)

You can run the binary natively inside the VM even though it is built for aarch64:

```bash
export QEMU_LD_PREFIX="$RMSDK/sysroots/cortexa53-crypto-remarkable-linux"
qemu-aarch64 ./buid/mirtillo --version
```

This uses the SDK's sysroot as a transparent loader.

If it runs correctly under QEMU, it will run on the Paper Pro.

---

## 🧩 Developer Documentation
See: [CODE_STRUCTURE.md](CODE_STRUCTURE.md)

---

# 🚀 Deploying to the Paper Pro

Ensure USB‑SSH is enabled on the device.

You may deploy in **three ways**:

---

## 1) Password‑based deploy
```bash
scripts/deploy_to_paperpro.sh --build --deploy
```

Prompts for password when necessary.

---

## 2) Deploy using SSH keys (recommended)

Using SSH keys avoids entering the password at every deploy and makes
scripts/deploy_to_paperpro.sh --sshkeys work instantly.

# Generate a dedicated keypair for mirtillo
```bash
ssh-keygen -t ed25519 -f ~/.ssh/rm_key -C "remarkable-mirtillo"
```

This will create:

```bash
~/.ssh/rm_key      (private key)
~/.ssh/rm_key.pub  (public key)
```

### Copy key to the Paper Pro
Try the standard method first:

```bash
ssh-copy-id -i ~/.ssh/rm_key.pub root@10.11.99.1
```

⚠️ *If ssh-copy-id fails (common on bare-bones embedded systems)*

Some minimal systems (including some reMarkable firmware builds) do not
ship with ssh-copy-id. If the command fails or appears to work but
`ssh -i ~/.ssh/rm_key root@10.11.99.1` still asks for a password,
install the key **manually**:

```bash
scp ~/.ssh/rm_key.pub root@10.11.99.1:/home/root/
ssh root@10.11.99.1 "mkdir -p ~/.ssh && chmod 700 ~/.ssh && cat ~/rm_key.pub >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys && rm ~/rm_key.pub"
```

This guarantees:
	•	`~/.ssh/` exists
	•	correct permissions
	•	key correctly appended to authorized_keys
	•	no stray files left around

Now test:

```bash
ssh -i ~/.ssh/rm_key root@10.11.99.1
```

If it logs in **without asking a password**, the setup is correct.

### Deploy without prompts
```bash
scripts/deploy_to_paperpro.sh --build --sshkeys
```

---

## 3) Setup only once after reMarkable firmware updates (usually not necessqry now)

The script:

```bash
scripts/post-update-mirtillo-setup.sh
```

creates:

```
~/.local/bin
~/.local/lib
~/.local/share/mirtillo
```

Run once after each firmware update:

```bash
ssh root@10.11.99.1 'sh /home/root/post-update-mirtillo-setup.sh'
```

---

# 🖥️ Running mirtillo on the device

```bash
ssh root@10.11.99.1
mirtillo
```

Example:

```
Document type to list? [p] PDF  [e] EPUB  [n] notebook:
p

Found 42 PDF document(s):

  1) Confutatio Omnium Haeresium          2) Gospel of Thomas
  3) Plato — Phaedrus                      4) Tolkien Companion
  …

Select: 1

Title : Confutatio Omnium Haeresium
UUID  : 6667662b-ff4a-4e25-bf9d-b719a36c26f3
Parent: <root>
Tags:
  1. Simon Mago — page 38  (UUID: bad5d945‑fc7c‑4a68‑a522‑290817f2413c)
  2. Ofiti — page 41       (UUID: 8bb36620‑4b0f‑4355‑8d3b‑6f4ac68a61b3)
```

---

# 🛠️ Troubleshooting

### UTF‑8 warning on the Paper Pro
The firmware’s locale is fixed to `C`.  
The warning is harmless:

```
Detected locale "C" with character encoding "ANSI_X3.4-1968"
```

mirtillo ignores it correctly.  
If needed, you may silence it by adding to `~/.profile`:

```
export QT_LOGGING_RULES='qt.core.locale.warning=false'
```

---

### SSH password prompts (multiple)
Ensure you are either:

- using `ssh-copy-id` (recommended), or  
- deploying with `--sshkeys`

If you see repeated prompts, your private key may not be loaded:

```
ssh-add ~/.ssh/id_ed25519
```

---

# 🏷️ Version Badge

You may add this badge at the top of the README:

```
![version](https://img.shields.io/badge/version-0.1-blue)
```

(It can be updated automatically when bumping project version.)

---

# ⚙️ Script Reference

## mirtillo.sh

Supports:

- `--build`
- `--deploy`
- `--sshkeys`
- `--setup`

Examples:

```bash
scripts/mirtillo.sh --build --deploy
scripts/mirtillo.sh --build --sshkeys
scripts/mirtillo.sh --setup
```

---

# ⚠️ Disclaimer

mirtillo:

- does **not** replace or patch `xochitl`
- does **not** modify or remount the system partition
- operates fully in **/home/root**
- is compatible with official update policies

This project is **not affiliated** with reMarkable AS.

---

# 📜 License

MIT License.

---

# ❤️ Contributions

Pull requests are welcome!  
If you add features (thumbnails, PDF summaries, GUI), please open an issue  
to coordinate design choices.