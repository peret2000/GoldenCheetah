# Resumen de la GitHub Action para Nightly Build

## Archivos creados

### 1. `.github/workflows/nightly-build.yml`
**GitHub Action principal** que replica la funcionalidad de `build_project.sh`:

#### Características principales:
- **Ejecución automática:** Diariamente a las 2:00 AM UTC
- **Ejecución manual:** Disponible con parámetros configurables
- **Gestión de ramas:** Crea rama `NightlyBuild` y merge de múltiples ramas
- **Build completo:** Instalación de dependencias, compilación y empaquetado
- **Artefactos:** Subida automática de binarios y logs
- **Releases:** Creación automática de releases para AppImages

#### Parámetros disponibles:
- `appimage` (boolean, default: false) - Crear AppImage
- `fromscratch` (boolean, default: true) - Build desde cero
- `updatecode` (boolean, default: true) - Actualizar código

### 2. `scripts/build_helper.sh`
**Script auxiliar** con funciones reutilizables:

#### Funciones incluidas:
- `merge_branch` - Merge con manejo de errores
- `check_branch_up_to_date` - Verificar actualización de ramas
- `prepare_build_environment` - Configurar entorno de compilación
- `cleanup_conflicting_files` - Limpiar archivos conflictivos
- `get_build_info` - Extraer información del build
- `log_message` - Logging consistente
- `exit_with_error` - Manejo de errores

### 3. `scripts/test_nightly_build.sh`
**Script de testing** para probar componentes localmente:

#### Tests incluidos:
- Test de funciones de merge
- Test de preparación de build
- Test de funciones de cleanup
- Test de extracción de información del build
- Test de dependencias
- Test de sintaxis de GitHub Action

### 4. `.github/workflows/README.md`
**Documentación completa** con:
- Instrucciones de uso
- Configuración de secrets
- Troubleshooting
- Guía de personalización

## Configuración requerida

### Secrets en GitHub:
```
STRAVA_CLIENT_SECRET - Para integración con Strava
CLOUD_DB_BASIC_AUTH - Para autenticación con base de datos
CLOUD_DB_APP_NAME - Nombre de la aplicación para la base de datos
```

### Ramas que se mergean automáticamente:
- `origin/TrainButtons`
- `origin/MyZEW`
- `origin/VideoWidgets`
- `origin/SmoothPowerEstim`
- `origin/Strava`
- `origin/PyAutomatedProcessors`
- `origin/treadmill_qdomyos`
- `origin/utils`
- `origin/train_view_improvements`
- `origin/activities_view_improvements`
- `origin/train_geolocation_widget`
- `goldencheetah/master`
- `origin/tmp-equipment-management-feature`
- `paulj49457/origin-equipment-management-feature`

## Cómo usar

### 1. Ejecución automática
La acción se ejecuta automáticamente cada día a las 2:00 AM UTC.

### 2. Ejecución manual
1. Ve a la pestaña "Actions" en GitHub
2. Selecciona "Nightly Build"
3. Haz clic en "Run workflow"
4. Configura los parámetros según necesites

### 3. Testing local
```bash
# Ejecutar todos los tests
./scripts/test_nightly_build.sh --test-all

# Ejecutar tests específicos
./scripts/test_nightly_build.sh --test-merge
./scripts/test_nightly_build.sh --test-build
./scripts/test_nightly_build.sh --test-cleanup
```

## Diferencias con el script original

### ✅ Funcionalidades replicadas:
- Gestión de ramas y merges
- Compilación del proyecto
- Creación de AppImage
- Creación de binario con linuxdeployqt
- Logging de errores
- Limpieza de archivos conflictivos

### ❌ Funcionalidades NO incluidas:
- Notificaciones Pushover (específicas del entorno local)
- Logs acumulativos locales
- Gestión de archivos de log específicos del sistema

### ✅ Mejoras añadidas:
- Integración nativa con GitHub
- Artefactos automáticos
- Releases automáticos
- Mejor manejo de errores
- Documentación completa
- Scripts de testing

## Ventajas de la GitHub Action

1. **Automatización completa:** No requiere intervención manual
2. **Integración con GitHub:** Releases, artefactos y notificaciones automáticas
3. **Entorno controlado:** Siempre usa el mismo entorno de Ubuntu
4. **Escalabilidad:** Puede ejecutarse en paralelo si es necesario
5. **Historial:** Logs completos de cada ejecución
6. **Flexibilidad:** Parámetros configurables para diferentes tipos de build

## Próximos pasos

1. **Configurar secrets** en GitHub
2. **Probar la acción** con ejecución manual
3. **Ajustar horarios** si es necesario
4. **Personalizar ramas** según tus necesidades
5. **Añadir notificaciones** adicionales si se desea

## Troubleshooting

### Errores comunes:
1. **Secrets no configurados:** Verifica que los secrets estén disponibles
2. **Ramas no encontradas:** Algunas ramas pueden no existir
3. **Conflictos de merge:** La acción intenta resolverlos automáticamente
4. **Dependencias faltantes:** La acción instala todo lo necesario

### Para debug:
- Revisa los logs en la pestaña "Actions"
- Usa el script de testing local
- Verifica que todas las ramas existan
- Comprueba los secrets de GitHub

¡La GitHub Action está lista para usar! 🚀
