# Kolosal Server - Termux Installation Guide

Build and run Kolosal Server (LLM inference) on Android via Termux.

## Quick Install

```bash
cd ~/kolosal/kolosal-server
bash install-termux.sh
```

## Usage

### Start Server
```bash
kolosal-server
# Or with config:
kolosal-server --config $PREFIX/opt/kolosal-server/configs/config.yaml
```

### Test Server
```bash
curl http://localhost:8080/v1/health
```

## Commands

| Command | Description |
|---------|-------------|
| `bash install-termux.sh` | Full install |
| `bash install-termux.sh run` | Start server |
| `bash install-termux.sh build clean` | Clean rebuild |
| `bash install-termux.sh clean` | Remove build dir |

## Directories

- **Binary**: `$PREFIX/bin/kolosal-server`
- **App**: `$PREFIX/opt/kolosal-server/`
- **Models**: `$PREFIX/opt/kolosal-server/models/`
- **Config**: `$PREFIX/opt/kolosal-server/configs/`

## Notes

- Build takes **10-30 minutes** on phone
- CPU-only build (CUDA/Vulkan disabled)
- FAISS and PoDoFo disabled for compatibility
- Requires ~2GB storage for build

## Uninstall

```bash
rm -rf $PREFIX/opt/kolosal-server
rm -f $PREFIX/bin/kolosal-server
```
