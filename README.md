# Claudimon

**Claudimon** is an experimental operating system kernel project built from scratch, exploring how far Claude can be used as a primary tool for systems programming and kernel development.

This project is less about “shipping a production kernel” and more about answering a question:

> How far can AI go when tasked with building something as low-level, complex, and unforgiving as an OS kernel?

---

Every commit, every file created (except the LICENSE and README.md) is fully created by Anthropic's AI, 'Claude.', using Sonnet 4.6 high.
</br>
Pronounced 'Klaud-ih-mon'.

---

# Installation

For Debian/Ubuntu;

```bash
sudo apt install nasm gcc grub-pc-bin grub-common xorriso qemu-system-x86
```

For Fedora;

```bash
sudo dnf install nasm gcc grub2-tools xorriso qemu-system-x86
```

For Arch;

```bash
sudo pacman -S nasm gcc binutils grub xorriso qemu
```

---

Then run ```make```, then once it completes, run ```make run``` and enjoy!
</br>
More information available at the official [website](https://kernel.mtgp.cc), (recommended to visit first)
