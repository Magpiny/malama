# Malama v0.2.4

> Native Linux chat client for local LLMs — no cloud, no browser, no compromise.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](#)
[![Platform: Linux](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](#)
[![Built with wxWidgets](https://img.shields.io/badge/UI-wxWidgets%203.3-green.svg)](#)

---

![Malama](./assets/ic_malama.jpeg)

## 🚀 Key Features

* **Ultra-Minimalist Footprint:** Idles at ~20MB of RAM. Bypasses bloated Electron wrappers,
  ensuring your hardware resources are strictly dedicated to model inference execution passes.
* **Persistent Sessions:** Robust SQLite3 table tracking. Pin, rename, delete, and switch 
  between historical conversation indices natively without memory leaks.
* **Startup Session Restoration:** Automatically queries data records upon application 
  initialization to instantly restore your most recent conversations exactly where you left off.
* **Asynchronous Response Hardening:** Thread-isolated final token capturing ensures model 
  completions are committed to disk before volatile memory deallocation vectors run.
* **Native Markdown & Syntax Highlighting:** Custom, zero-dependency Markdown pipeline featuring 
  a dynamic, JSON-pluggable syntax registry (Semantic highlighting for C++, Rust, and Python).
* **Low-Latency Streaming:** Asynchronous, non-blocking TCP socket implementation via Boost.Asio 
  for real-time code and text generation loops.

---

## 🎥 Introduction

### The UI
![Malama v0.2.4 UI](./assets/malama_ui.png)

---

## 🏗 Architecture & Engineering

`malama` is engineered for extreme stability, low latency, and low memory overhead.

* **Transactional Storage Engine:** Employs an optimized, thread-safe SQLite3 database engine 
  running parameterized queries. Shared database connections are protected across background thread 
  partitions using strict transaction guard locks.
* **Asynchronous Token Capture:** The background worker thread (`StreamWorker`) processes raw line 
  segments via the `glaze` JSON reflection layer and signals terminal closure markers directly to the 
  `MainFrame` controller via `wxQueueEvent`, shielding the UI thread from blocking disk operations.
* **Memory Safety Constraints:** All memory requests use a non-throwing allocation policy 
  (`std::nothrow`). Raw owning pointers are completely banned from class configurations; object 
  lifecycles are managed through clean smart pointers (`std::unique_ptr`).
* **K&R Layout Standardization:** Source formatting is governed by deterministic style mappings. 
  Every function definition, conditional block expression, and control loop uses uniform brace 
  alignment standard parameters to maximize team readability.

---

## 🛠 Build Requirements

`malama` is designed exclusively for native Linux environments. You will need:

* **Compiler:** A C++23 compliant compiler (GCC 13+ or Clang 16+).
* **Dependencies:**
  * `wxWidgets` (3.2+ or 3.3)
  * `SQLite3` (Relational database development library headers)
  * `Boost.Asio` (Network/Asynchronous Concurrency)
  * `Boost.UUID` (Secure Session Identifiers Generator)
  * `spdlog` (High-performance text stream logger engine)
  * `glaze` (Compile-time metadata-reflected JSON deserialization)
* **Build System:** `CMake` (3.28+)

---

## ⚙️ Setup and Deployment Guide

### Part 1: Install and Launch Ollama Backend (4 Pathways)

Choose the precise installation pathway matching your target Linux distribution framework:

#### Option A: Universal Script Installation (Recommended)
This standalone installer automates architecture detection, downloads compiled binaries, 
and sets up a dedicated systemd service context:
```bash
curl -fsSL [https://ollama.com/install.sh](https://ollama.com/install.sh) | sh

```

Option B: Ubuntu / Debian Deployment
On Ubuntu environments, you can utilize native snap configuration containers to enforce absolute
process sandboxing and isolated access permissions:

```Bash
sudo snap install ollama
```
#### Option C: Fedora / Red Hat Enterprise Linux
For Fedora setups, manually extract the binary stack directly into your local system execution path
and verify group authorization metrics:

```bash
sudo curl -L [https://ollama.com/download/ollama-linux-amd64](https://ollama.com/download/ollama-linux-amd64) -o /usr/local/bin/ollama
sudo chmod +x /usr/local/bin/ollama
```

Option D: Arch Linux Deployment
Arch Linux provides native tracking packages directly inside the official software repositories:

```Bash
sudo pacman -S ollama
```

#### Part 2: Manage and Provision Local Inference Models
Once the background service container is fully operational, follow these instructions to download,
provision, and audit your underlying LLM infrastructure.

##### Step 1: Start and Enable the Background Service Daemon
Ensure the systemd execution framework is actively hosting the daemon listener pipeline:

```Bash
sudo systemctl enable ollama
sudo systemctl start ollama
```

Step 2: Download Your Working Target Model Asset
Pull the specialized model asset to populate your local parameters storage vault.

```Bash
ollama pull ornith
```

#### Step 3: Confirm Local Model Availability
Audit your local inference environment to verify that the target parameter block has been successfully
registered on your system:

Bash
ollama list
#### Part 3: Compile and Execute Malama UI Client
Follow this systematic build sequence to pull the codebase layer from GitHub, link against your native
system dependencies, and run the localized chat workspace wrapper.

Step 1: Install System Development Pre-requisites
Install the standard system development headers required to build the native desktop container:

Bash
# Example for Debian/Ubuntu environments
```bash
sudo apt update
sudo apt install build-essential cmake libwxgtk3.2-dev libsqlite3-dev libboost-all-dev
Step 2: Clone the Project Framework Repository
Pull the stable release branch directly from the remote GitHub source mirror:
```

```bash
git clone [https://github.com/Magpiny/malama.git](https://github.com/Magpiny/malama.git)
cd malama
```

#### Step 3: Configure and Build the Application
Create a thread-isolated build workspace layout and run the multi-threaded compilation pass:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

#### Step 4: Run the Application Client Workspace
Launch the native GTK-backed interface binary directly from your active compilation path:

```bash
./malama
```
Typing Prompts: Utilize the low-profile text area input field at the base of the UI layout.
Type your request and press Enter or click Send.

Session Controls: Right-click sidebar entries to toggle pins, rename threads, or wipe histories.
