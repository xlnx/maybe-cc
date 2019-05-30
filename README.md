# Build

Before build, please make sure:
* **Rust 2018** is installed in your PATH.
  * Linux: `curl https://sh.rustup.rs -sSf | sh`
  * Windows: Find out by your self.
* **Rust Nightly Toolchain** in installed and made by default.
  * `rustup toolchain add nightly && rustup default nightly`
* **LLVM ^6.0** is installed in your PATH.
  * Fedora: `sudo dnf install llvm-devel`
  * Ubuntu: `sudo apt install llvm-dev`

Clone this repo with dependencies initialized:

```bash
$ git clone --recursive https://github.com/xlnx/maybe-cc.git
```

Run cargo build commands over the repo:

```bash
$ cd maybe-cc
$ cmake . -Bbuild
$ cmake --build build
```
