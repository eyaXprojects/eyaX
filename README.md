# eyaX
The eyaX kernel (created by Eyad Ahmed Mohamed), is an x86_64 Limine-based kernel focused on simplicity and non-bloat.

# Building
You need the x86_64-elf-gcc cross compiler to compile the kernel.

To compile, run:
```make```

And to create the ISO, run:
```chmod +x mkiso.sh```
Afterwards:
```./mkiso.sh```

You can emulate the image by QEMU:
```qemu-system-x86_64 -cdrom image.iso```
or by VBox, VMware, etc.

# Features
GDT
IDT
Keyboard
Framebuffer console
Shell with commands.

You can contribute to add more features to the eyaX kernel.

# Contribution
Contribution is strongly appreciated.

# License
eyaX was released under the open-source GPL license.

# Look
<img width="1366" height="768" alt="Screenshot from 2026-09-25 16-11-58" src="https://github.com/user-attachments/assets/a761f817-ddbd-4cfa-967e-7be2bc6d343d" />

