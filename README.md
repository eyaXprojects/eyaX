# 🖥️ eyaX

**An experimental, lightweight x86_64 hobby operating system built completely from scratch using the Limine boot protocol.**

Designed with an absolute focus on architectural simplicity, transparency, and minimalist, non-bloated software design. Developed by Eyad Ahmed Mohamed.

[![Sponsor Developer](https://shields.io)](https://github.com)
[![Contribute Welcome](https://shields.io)](CONTRIBUTING.md)
[![License: GPL v2](https://shields.io)](LICENSE)

---

## 📸 System Preview

Here is `eyaX` boot diagnostics, custom memory map validation, and interactive shell shell routing running inside a QEMU hardware emulator:

<img width="100%" alt="eyaX OS Shell and Memory Management Diagnostics running under QEMU Emulator" src="https://github.com/user-attachments/assets/a761f817-ddbd-4cfa-967e-7be2bc6d343d" />

---

## 🛠️ Core Features

Despite its small footprint, `eyaX` features an entirely custom, hand-crafted core subsystem stack:
* **Segment Isolation:** Custom Global Descriptor Table (GDT) configuration.
* **Interrupt Routing:** Robust Interrupt Descriptor Table (IDT) for CPU exceptions and hardware requests.
* **Hardware Drivers:** Low-level PS/2 Keyboard controller interfacing.
* **Graphic Engine:** Direct linear framebuffer console for text rendering and custom fonts.
* **User Interface:** Lightweight, interactive built-in terminal shell with responsive utility commands.
* **Memory Architecture:** Custom **ZMM Memory Manager** built completely by hand for allocation and mapping tracking.

---

## ⚡ Quick Start: How to Build & Run

### Prerequisites
To cross-compile the codebase, your system needs the target toolchain and hardware emulation packages:
```bash
# Example for Debian/Ubuntu systems to grab build utilities & QEMU
sudo apt install build-essential qemu-system-x86 gcc-x86-64-linux-gnu
```
*(Note: You will specifically need the `x86_64-elf-gcc` cross-compiler environment config configured).*

### 1. Compile the Kernel
Build the raw kernel binaries directly using the native configuration file:
```bash
make
```

### 2. Generate the Bootable ISO Image
Generate a Limine-compliant bootable image media file:
```bash
chmod +x mkiso.sh
./mkiso.sh
```

### 3. Launch the Emulator
Instantly test and execute your local environment build inside QEMU:
```bash
qemu-system-x86_64 -cdrom image.iso
```
*Alternatively, you can load the generated `image.iso` into VirtualBox, VMware, or deploy it directly to real hardware.*

---

## 🤝 Community & Contributing

Contributions are heavily appreciated and highly welcome! `eyaX` is the perfect playground for people wanting to learn how low-level systems programming, x86_64 architecture, and bare-metal components operate.

If you would like to implement a feature, optimize code, or fix documentation, please read our structural guidelines first in [CONTRIBUTING.md](CONTRIBUTING.md).

---

## 💖 Support the Project

Building an independent operating system from scratch takes a massive investment of time, research, and technical debugging. If you love this project, find it educational, or want to fuel its ongoing development, **please consider starring the repository or donating/sponsoring.** 

Every bit of financial backing directly supports the creator and keeps development moving forward!

---

## 💡 Engineering Philosophy & Open Transparency
* **Human-Crafted Foundation:** While artificial intelligence tools were consulted for small contextual syntax queries, such as this README, and other things, the architectural heavy-lifting—including the entire custom **ZMM memory manager** and **framebuffer console pipeline**—was completely designed and coded line-by-line by a human engineer.
* **License:** `eyaX` is released as free, open-source software under the terms of the robust **GNU General Public License v2.0 (GPL-2.0)**.
