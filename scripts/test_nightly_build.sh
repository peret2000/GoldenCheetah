#!/bin/bash

# Script de ejemplo para testing local de la funcionalidad de la GitHub Action
# Este script puede ser usado para probar los componentes de la GitHub Action localmente

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Función para logging colorido
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Configuración
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
LOGFILE="$SCRIPT_DIR/test_build.log"
BUILDBRANCH="MyBuildAdapt"

# Función de ayuda
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Test script for nightly build GitHub Action components"
    echo ""
    echo "Options:"
    echo "  --test-merge     Test merge functionality"
    echo "  --test-build     Test build preparation"
    echo "  --test-cleanup   Test cleanup functions"
    echo "  --test-all       Run all tests"
    echo "  --help, -h       Show this help message"
    echo ""
}

# Test merge functionality
test_merge() {
    log_info "Testing merge functionality..."
    
    # Crear un log temporal para testing
    local test_log="$SCRIPT_DIR/test_merge.log"
    
    # Test de la función de merge (sin hacer merge real)
    log_info "Testing merge helper function..."
    
    # Verificar que el script auxiliar existe
    if [[ ! -f "$SCRIPT_DIR/build_helper.sh" ]]; then
        log_error "build_helper.sh not found!"
        return 1
    fi
    
    # Test log_message function
    "$SCRIPT_DIR/build_helper.sh" log_message "Test message" "$test_log"
    
    if [[ -f "$test_log" ]]; then
        log_info "✅ Log function works correctly"
        cat "$test_log"
        rm "$test_log"
    else
        log_error "❌ Log function failed"
        return 1
    fi
}

# Test build preparation
test_build() {
    log_info "Testing build preparation..."
    
    cd "$REPO_ROOT"
    
    # Verificar que estamos en el directorio correcto
    if [[ ! -f "src/gcconfig.pri" ]]; then
        log_error "Not in GoldenCheetah root directory"
        return 1
    fi
    
    # Test de preparación del entorno
    local test_log="$SCRIPT_DIR/test_build.log"
    
    if "$SCRIPT_DIR/build_helper.sh" prepare_build_environment "$test_log"; then
        log_info "✅ Build environment preparation works"
        cat "$test_log"
        rm "$test_log"
    else
        log_error "❌ Build environment preparation failed"
        return 1
    fi
}

# Test cleanup functions
test_cleanup() {
    log_info "Testing cleanup functions..."
    
    cd "$REPO_ROOT"
    
    local test_log="$SCRIPT_DIR/test_cleanup.log"
    
    if "$SCRIPT_DIR/build_helper.sh" cleanup_conflicting_files "$test_log"; then
        log_info "✅ Cleanup function works"
        cat "$test_log"
        rm "$test_log"
    else
        log_error "❌ Cleanup function failed"
        return 1
    fi
}

# Test build info
test_build_info() {
    log_info "Testing build info extraction..."
    
    cd "$REPO_ROOT"
    
    local test_log="$SCRIPT_DIR/test_info.log"
    
    # Backup original gcconfig.pri
    cp src/gcconfig.pri src/gcconfig.pri.backup
    
    if "$SCRIPT_DIR/build_helper.sh" get_build_info "$test_log"; then
        log_info "✅ Build info function works"
        cat "$test_log"
        
        # Verificar que se añadió la información de versión
        if grep -q "GC_VERSION" src/gcconfig.pri; then
            log_info "✅ Version info added to gcconfig.pri"
        else
            log_warning "⚠️ Version info not found in gcconfig.pri"
        fi
        
        rm "$test_log"
    else
        log_error "❌ Build info function failed"
        return 1
    fi
    
    # Restore original gcconfig.pri
    mv src/gcconfig.pri.backup src/gcconfig.pri
}

# Test dependencies
test_dependencies() {
    log_info "Testing dependencies..."
    
    # Verificar comandos necesarios
    local missing_deps=()
    
    command -v git >/dev/null 2>&1 || missing_deps+=("git")
    command -v qmake6 >/dev/null 2>&1 || missing_deps+=("qmake6")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    
    if [[ ${#missing_deps[@]} -eq 0 ]]; then
        log_info "✅ All required dependencies are available"
    else
        log_error "❌ Missing dependencies: ${missing_deps[*]}"
        return 1
    fi
}

# Test GitHub Action syntax
test_github_action() {
    log_info "Testing GitHub Action syntax..."
    
    local action_file="$REPO_ROOT/.github/workflows/nightly-build.yml"
    
    if [[ ! -f "$action_file" ]]; then
        log_error "GitHub Action file not found: $action_file"
        return 1
    fi
    
    # Test básico de sintaxis YAML
    if command -v python3 >/dev/null 2>&1; then
        if python3 -c "import yaml; yaml.safe_load(open('$action_file'))" 2>/dev/null; then
            log_info "✅ GitHub Action YAML syntax is valid"
        else
            log_error "❌ GitHub Action YAML syntax is invalid"
            return 1
        fi
    else
        log_warning "⚠️ Python3 not available, skipping YAML syntax test"
    fi
    
    # Verificar que tiene los elementos básicos
    if grep -q "name: Nightly Build" "$action_file" && \
       grep -q "on:" "$action_file" && \
       grep -q "jobs:" "$action_file"; then
        log_info "✅ GitHub Action has required structure"
    else
        log_error "❌ GitHub Action missing required structure"
        return 1
    fi
}

# Run all tests
run_all_tests() {
    log_info "Running all tests..."
    
    local failed_tests=()
    
    test_dependencies || failed_tests+=("dependencies")
    test_github_action || failed_tests+=("github-action")
    test_merge || failed_tests+=("merge")
    test_build || failed_tests+=("build")
    test_cleanup || failed_tests+=("cleanup")
    test_build_info || failed_tests+=("build-info")
    
    if [[ ${#failed_tests[@]} -eq 0 ]]; then
        log_info "🎉 All tests passed!"
        return 0
    else
        log_error "❌ Failed tests: ${failed_tests[*]}"
        return 1
    fi
}

# Main execution
cd "$REPO_ROOT"

case "${1:-}" in
    --test-merge)
        test_merge
        ;;
    --test-build)
        test_build
        ;;
    --test-cleanup)
        test_cleanup
        ;;
    --test-info)
        test_build_info
        ;;
    --test-deps)
        test_dependencies
        ;;
    --test-action)
        test_github_action
        ;;
    --test-all)
        run_all_tests
        ;;
    --help|-h)
        show_help
        ;;
    "")
        log_warning "No option specified. Use --help for usage information."
        show_help
        ;;
    *)
        log_error "Unknown option: $1"
        show_help
        exit 1
        ;;
esac
