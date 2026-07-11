# Release Notes

## v1.0.8-beta

### Changes
- **Removed macOS x86_64 (Intel) builds** — GitHub Actions no longer provides free Intel macOS runners. Only Apple Silicon (arm64) builds are shipped going forward.

### Components
- **sub**: Transpiler (SUB → Python, JavaScript, C, Go, Rust, and more)
- **subc**: Native compiler (SUB → machine binary via C backend)
- **subi**: Tree-walk interpreter (direct execution)

### Supported Platforms
- Linux x86_64
- Linux arm64 (aarch64)
- macOS arm64 (Apple Silicon)
- Windows x86_64 (MinGW)
