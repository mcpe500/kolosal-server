#!/usr/bin/env bash

##
# Kolosal Server Termux Installer
# Builds and installs Kolosal Server on Android via Termux without root
#
# Usage:
#   cd kolosal-server
#   bash install-termux.sh           # Build and install
#   bash install-termux.sh run       # Run the server
#   bash install-termux.sh --help    # Show help
#
# Requirements:
#   - Termux with pkg
#   - Internet connection (for submodules)
##

set -e

# Version
VERSION="0.1.0"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'

# Print functions
print_info() { printf "${BLUE}ℹ${NC} %s\n" "$1"; }
print_success() { printf "${GREEN}✓${NC} %s\n" "$1"; }
print_warning() { printf "${YELLOW}⚠${NC}  %s\n" "$1"; }
print_error() { printf "${RED}✗${NC} %s\n" "$1"; }
print_header() {
    printf "\n${BOLD}%s${NC}\n" "$1"
    printf "================================\n"
}

# Get the directory where this script is located
get_repo_dir() {
    local script_path="$0"
    if [ -z "${script_path%%/*}" ]; then
        script_path="$script_path"
    else
        script_path="$(pwd)/$script_path"
    fi
    cd "$(dirname "$script_path")" && pwd
}

REPO_DIR="$(get_repo_dir)"
INSTALL_DIR="$PREFIX/opt/kolosal-server"
BIN_DIR="$PREFIX/bin"

# Check if running in Termux
check_termux() {
    if [ -z "$PREFIX" ]; then
        print_error "This script is designed for Termux on Android."
        print_info "The PREFIX environment variable is not set."
        exit 1
    fi
    
    if [ ! -d "$PREFIX" ]; then
        print_error "Termux PREFIX directory not found: $PREFIX"
        exit 1
    fi
}

# Detect Architecture
detect_arch() {
    local arch
    arch=$(uname -m)
    case "$arch" in
        aarch64|arm64) printf "arm64" ;;
        armv7l|armv8l) printf "arm" ;;
        x86_64) printf "x64" ;;
        *) print_error "Unsupported architecture: $arch"; exit 1 ;;
    esac
}

# Install required dependencies
install_dependencies() {
    print_header "Installing Dependencies"
    
    print_info "Updating package lists..."
    pkg update -y 2>/dev/null || true
    
    print_info "Upgrading packages (this may take a while)..."
    pkg upgrade -y 2>&1 || print_warning "Some packages failed to upgrade"
    
    print_info "Installing required packages..."
    
    # Core build tools and libraries
    # Note: yaml-cpp is bundled in external/ and built from source
    for package in cmake git clang make libcurl pkg-config; do
        if pkg list-installed 2>/dev/null | grep -q "^$package/"; then
            print_success "$package is already installed"
        else
            print_info "Installing $package..."
            if pkg install -y "$package" 2>&1; then
                print_success "$package installed"
            else
                print_warning "Failed to install $package"
            fi
        fi
    done
    
    # Verify CMake
    if command -v cmake >/dev/null 2>&1; then
        print_success "CMake is working: $(cmake --version | head -n1)"
    else
        print_error "CMake not found!"
        exit 1
    fi
    
    print_success "Dependencies installed"
}

# Initialize git submodules
init_submodules() {
    print_header "Initializing Submodules"
    
    cd "$REPO_DIR"
    
    if [ ! -f ".gitmodules" ]; then
        print_warning "No .gitmodules file found, skipping"
        return 0
    fi
    
    print_info "Updating git submodules (this may take a while)..."
    
    # Try submodule update, but don't fail if some submodules have issues
    git submodule update --init --recursive 2>&1 || true
    
    # Check if llama.cpp exists
    if [ ! -f "$llama_path/CMakeLists.txt" ]; then
        print_warning "llama.cpp not found, cloning latest version..."
        
        # Remove empty directory if exists
        rm -rf "$llama_path" 2>/dev/null || true
        mkdir -p "$(dirname "$llama_path")"
        
        # Clone latest llama.cpp
        if git clone --depth 1 https://github.com/ggerganov/llama.cpp.git "$llama_path"; then
            print_success "llama.cpp cloned successfully"
        else
            print_error "Failed to clone llama.cpp!"
            exit 1
        fi
    else
        print_success "llama.cpp submodule found"
    fi
    
    # Check yaml-cpp (bundled in external/)
    if [ ! -f "external/yaml-cpp/CMakeLists.txt" ]; then
        print_warning "yaml-cpp not found, trying to initialize..."
        git submodule update --init external/yaml-cpp 2>&1 || true
        
        if [ ! -f "external/yaml-cpp/CMakeLists.txt" ]; then
            print_warning "yaml-cpp submodule missing, trying manual clone..."
            rm -rf "external/yaml-cpp" 2>/dev/null || true
            git clone --depth 1 https://github.com/jbeder/yaml-cpp.git external/yaml-cpp || true
        fi
    fi
    
    # Check zlib
    if [ ! -f "external/zlib/CMakeLists.txt" ]; then
        print_warning "zlib not found, trying to initialize..."
        git submodule update --init external/zlib 2>&1 || true
        
        if [ ! -f "external/zlib/CMakeLists.txt" ]; then
            rm -rf "external/zlib" 2>/dev/null || true
            git clone --depth 1 https://github.com/madler/zlib.git external/zlib || true
        fi
    fi
    
    # Check pugixml
    if [ ! -f "external/pugixml/CMakeLists.txt" ]; then
        print_warning "pugixml not found, trying to initialize..."
        git submodule update --init external/pugixml 2>&1 || true
        
        if [ ! -f "external/pugixml/CMakeLists.txt" ]; then
            rm -rf "external/pugixml" 2>/dev/null || true
            git clone --depth 1 https://github.com/zeux/pugixml.git external/pugixml || true
        fi
    fi
    
    print_success "Submodules initialized"
}

# Build the server
build_server() {
    print_header "Building Kolosal Server"
    
    cd "$REPO_DIR"
    
    # Clean previous build if requested
    if [ "$1" = "clean" ]; then
        print_info "Cleaning previous build..."
        rm -rf build
    fi
    
    # Create build directory
    mkdir -p build
    cd build
    
    print_info "Configuring CMake..."
    print_info "Build options: CPU-only, No PoDoFo, No FAISS (for Termux compatibility)"
    
    # Configure with minimal features for Termux
    # Disable PoDoFo and FAISS to avoid complex dependencies
    if cmake -DCMAKE_BUILD_TYPE=Release \
             -DUSE_PODOFO=OFF \
             -DUSE_FAISS=OFF \
             -DUSE_CUDA=OFF \
             -DUSE_VULKAN=OFF \
             ..; then
        print_success "CMake configuration complete"
    else
        print_error "CMake configuration failed!"
        exit 1
    fi
    
    # Get number of CPU cores (limit to 2 to prevent OOM on phones)
    local jobs=2
    if [ -f "/proc/cpuinfo" ]; then
        local cores=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || echo 2)
        # Use half the cores, minimum 1, maximum 4 for phones
        jobs=$((cores / 2))
        [ "$jobs" -lt 1 ] && jobs=1
        [ "$jobs" -gt 4 ] && jobs=4
    fi
    
    print_info "Building with $jobs parallel jobs..."
    print_info "This may take 10-30 minutes on a phone. Please be patient."
    
    if make -j"$jobs"; then
        print_success "Build complete!"
    else
        print_error "Build failed!"
        print_info "Try running with less parallelism: make -j1"
        exit 1
    fi
    
    # Verify executable
    if [ -f "Release/kolosal-server" ]; then
        print_success "Executable created: Release/kolosal-server"
    elif [ -f "kolosal-server" ]; then
        print_success "Executable created: kolosal-server"
    else
        print_error "Executable not found after build!"
        exit 1
    fi
}

# Install the built server
install_server() {
    print_header "Installing Kolosal Server"
    
    cd "$REPO_DIR/build"
    
    # Find the executable
    local exe_path=""
    if [ -f "Release/kolosal-server" ]; then
        exe_path="Release/kolosal-server"
    elif [ -f "kolosal-server" ]; then
        exe_path="kolosal-server"
    else
        print_error "Executable not found! Please build first."
        exit 1
    fi
    
    # Create installation directory
    print_info "Installing to $INSTALL_DIR..."
    mkdir -p "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR/bin"
    mkdir -p "$INSTALL_DIR/configs"
    mkdir -p "$INSTALL_DIR/models"
    mkdir -p "$BIN_DIR"
    
    # Copy executable and libraries
    print_info "Copying executable..."
    cp "$exe_path" "$INSTALL_DIR/bin/"
    chmod +x "$INSTALL_DIR/bin/kolosal-server"
    
    # Copy shared libraries from the same directory
    local exe_dir=$(dirname "$exe_path")
    if ls "$exe_dir"/*.so 2>/dev/null; then
        print_info "Copying shared libraries..."
        cp "$exe_dir"/*.so "$INSTALL_DIR/bin/" 2>/dev/null || true
    fi
    
    # Copy config files
    if [ -d "$REPO_DIR/configs" ]; then
        print_info "Copying config files..."
        cp -r "$REPO_DIR/configs/"* "$INSTALL_DIR/configs/" 2>/dev/null || true
    fi
    
    # Copy static files
    if [ -d "$REPO_DIR/static" ]; then
        print_info "Copying static files..."
        cp -r "$REPO_DIR/static" "$INSTALL_DIR/" 2>/dev/null || true
    fi
    
    # Create launcher script
    print_info "Creating launcher script..."
    cat > "$INSTALL_DIR/bin/kolosal-server-launcher" << 'LAUNCHER'
#!/usr/bin/env bash

# Kolosal Server Launcher for Termux

# Resolve script location
SCRIPT_PATH="$0"
if command -v readlink > /dev/null 2>&1; then
    REAL_PATH="$(readlink -f "$SCRIPT_PATH" 2>/dev/null)" || REAL_PATH="$SCRIPT_PATH"
else
    REAL_PATH="$SCRIPT_PATH"
fi

SCRIPT_DIR="$(cd "$(dirname "$REAL_PATH")" && pwd)"
APP_DIR="$(dirname "$SCRIPT_DIR")"

# Add library path
export LD_LIBRARY_PATH="$SCRIPT_DIR:$LD_LIBRARY_PATH"

# Change to app directory (for relative paths in config)
cd "$APP_DIR"

# Execute the server
exec "$SCRIPT_DIR/kolosal-server" "$@"
LAUNCHER
    
    chmod +x "$INSTALL_DIR/bin/kolosal-server-launcher"
    
    # Create symlink in bin directory
    print_info "Creating symlink in $BIN_DIR..."
    rm -f "$BIN_DIR/kolosal-server"
    ln -sf "$INSTALL_DIR/bin/kolosal-server-launcher" "$BIN_DIR/kolosal-server"
    
    print_success "Installation complete!"
}

# Verify installation
verify_installation() {
    print_header "Verifying Installation"
    
    hash -r 2>/dev/null || true
    
    if [ -x "$BIN_DIR/kolosal-server" ]; then
        print_success "Kolosal Server installed successfully!"
        print_info "Location: $BIN_DIR/kolosal-server"
        print_info "App directory: $INSTALL_DIR"
        
        # Try to get version/help
        if "$BIN_DIR/kolosal-server" --help 2>/dev/null | head -n5; then
            print_success "Server is working!"
        else
            print_warning "Server installed but --help check failed"
        fi
    else
        print_error "Installation failed: kolosal-server not found"
        return 1
    fi
}

# Run the server
run_server() {
    print_header "Starting Kolosal Server"
    
    local server_exe=""
    
    # Check installed location first
    if [ -x "$BIN_DIR/kolosal-server" ]; then
        server_exe="$BIN_DIR/kolosal-server"
    elif [ -x "$INSTALL_DIR/bin/kolosal-server" ]; then
        server_exe="$INSTALL_DIR/bin/kolosal-server"
    elif [ -x "$REPO_DIR/build/Release/kolosal-server" ]; then
        server_exe="$REPO_DIR/build/Release/kolosal-server"
    elif [ -x "$REPO_DIR/build/kolosal-server" ]; then
        server_exe="$REPO_DIR/build/kolosal-server"
    else
        print_error "Kolosal Server not found. Please install first."
        print_info "Run: bash install-termux.sh"
        exit 1
    fi
    
    print_info "Using: $server_exe"
    print_info "Server will start on http://localhost:8080"
    print_info "Press Ctrl+C to stop"
    printf "\n"
    
    # Pass any additional arguments
    exec "$server_exe" "$@"
}

# Show usage
show_usage() {
    print_header "Quick Start"
    printf "\n"
    printf "Start the server:\n"
    printf "  ${BOLD}kolosal-server${NC}\n"
    printf "\n"
    printf "Start with config file:\n"
    printf "  ${BOLD}kolosal-server --config $INSTALL_DIR/configs/config.yaml${NC}\n"
    printf "\n"
    printf "Test the server:\n"
    printf "  ${BOLD}curl http://localhost:8080/v1/health${NC}\n"
    printf "\n"
    printf "Models directory:\n"
    printf "  ${BOLD}$INSTALL_DIR/models/${NC}\n"
    printf "\n"
}

# Show uninstall instructions
show_uninstall() {
    print_header "Uninstallation"
    printf "\n"
    printf "To uninstall Kolosal Server from Termux:\n"
    printf "  rm -rf \$PREFIX/opt/kolosal-server\n"
    printf "  rm -f \$PREFIX/bin/kolosal-server\n"
    printf "\n"
}

# Ask yes/no
ask_yes_no() {
    local prompt="$1"
    local response
    printf "%s (y/n) " "$prompt"
    read -r response
    case "$response" in
        [Yy]|[Yy][Ee][Ss]) return 0 ;;
        *) return 1 ;;
    esac
}

# Main installation flow
main() {
    print_header "Kolosal Server Termux Installer v${VERSION}"
    printf "\n"
    print_info "Building from local source (no root required)"
    
    # Check Termux
    check_termux
    print_success "Running in Termux environment"
    print_info "PREFIX: $PREFIX"
    
    # Detect architecture
    local arch=$(detect_arch)
    print_success "Detected architecture: $arch"
    
    # Check CMakeLists.txt exists
    if [ ! -f "$REPO_DIR/CMakeLists.txt" ]; then
        print_error "CMakeLists.txt not found!"
        print_info "Please run this script from the kolosal-server directory."
        exit 1
    fi
    print_success "Repository files verified"
    
    # Check if already installed
    if [ -x "$BIN_DIR/kolosal-server" ]; then
        print_warning "Kolosal Server is already installed"
        print_info "Current location: $BIN_DIR/kolosal-server"
        printf "\n"
        if ! ask_yes_no "Do you want to rebuild and reinstall?"; then
            print_info "Installation cancelled"
            exit 0
        fi
    fi
    
    # Install dependencies
    install_dependencies
    
    # Initialize submodules
    init_submodules
    
    # Build the server
    build_server
    
    # Install the server
    install_server
    
    # Verify installation
    verify_installation
    
    # Show usage
    show_usage
    
    print_success "Installation completed successfully!"
}

# Handle arguments
case "${1:-}" in
    run|start)
        shift
        run_server "$@"
        ;;
    build)
        check_termux
        init_submodules
        build_server "${2:-}"
        ;;
    install)
        check_termux
        install_server
        verify_installation
        show_usage
        ;;
    clean)
        print_info "Cleaning build directory..."
        rm -rf "$REPO_DIR/build"
        print_success "Build directory cleaned"
        ;;
    --uninstall|uninstall)
        show_uninstall
        ;;
    --help|-h|help)
        printf "Kolosal Server Termux Installer v${VERSION}\n"
        printf "\n"
        printf "Usage:\n"
        printf "  bash install-termux.sh [COMMAND]\n"
        printf "\n"
        printf "Commands:\n"
        printf "  (none)       Full install: dependencies + submodules + build + install\n"
        printf "  run          Run the installed server\n"
        printf "  build        Build only (skip install)\n"
        printf "  build clean  Clean build then rebuild\n"
        printf "  install      Install only (requires previous build)\n"
        printf "  clean        Remove build directory\n"
        printf "  uninstall    Show uninstall instructions\n"
        printf "  help         Show this help\n"
        printf "\n"
        printf "Examples:\n"
        printf "  bash install-termux.sh           # Full install\n"
        printf "  bash install-termux.sh run       # Start server\n"
        printf "  bash install-termux.sh run --config config.yaml\n"
        printf "\n"
        ;;
    *)
        main "$@"
        ;;
esac
