#!/usr/bin/env bash
#
# Setup Development Environment for life-lang
# ===========================================
#
# This script installs and configures everything needed to develop life-lang:
# - Nix package manager (for reproducible builds)
# - direnv (for automatic environment activation)
# - VSCode extensions (optional)
#
# Usage:
#   ./scripts/setup-dev-environment.sh [--skip-direnv] [--skip-vscode]
#
# Options:
#   --skip-direnv    Don't install direnv
#   --skip-vscode    Don't install VSCode extensions
#
# Requirements:
#   - Linux (tested on Ubuntu, should work on most distributions)
#   - curl (for downloading installers)
#   - VSCode (only if installing extensions)
#
# After installation:
#   1. Restart your shell or run: source ~/.bashrc
#   2. cd into the project directory
#   3. If direnv is installed: direnv allow
#   4. Otherwise: nix develop
#
# Troubleshooting:
#   - "nix: command not found" → Restart shell or source ~/.bashrc
#   - "direnv: error .envrc is blocked" → Run: direnv allow
#   - Nix install fails → Check https://nixos.org/manual/nix/stable/installation/
#

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SKIP_DIRENV=false
SKIP_VSCODE=false

# Parse command line arguments
for arg in "$@"; do
  case $arg in
    --skip-direnv)
      SKIP_DIRENV=true
      shift
      ;;
    --skip-vscode)
      SKIP_VSCODE=true
      shift
      ;;
    --help)
      head -n 30 "$0" | grep '^#' | sed 's/^# \?//'
      exit 0
      ;;
    *)
      echo -e "${RED}Unknown option: $arg${NC}"
      echo "Run with --help for usage information"
      exit 1
      ;;
  esac
done

# Helper functions
info() {
  echo -e "${BLUE}[INFO]${NC} $*"
}

success() {
  echo -e "${GREEN}[SUCCESS]${NC} $*"
}

warning() {
  echo -e "${YELLOW}[WARNING]${NC} $*"
}

error() {
  echo -e "${RED}[ERROR]${NC} $*"
}

check_command() {
  if command -v "$1" &> /dev/null; then
    return 0
  else
    return 1
  fi
}

# Main installation steps

echo ""
echo "======================================"
echo "life-lang Development Environment Setup"
echo "======================================"
echo ""

# Step 1: Install Nix
info "Checking for Nix package manager..."
if check_command nix; then
  success "Nix is already installed"
  nix --version
else
  info "Installing Nix with flakes support..."
  echo ""
  warning "This will install Nix to /nix and modify your shell profile"
  warning "You'll need to restart your shell after installation"
  echo ""
  read -p "Continue? (y/N) " -n 1 -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    error "Installation cancelled"
    exit 1
  fi

  # Install Nix using Determinate Systems installer (enables flakes by default)
  if curl --proto '=https' --tlsv1.2 -sSf -L https://install.determinate.systems/nix | sh -s -- install; then
    success "Nix installed successfully"
    info "You need to restart your shell or run: source ~/.bashrc"
    warning "After restarting, run this script again to continue setup"
    exit 0
  else
    error "Nix installation failed"
    exit 1
  fi
fi

# Verify flakes are enabled
info "Verifying Nix flakes support..."
if nix flake --version &> /dev/null; then
  success "Nix flakes are enabled"
else
  error "Nix flakes are not enabled"
  error "Your Nix installation may be outdated or misconfigured"
  exit 1
fi

# Step 2: Install direnv and nix-direnv (optional)
if [ "$SKIP_DIRENV" = false ]; then
  info "Checking for direnv..."
  if check_command direnv; then
    success "direnv is already installed"
    direnv --version
  else
    info "Installing direnv and nix-direnv via Nix..."
    if nix profile install nixpkgs#direnv nixpkgs#nix-direnv; then
      success "direnv and nix-direnv installed successfully"

      # Setup direnv hook
      info "Setting up direnv shell hook..."
      SHELL_CONFIG=""
      if [ -n "${BASH_VERSION:-}" ]; then
        SHELL_CONFIG="$HOME/.bashrc"
      elif [ -n "${ZSH_VERSION:-}" ]; then
        SHELL_CONFIG="$HOME/.zshrc"
      else
        warning "Unknown shell, please add direnv hook manually"
        echo "Add this to your shell config:"
        echo '  eval "$(direnv hook bash)"  # for bash'
        echo '  eval "$(direnv hook zsh)"   # for zsh'
      fi

      if [ -n "$SHELL_CONFIG" ]; then
        if ! grep -q "direnv hook" "$SHELL_CONFIG" 2>/dev/null; then
          echo "" >> "$SHELL_CONFIG"
          echo "# direnv hook" >> "$SHELL_CONFIG"
          if [ -n "${BASH_VERSION:-}" ]; then
            echo 'eval "$(direnv hook bash)"' >> "$SHELL_CONFIG"
          else
            echo 'eval "$(direnv hook zsh)"' >> "$SHELL_CONFIG"
          fi
          success "direnv hook added to $SHELL_CONFIG"
          warning "Restart your shell or run: source $SHELL_CONFIG"
        else
          success "direnv hook already configured"
        fi
      fi
    else
      error "direnv/nix-direnv installation failed"
      exit 1
    fi
  fi

  # Setup nix-direnv integration
  info "Setting up nix-direnv integration..."
  DIRENV_CONFIG="$HOME/.config/direnv/direnvrc"
  mkdir -p "$(dirname "$DIRENV_CONFIG")"

  if ! grep -q "nix-direnv" "$DIRENV_CONFIG" 2>/dev/null; then
    echo "" >> "$DIRENV_CONFIG"
    echo "# nix-direnv integration" >> "$DIRENV_CONFIG"
    echo 'source $HOME/.nix-profile/share/nix-direnv/direnvrc' >> "$DIRENV_CONFIG"
    success "nix-direnv integration configured"
  else
    success "nix-direnv integration already configured"
  fi
else
  info "Skipping direnv installation (--skip-direnv specified)"
fi

# Step 3: Setup project environment
info "Setting up life-lang development environment..."

# Check if we're in the project directory
if [ ! -f "flake.nix" ]; then
  error "flake.nix not found in current directory"
  error "Please cd to the life-lang project directory and run this script again"
  exit 1
fi

# Build the development environment (downloads dependencies)
info "Building Nix development environment (this may take a few minutes on first run)..."
if nix develop --command echo "Environment ready"; then
  success "Development environment built successfully"
else
  error "Failed to build development environment"
  exit 1
fi

# Setup direnv for auto-activation (if installed)
if [ "$SKIP_DIRENV" = false ] && check_command direnv; then
  if [ -f ".envrc" ]; then
    info "Allowing direnv for this directory..."
    if direnv allow; then
      success "direnv configured - environment will auto-activate on cd"
    else
      warning "direnv allow failed - you may need to run 'direnv allow' manually"
    fi
  else
    warning ".envrc file not found - direnv auto-activation not available"
    info "You'll need to run 'nix develop' manually to enter the environment"
  fi
fi

# Step 4: VSCode extensions (optional)
if [ "$SKIP_VSCODE" = false ]; then
  if check_command code; then
    info "Installing VSCode extensions..."

    # Read extensions from .vscode/extensions.json
    if [ -f ".vscode/extensions.json" ]; then
      # Install recommended extensions
      # Note: This is a simple implementation - a more robust version would parse JSON properly
      grep '"' .vscode/extensions.json | grep -v '//' | grep -v 'recommendations' | grep -v 'unwantedRecommendations' | sed 's/.*"\(.*\)".*/\1/' | while read -r ext; do
        if [ -n "$ext" ]; then
          info "Installing $ext..."
          code --install-extension "$ext" --force || warning "Failed to install $ext"
        fi
      done
      success "VSCode extensions installed"
    else
      warning ".vscode/extensions.json not found - skipping extension installation"
    fi
  else
    info "VSCode not found - skipping extension installation"
    info "Install extensions manually after installing VSCode"
  fi
else
  info "Skipping VSCode extension installation (--skip-vscode specified)"
fi

# Step 5: Verify installation
echo ""
info "Verifying installation..."
echo ""

if nix develop --command show-versions; then
  echo ""
  success "All tools verified successfully!"
else
  warning "Verification failed - some tools may not be available"
fi

# Final instructions
echo ""
echo "======================================"
echo "Setup Complete!"
echo "======================================"
echo ""
info "Next steps:"
echo ""
if check_command direnv && [ "$SKIP_DIRENV" = false ]; then
  echo "  1. Restart your shell (or run: source ~/.bashrc)"
  echo "  2. cd out and back into this directory to activate direnv"
  echo "  3. Start coding! Environment will auto-activate on cd"
  echo ""
  echo "  Build commands:"
  echo "    cmake --preset debug"
  echo "    cmake --build --preset debug"
  echo "    ctest --preset debug"
  echo ""
  echo "  Helper scripts (available in PATH automatically):"
  echo "    show-versions      - Display all tool versions"
  echo "    run-tidy          - Run clang-tidy on all files"
  echo "    generate-coverage - Generate code coverage report"
else
  echo "  1. Enter the development environment: nix develop"
  echo "  2. Build the project:"
  echo "       cmake --preset debug"
  echo "       cmake --build --preset debug"
  echo "       ctest --preset debug"
  echo ""
  echo "  Helper scripts (available inside 'nix develop'):"
  echo "    show-versions      - Display all tool versions"
  echo "    run-tidy          - Run clang-tidy on all files"
  echo "    generate-coverage - Generate code coverage report"
fi
echo ""
info "For more information, see README.md"
echo ""
