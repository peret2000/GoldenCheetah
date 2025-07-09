# GitHub Action para Nightly Build de GoldenCheetah

Este directorio contiene una GitHub Action que replica la funcionalidad del script `build_project.sh` para realizar builds nocturnos automáticos del proyecto GoldenCheetah.

## Archivos

- `nightly-build.yml` - GitHub Action principal
- `build_helper.sh` - Script auxiliar con funciones compartidas

## Funcionalidades

### GitHub Action (`nightly-build.yml`)

La acción realiza los siguientes pasos:

1. **Configuración del entorno:**
   - Checkout del repositorio con historial completo
   - Configuración de Git
   - Instalación de Qt6 y dependencias

2. **Gestión de ramas:**
   - Limpia archivos que pueden causar conflictos en merge
   - Crea/actualiza la rama `NightlyBuild` desde `MyBuildAdapt`
   - Verifica que `MyBuildAdapt` esté actualizada

3. **Merge de ramas de características:**
   - `TrainButtons`, `MyZEW`, `VideoWidgets`, `SmoothPowerEstim`
   - `Strava`, `PyAutomatedProcessors`, `treadmill_qdomyos`
   - `utils`, `train_view_improvements`, `activities_view_improvements`
   - `train_geolocation_widget`, `goldencheetah/master`
   - Características de gestión de equipamiento

4. **Compilación:**
   - Ejecuta `preparedirectory.sh` si es necesario
   - Ejecuta `travis/linux/script.sh` para compilar
   - Maneja errores de compilación

5. **Empaquetado:**
   - Crea binario con `linuxdeployqt` (por defecto)
   - Opcionalmente crea AppImage
   - Sube artefactos

6. **Release:**
   - Crea release automático si se genera AppImage
   - Incluye información del build

## Configuración

### Secrets requeridos

La acción requiere los siguientes secrets en GitHub:

- `STRAVA_CLIENT_SECRET` - Para integración con Strava
- `CLOUD_DB_BASIC_AUTH` - Para autenticación con base de datos en la nube
- `CLOUD_DB_APP_NAME` - Nombre de la aplicación para la base de datos

### Variables de entorno

- `GITHUB_TOKEN` - Token automático de GitHub (se proporciona automáticamente)

## Uso

### Ejecución automática

La acción se ejecuta automáticamente todos los días a las 2:00 AM UTC.

### Ejecución manual

Puedes ejecutar la acción manualmente desde la pestaña "Actions" en GitHub:

1. Ve a la pestaña "Actions"
2. Selecciona "Nightly Build"
3. Haz clic en "Run workflow"
4. Configura los parámetros:
   - **Create AppImage?** - `true` para crear AppImage, `false` para binario simple
   - **Build from scratch?** - `true` para build completo, `false` para incremental
   - **Update code from repository?** - `true` para actualizar código

### Parámetros disponibles

- `appimage` (boolean, default: false) - Crear AppImage en lugar de binario simple
- `fromscratch` (boolean, default: true) - Realizar build desde cero
- `updatecode` (boolean, default: true) - Actualizar código del repositorio

## Diferencias con el script original

### Funcionalidades replicadas

- ✅ Gestión de ramas y merge
- ✅ Compilación del proyecto
- ✅ Creación de AppImage
- ✅ Creación de binario con linuxdeployqt
- ✅ Logging de errores
- ✅ Limpieza de archivos conflictivos

### Funcionalidades no incluidas

- ❌ Notificaciones Pushover (específicas del entorno local)
- ❌ Logs acumulativos locales
- ❌ Gestión de archivos de log específicos del sistema local

### Mejoras añadidas

- ✅ Integración nativa con GitHub
- ✅ Artefactos automáticos
- ✅ Releases automáticos
- ✅ Mejor manejo de errores con anotaciones de GitHub
- ✅ Paralelización donde sea posible

## Troubleshooting

### Errores comunes

1. **Conflictos de merge:** La acción limpia automáticamente archivos que pueden causar conflictos
2. **Dependencias faltantes:** La acción instala automáticamente Qt6 y todas las dependencias necesarias
3. **Permisos:** Asegúrate de que los secrets estén configurados correctamente
4. **Rama no actualizada:** La acción verifica que `MyBuildAdapt` esté actualizada antes de continuar

### Logs y debug

- Los logs están disponibles en la pestaña "Actions" de GitHub
- Los artefactos incluyen el log de compilación
- Los errores se muestran como anotaciones en GitHub

## Migración desde el script local

Para migrar desde el uso del script `build_project.sh`:

1. Configura los secrets necesarios en GitHub
2. Ajusta los horarios de ejecución en `nightly-build.yml` si es necesario
3. Personaliza las ramas a mergear según tus necesidades
4. Considera usar el script auxiliar `build_helper.sh` para builds locales

## Personalización

### Cambiar ramas a mergear

Edita la sección "Merge feature branches" en `nightly-build.yml` para añadir/quitar ramas:

```yaml
# Añadir nueva rama
merge_branch origin/nueva-rama
```

### Cambiar horario de ejecución

Edita la sección `schedule` en `nightly-build.yml`:

```yaml
schedule:
  # Ejecutar a las 3:00 AM UTC en lugar de 2:00 AM
  - cron: '0 3 * * *'
```

### Personalizar notificaciones

Puedes añadir notificaciones personalizadas usando acciones de terceros como:
- Slack notifications
- Discord notifications
- Email notifications

## Contribución

Para contribuir a esta GitHub Action:

1. Haz fork del repositorio
2. Crea una rama para tu funcionalidad
3. Actualiza tanto la acción como este README
4. Crea un pull request

## Soporte

Si encuentras problemas con la GitHub Action:

1. Revisa los logs en la pestaña "Actions"
2. Comprueba que los secrets estén configurados
3. Verifica que las ramas requeridas existan
4. Consulta la documentación de GitHub Actions
