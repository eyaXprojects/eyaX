# 🚀 Contributing to eyaX

Thank you so much for stopping by! Whether you want to fix a tiny typo, write documentation, or implement a brand-new kernel feature, your help is incredibly valuable. 

eyaX is an experimental x86_64 operating system built completely from scratch. Because we are in early development, the architecture is highly evolving—meaning **your ideas can directly shape the future of this OS.**

---

## 👋 New Here? Start with "Good First Issues"

Writing an operating system can feel intimidating, but you don't need to be a low-level wizard to contribute! We maintain a list of small, approachable tasks specifically for newcomers.

* **Browse Open Tasks:** Check out our [Good First Issues](https://github.com).
* **Documentation & Typos:** Found a confusing comment or a missing step? Fixes to documentation are highly celebrated contributions!
* **Ask for Guidance:** If you want to help but don't know where to start, open an issue and just say *"I want to contribute!"*—we will gladly find a cool task for you.

---

## 🛠️ Getting Started (Quick Lifecycle)

1. **Fork & Clone:** Fork the `eyaX` repository and clone it to your local machine.
2. **Environment Setup:** Ensure you have the required cross-compiler (e.g., `x86_64-elf-gcc`) and emulator tools (like `QEMU`) installed. *(Tip: Check our README.md for the exact toolchain setup command!)*
3. **Build & Test:** Run a local build to make sure everything compiles out of the box.
4. **Hack Away:** Keep your changes focused on one specific feature or fix.
5. **Push & PR:** Commit your changes to a new branch, push to your fork, and open a Pull Request.

---

## 🧠 Our Development Philosophy

We develop eyaX incrementally. Our goal isn't to instantly recreate a massive, complex operating system. 
* We value **simplicity, clarity, and readability** over over-engineering.
* New functionality should have a clear purpose and fit neatly into the current architecture.
* Experimentation is highly encouraged!

---

## 📝 Rules of the Road

### 💡 Before Making Major Changes
For large changes (like new kernel subsystems or major architectural overhauls), **please open an issue first.** We love brainstorming and want to discuss the high-level design with you before you spend hours writing code! Small fixes, optimizations, and documentation updates can be submitted directly as PRs.

### 💻 Code Guidelines
* **Style:** Mirror the existing codebase layout and formatting.
* **Keep it lean:** Keep the kernel simple. Avoid adding unnecessary dependencies.
* **Verify:** Always test your changes locally in your emulator environment before submitting.
* **Minimal Friction:** Do not modify unrelated components when a smaller, isolated change is sufficient.

### 🐛 Reporting Issues & Bugs
When opening an issue, give us the context we need to help you quickly:
* What happened vs. what you expected to happen.
* Step-by-step instructions to reproduce the behavior.
* Relevant build error output or emulator panic screenshots.
* Your current development environment (e.g., Host OS version, QEMU version).

### 🚀 Pull Request Checklist
When you submit a PR, please briefly fill out the template describing:
* **What** you changed.
* **Why** you changed it.
* **How** you verified/tested it (e.g., "Tested successfully in QEMU").

---

## 💬 Questions & Open Discussion

Unsure about how a specific low-level mechanism works in eyaX? Stuck on a stubborn compiler bug? Don't hesitate to **open an issue just to ask questions**. The open-source community thrives on collaboration.

Have fun hacking on eyaX! 🖥️✨
