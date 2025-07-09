#!/bin/bash

# Script auxiliar para la GitHub Action de nightly builds
# Replica algunas funcionalidades del build_project.sh original

# Función para logging similar al script original
log_message() {
    local message="$1"
    local logfile="$2"
    echo "$(date): $message" | tee -a "$logfile"
}

# Función para salir con código de error
exit_with_error() {
    local error_code="$1"
    local message="$2"
    local logfile="$3"
    
    if [[ -n "$message" ]]; then
        log_message ">>>EJECUCIÓN FALLIDA: $message (código: $error_code)" "$logfile"
    fi
    
    # En GitHub Actions, usar las anotaciones de GitHub
    if [[ -n "$GITHUB_ACTIONS" ]]; then
        echo "::error::Build failed with code $error_code: $message"
    fi
    
    exit "$error_code"
}

# Función para merge con manejo de errores
merge_branch() {
    local branch="$1"
    local current_branch="$2"
    local logfile="$3"
    
    log_message "Merging $branch..." "$logfile"
    
    if git merge --no-edit "$branch" > /dev/null 2>&1; then
        log_message "✅ merge $branch OK" "$logfile"
    else
        local err=$?
        log_message "❌ ERROR $err: merge $branch FAILED" "$logfile"
        exit_with_error $err "merge $branch failed" "$logfile"
    fi
}

# Función para verificar si estamos en la última versión de una rama
check_branch_up_to_date() {
    local branch="$1"
    local logfile="$2"
    
    local commit_before=$(git rev-parse "$branch")
    local commit_after=$(git rev-parse "origin/$branch")
    
    if [ "$commit_before" != "$commit_after" ]; then
        log_message "FAILED. $branch NOT in last version." "$logfile"
        exit_with_error 1 "$branch not up to date" "$logfile"
    fi
}

# Función para preparar el entorno de build
prepare_build_environment() {
    local logfile="$1"
    
    log_message "Preparing build environment..." "$logfile"
    
    # Verificar que estamos en el directorio correcto
    if [[ ! -f "src/gcconfig.pri" ]]; then
        exit_with_error 1 "Not in GoldenCheetah root directory" "$logfile"
    fi
    
    # Establecer variables de entorno para Qt6
    export PATH="/usr/lib/qt6/bin:$PATH"
    export LD_LIBRARY_PATH="/usr/lib/qt6/lib:$LD_LIBRARY_PATH"
    
    log_message "✅ Build environment prepared" "$logfile"
}

# Función para limpiar archivos que pueden causar conflictos en merge
cleanup_conflicting_files() {
    local logfile="$1"
    
    log_message "Cleaning up files that might cause merge conflicts..." "$logfile"
    
    git checkout -- src/Resources/translations/ 2>/dev/null || true
    git checkout -- src/Core/Secrets.h 2>/dev/null || true
    git checkout -- src/Resources/linux/MakeAppImageQt6.sh 2>/dev/null || true
    git checkout -- travis/linux/script.sh 2>/dev/null || true
    
    # Abortar cualquier merge en progreso
    git merge --abort 2>/dev/null || true
    
    log_message "✅ Cleanup completed" "$logfile"
}

# Función para obtener información del build
get_build_info() {
    local logfile="$1"
    
    log_message "Getting build information..." "$logfile"
    
    local commit_hash=$(git rev-parse HEAD | cut -c1-7)
    local branch_name=$(git rev-parse --abbrev-ref HEAD)
    local build_date=$(date)
    
    echo "Build Information:"
    echo "  Commit: $commit_hash"
    echo "  Branch: $branch_name"
    echo "  Date: $build_date"
    
    # Añadir información de versión a gcconfig.pri
    echo "DEFINES += GC_VERSION=\\\"\\\\(Release\\ $commit_hash\\\\)\\\"" >> src/gcconfig.pri
    
    log_message "✅ Build info added to gcconfig.pri" "$logfile"
}

# Mostrar ayuda
show_help() {
    echo "Usage: $0 [function] [arguments...]"
    echo ""
    echo "Available functions:"
    echo "  merge_branch <branch> <current_branch> <logfile>"
    echo "  check_branch_up_to_date <branch> <logfile>"
    echo "  prepare_build_environment <logfile>"
    echo "  cleanup_conflicting_files <logfile>"
    echo "  get_build_info <logfile>"
    echo "  log_message <message> <logfile>"
    echo "  exit_with_error <code> <message> <logfile>"
    echo ""
    echo "Example:"
    echo "  $0 merge_branch origin/master main /tmp/build.log"
    echo ""
}

# Main execution
if [[ $# -eq 0 ]]; then
    show_help
    exit 0
fi

# Ejecutar la función solicitada
case "$1" in
    merge_branch)
        if [[ $# -lt 4 ]]; then
            echo "Error: merge_branch requires 3 arguments"
            exit 1
        fi
        merge_branch "$2" "$3" "$4"
        ;;
    check_branch_up_to_date)
        if [[ $# -lt 3 ]]; then
            echo "Error: check_branch_up_to_date requires 2 arguments"
            exit 1
        fi
        check_branch_up_to_date "$2" "$3"
        ;;
    prepare_build_environment)
        if [[ $# -lt 2 ]]; then
            echo "Error: prepare_build_environment requires 1 argument"
            exit 1
        fi
        prepare_build_environment "$2"
        ;;
    cleanup_conflicting_files)
        if [[ $# -lt 2 ]]; then
            echo "Error: cleanup_conflicting_files requires 1 argument"
            exit 1
        fi
        cleanup_conflicting_files "$2"
        ;;
    get_build_info)
        if [[ $# -lt 2 ]]; then
            echo "Error: get_build_info requires 1 argument"
            exit 1
        fi
        get_build_info "$2"
        ;;
    log_message)
        if [[ $# -lt 3 ]]; then
            echo "Error: log_message requires 2 arguments"
            exit 1
        fi
        log_message "$2" "$3"
        ;;
    exit_with_error)
        if [[ $# -lt 4 ]]; then
            echo "Error: exit_with_error requires 3 arguments"
            exit 1
        fi
        exit_with_error "$2" "$3" "$4"
        ;;
    *)
        echo "Error: Unknown function '$1'"
        show_help
        exit 1
        ;;
esac
