# HTML Charts en la vista Train de GoldenCheetah

> **Análisis técnico de la implementación**
>
> Documento de diseño y lectura del chart HTML embebido en la vista **Train**:
> clases, herencia, flujos, contrato JSON, decisiones de diseño, limitaciones y
> evolución posterior.

| Dato | Valor |
|---|---|
| Proyecto | GoldenCheetah |
| Ámbito | Vista **Train** → chart **HTML Chart** |
| Estado analizado | `b1b186390` (`Add RealtimeData symbols list for Telemetry in HTML charts`, PR #4895) |
| HEAD de referencia al escribir | `74df589d4` |
| Fecha del documento | 2026-11-05 |
| Fuentes principales | `src/Train/HtmlChart.*`, `src/Train/HtmlTrainingBridge.*`, `src/Core/Context.*`, `src/Gui/GcWindowRegistry.*`, `src/Train/RealtimeData.*`, `src/Metrics/HrZones.*`, `src/Metrics/Zones.*` |
| Carácter | Análisis técnico; no es documentación oficial de usuario |

> **Criterio de alcance.** El núcleo del análisis se fija en `b1b186390`, que es
> el punto en el que el diseño original ya está completo: chart, editor,
> configuración, puente C++/JavaScript, telemetría, ruta y fixture de prueba.
> Los commits posteriores se documentan por separado en la sección
> [17](#17-evolución-posterior-a-b1b186390) para no mezclar el diseño inicial con
> sus correcciones.

---

## Índice

1. [Resumen ejecutivo](#1-resumen-ejecutivo)
2. [Mapa de commits](#2-mapa-de-commits)
3. [Motivación y encaje en GoldenCheetah](#3-motivación-y-encaje-en-goldencheetah)
4. [Arquitectura general](#4-arquitectura-general)
5. [Modelo de clases y herencia](#5-modelo-de-clases-y-herencia)
6. [HtmlChart: vista, editor y configuración](#6-htmlchart-vista-editor-y-configuración)
7. [Bridges y API JavaScript](#7-bridges-y-api-javascript)
8. [Telemetría](#8-telemetría)
9. [Workout y ruta](#9-workout-y-ruta)
10. [Configuración y zonas](#10-configuración-y-zonas)
11. [Persistencia en perspectivas](#11-persistencia-en-perspectivas)
12. [Integración con RealtimeData, Zones y HrZones](#12-integración-con-realtimedata-zones-y-hrzones)
13. [Pruebas y fixture](#13-pruebas-y-fixture)
14. [Decisiones de diseño y motivaciones](#14-decisiones-de-diseño-y-motivaciones)
15. [Limitaciones y riesgos](#15-limitaciones-y-riesgos)
16. [Oportunidades de mejora](#16-oportunidades-de-mejora)
17. [Evolución posterior a b1b186390](#17-evolución-posterior-a-b1b186390)
18. [Conclusiones](#18-conclusiones)
- [Anexo A. Series de telemetría](#anexo-a-series-de-telemetría)
- [Anexo B. Referencias de código](#anexo-b-referencias-de-código)
- [Anexo C. Glosario](#anexo-c-glosario)

---

## 1. Resumen ejecutivo

La implementación añade un nuevo chart a la vista **Train** que permite escribir
HTML, CSS y JavaScript dentro de GoldenCheetah y alimentarlo con datos de la
sesión de entrenamiento. El chart no es un navegador genérico: es un
`GcChartWindow` que contiene un editor de HTML, una tabla de configuración y un
`QWebEngineView`, comunicados mediante `QWebChannel`.

Los puntos centrales son:

- **`HtmlChart`** es la vista. Hereda de `GcChartWindow` y, por tanto, participa
  del sistema de charts, perspectivas y serialización de GoldenCheetah.
- **`HtmlTrainingBridge`** es el puente global del `Context`, expuesto a
  JavaScript como `gc`. Entrega configuración, ruta planificada, telemetría,
  notificaciones y cambios de estado.
- **`HtmlChartBridge`** es un puente pequeño y específico de cada chart,
  expuesto como `chart`. Solo permite leer la configuración key/value del chart.
- La telemetría se construye recorriendo `RealtimeData::listDataSeries()` y
  usando `RealtimeData::seriesSymbol()` como clave JSON. Se emite como máximo a
  **1 Hz**, aunque la fuente puede actualizar a **5 Hz**.
- La ruta se entrega como un objeto con tres partes: `workout` (metadatos y
  perfil ERG), `geojson` (líneas y marcadores cada 5 km) y `elevProfile`
  (perfil de elevación y pendiente).
- La configuración de zonas se limita a **ciclismo** y usa las zonas válidas
  para la **fecha actual**, no necesariamente las del workout.
- El HTML se recarga por completo con `QWebEnginePage::setHtml()` cuando cambia
  el texto, se edita la configuración en la tabla, cambia el workout o ciertos
  filtros. Es simple y robusto, pero destruye el estado JavaScript de la página.

El diseño es un MVP bien delimitado: separa vista, puente y fuente de datos, y
define un contrato JSON fácil de consumir. Sus principales deudas son la
**falta de estado inicial/replay**, la **recarga completa como mecanismo de
actualización**, la **ausencia de unidades** en el contrato y la **persistencia
incompleta** de algunas propiedades.

---

## 2. Mapa de commits

| Commit | Fecha | PR | Cambio | Relevancia |
|---|---|---:|---|---|
| `b6e7f9305` | 2026-06-18 | #4886 | HTML Chart in train view | Implementación inicial: chart, editor, configuración, puentes, registro y build. `+1074/−4` en 9 ficheros. |
| `7c1290e4a` | 2026-06-22 | #4888 | `HrZones::getZoneDescriptions()` | Completa la API de zonas para que el puente pueda exponer descripciones de FC. |
| `ecdcc72c5` | 2026-06-23 | #4889 | Telemetría y configuración ampliadas | Añade perfil ERG, colores/descripciones de zonas, notificaciones, `is_dark_background` y campos de telemetría. |
| `e064b7180` | 2026-07-03 | #4893 | `RealtimeData::value()`, `seriesName()`, `seriesSymbol()` | Añade `Wbal` al valor y nombres/símbolos de `LeftPCO`/`RightPCO`. |
| `b1b186390` | 2026-07-07 | #4895 | Lista de símbolos para telemetría | Sustituye campos fijos por `listDataSeries()` + `seriesSymbol()`; introduce `Current Position`; añade fixture. |
| `a23009ab6` | 2026-07-24 | #4902 | `CTRAINPLOTBACKGROUND` | Corrige el origen de `is_dark_background` para que coincida con el fondo real del chart. |
| `ef01151a5` | 2026-08-09 | #4911 | Distancia de `elevProfile` | Usa `ErgFilePoint::x` en vez de Haversine para que la ruta coincida con la telemetría. |
| `4c6430a2c` | 2026-08-23 | #4913 | Plantilla HTML por defecto | Sustituye la plantilla mínima por una genérica con metadatos y rejilla de telemetría. |
| `ed32e98dc` | 2026-10-28 | #4929 | Video + HTML | Añade `VideoHtmlWindow`, que reutiliza el mismo puente `gc`. |

La secuencia temporal muestra una evolución clara:

```text
b6e7f9305  chart + puente + configuración
    │
    ├── 7c1290e4a  completar API de zonas
    ├── ecdcc72c5  ampliar telemetría/configuración
    ├── e064b7180  completar valores/símbolos
    └── b1b186390  generalizar telemetría con listDataSeries
             │
             ├── a23009ab6  corregir flag de tema
             ├── ef01151a5  corregir distancias de ruta
             ├── 4c6430a2c  mejorar plantilla por defecto
             └── ed32e98dc  reutilizar el puente en Video + HTML
```

---

## 3. Motivación y encaje en GoldenCheetah

### 3.1 El problema

La vista **Train** ya ofrece charts especializados: telemetría, potencia,
cadencia, frecuencia cardiaca, workout, mapa, vídeo, etc. Añadir una visualización
nueva implicaba tradicionalmente escribir C++ y registrarla en el sistema de
charts. Eso es adecuado para funcionalidad estable, pero no para prototipos,
dashboards personales o integraciones con tecnologías web.

El chart HTML resuelve ese problema con una idea sencilla:

> Dar al usuario un documento HTML completo y un canal de datos desde C++.

El usuario puede construir con HTML/CSS/JS:

- un dashboard con tarjetas y gauges;
- un mapa o perfil de ruta;
- un overlay para vídeo o entrenamiento;
- una visualización específica de un entrenador o protocolo;
- una página que consuma APIs externas.

### 3.2 Por qué un `GcChartWindow`

`HtmlChart` hereda de `GcChartWindow`, no de `QWebEngineView`. Así obtiene:

- integración con el layout de la perspectiva;
- título, subtítulo, estilo, tamaño y botones de chart;
- serialización de propiedades de usuario;
- exportación como imagen/chart;
- el menú de configuración del chart.

El `QWebEngineView` queda como un widget interno, no como la identidad del chart.

### 3.3 Por qué no un `WebPageWindow`

Un `WebPageWindow` está pensado para cargar una URL. El chart HTML, en cambio:

- almacena el HTML dentro de la propia configuración del chart;
- lo edita el usuario en el propio GoldenCheetah;
- inyecta un puente de datos C++/JS;
- forma parte de la perspectiva Train.

Son dos casos de uso distintos aunque compartan `QWebEngineView` y el perfil web.

---

## 4. Arquitectura general

### 4.1 Capas

```text
┌──────────────────────────────────────────────────────────────┐
│ Página HTML del usuario                                      │
│                                                              │
│   window.gc        → HtmlTrainingBridge (global del Context) │
│   channel.objects.chart → HtmlChartBridge (por chart)        │
└───────────────────────────────┬──────────────────────────────┘
                                │ QtWebChannel
                                │ (métodos Q_INVOKABLE y señales Qt)
┌───────────────────────────────▼──────────────────────────────┐
│ Puentes C++                                                  │
│                                                              │
│   HtmlTrainingBridge                  HtmlChartBridge        │
│   · getConfig()                       · getChartConfig()     │
│   · getPlannedRoute()                                        │
│   · telemetry(QString)                                       │
│   · plannedRouteChanged(QString)                             │
│   · stateChanged(QString)                                    │
└───────────────────────────────┬──────────────────────────────┘
                                │ señales/slots de Qt
┌───────────────────────────────▼──────────────────────────────┐
│ Context                                                      │
│   · telemetryUpdate(RealtimeData)                            │
│   · ergFileSelected(ErgFile*)                                │
│   · start / pause / unpause / stop                           │
│   · setNotification / clearNotification                      │
│   · athlete, zones, hrZones, currentErgFile(), getNow()      │
└───────────────────────────────┬──────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────┐
│ Fuentes de datos                                             │
│   RealtimeData · ErgFile · Zones · HrZones · Colors          │
└──────────────────────────────────────────────────────────────┘
```

### 4.2 Propiedad y ciclo de vida

```text
Context
 │
 ├── m_HtmlTrainingBridge  (lazy, parent = Context)
 │       └── expuesto como "gc" en cada QWebChannel
 │
 └── ...

HtmlChart : GcChartWindow
 │
 ├── QSplitter (vertical)
 │    ├── QTextEdit          editor HTML
 │    ├── QWidget            configuración key/value
 │    └── QWebEngineView     canvas
 │         └── QWebEnginePage (perfil compartido del Context)
 │              └── QWebChannel
 │                   ├── "gc"    → HtmlTrainingBridge del Context
 │                   └── "chart" → HtmlChartBridge de este chart
 │
 ├── QTimer m_renderTimer   debounce de 1 s para el editor
 ├── QCheckBox showEditorBtn
 ├── QCheckBox showConfigBtn
 ├── QCheckBox showTitleBtn
 └── QTableWidget configTable
```

Consecuencias importantes:

- El puente `gc` **no es propiedad del chart**: vive en el `Context` y se crea
  de forma perezosa la primera vez que un chart HTML lo pide.
- Si se abren varios charts HTML, todos comparten el mismo `gc` y reciben las
  mismas señales. Cada uno tiene su propio `chart`.
- El puente sobrevive al cierre del chart, pero no al `Context`.
- `HtmlChartBridge` tiene como padre el propio `HtmlChart`, por lo que su vida
  está ligada a la del chart.
- Todo el flujo está en el hilo GUI: generación de JSON, procesamiento de ruta y
  llamadas de QtWebChannel.

### 4.3 Por qué el puente vive en `Context`

La decisión tiene ventajas claras:

1. **Una sola fuente de verdad** para telemetría, ruta y estado.
2. **Reutilización**: varios charts HTML pueden consumir los mismos datos.
3. **Supervivencia al chart**: cerrar y reabrir el chart no recrea el puente.
4. **Extensibilidad**: otros consumidores, como `VideoHtmlWindow`, pueden
   registrarlo sin duplicar lógica.

El coste es que el puente acumula estado compartido (`m_plannedRoute`,
`m_currentMessage`, última muestra pendiente) y que las señales se difunden a
todas las páginas conectadas.

---

## 5. Modelo de clases y herencia

### 5.1 Herencia

```text
QObject
 └── QFrame
      └── GcWindow
           └── GcChartWindow
                └── HtmlChart

QObject
 ├── HtmlChartBridge
 └── HtmlTrainingBridge
```

`HtmlChart` hereda de `GcChartWindow`. En la declaración incluye `G_OBJECT`,
pero conviene precisar qué es: en `src/Charts/GoldenCheetah.h` ese macro está
definido como **vacío** (`#define G_OBJECT`) y la definición que lo convertía en
una propiedad `instanceName` está comentada. Por tanto, no aporta metapropiedades
ni persistencia por sí mismo; es una convención heredada del código de charts.
La persistencia real la aportan las `Q_PROPERTY(..., USER true)` declaradas
justo después.

### 5.2 Clases y responsabilidades

| Clase | Fichero | Responsabilidad |
|---|---|---|
| `HtmlChart` | `src/Train/HtmlChart.h/.cpp` | Chart completo: editor, tabla de configuración, `QWebEngineView`, `QWebChannel`, aplicación del HTML, paleta y título. |
| `HtmlChartBridge` | `src/Train/HtmlChart.h/.cpp` | Objeto mínimo expuesto como `chart`; solo entrega la configuración del chart. |
| `HtmlTrainingBridge` | `src/Train/HtmlTrainingBridge.h/.cpp` | Puente global expuesto como `gc`; configuración, ruta, telemetría, notificaciones y estado. |
| `Context` | `src/Core/Context.h/.cpp` | Dueño perezoso de `HtmlTrainingBridge`; fuente de señales y acceso a atleta/zonas. |
| `GcWindowRegistry` | `src/Gui/GcWindowRegistry.h/.cpp` | Registra el tipo `HtmlTraining = 58` y crea `HtmlChart`. |
| `RealtimeData` | `src/Train/RealtimeData.h/.cpp` | Enum de series, valores, nombres, símbolos y lista de series a exportar. |
| `Zones` / `HrZones` | `src/Metrics/Zones.*`, `src/Metrics/HrZones.*` | Zonas de potencia y frecuencia cardiaca, incluidas descripciones y colores. |

### 5.3 Ficheros afectados por la implementación inicial

| Fichero | Cambio |
|---|---|
| `src/Core/Context.h` | Declaración de `getHtmlTrainingBridge()` y miembro `m_HtmlTrainingBridge`. |
| `src/Core/Context.cpp` | Creación perezosa del puente, inicialización a `nullptr` y `#include`. |
| `src/Gui/GcWindowRegistry.h` | Nuevo `GcWindowTypes::HtmlTraining = 58`. |
| `src/Gui/GcWindowRegistry.cpp` | Entrada “HTML Chart” y factoría `new HtmlChart(context)`. |
| `src/Train/HtmlChart.h` | Clase `HtmlChart`, `HtmlChartBridge` y propiedades persistidas. |
| `src/Train/HtmlChart.cpp` | Construcción de UI, editor, tabla, WebEngine, WebChannel, HTML por defecto y paleta. |
| `src/Train/HtmlTrainingBridge.h` | API `gc`, señales y estado interno. |
| `src/Train/HtmlTrainingBridge.cpp` | Configuración, ruta, telemetría, notificaciones y estados. |
| `src/src.pro` | Inclusión de headers y sources; módulos web ya existentes. |

---

## 6. HtmlChart: vista, editor y configuración

### 6.1 Construcción paso a paso

El constructor de `HtmlChart` hace, en este orden:

1. Inicializa `GcChartWindow` y el puntero al `Context`.
2. Crea `m_renderTimer`, un `QTimer` de un solo disparo conectado a
   `applyHtml()`.
3. Crea el widget de controles del chart con tres checkboxes:
   - **Show Editor** (activado por defecto),
   - **Show Config** (desactivado por defecto),
   - **Show Title** (activado por defecto).
4. Conecta los checkboxes con sus slots.
5. Conecta señales del `Context` a `applyHtml()`:
   - `ergFileSelected(ErgFile*)`,
   - `filterChanged()`,
   - `homeFilterChanged()`.
6. Crea el layout principal y un `QSplitter` vertical.
7. Añade al splitter:
   - el editor `QTextEdit`,
   - el widget de configuración,
   - el `QWebEngineView`.
8. Configura el editor: texto plano, sin marco, fuente Courier.
9. Configura la tabla de configuración: dos columnas, `Key` y `Value`.
10. Crea el `QWebEngineView`, le asigna una `QWebEnginePage` con el perfil
    compartido del `Context` y configura el `QWebChannel`.
11. Registra `gc` y `chart` en el canal.
12. Ajusta márgenes, zoom (`dpiXFactor`) y política de tamaño.
13. Fija tamaños iniciales del splitter: `200 / 100 / 600`.
14. Aplica la paleta y conecta `configChanged`.
15. Carga la plantilla HTML por defecto con `setHtml()`.
16. Aplica la visibilidad inicial de editor y configuración.

> **Nota:** los checkboxes no están dentro del chart. Se colocan en el widget de
> controles que GoldenCheetah muestra en el panel/diálogo de configuración del
> chart.

### 6.2 Jerarquía visual

```text
HtmlChart (GcChartWindow)
└── QVBoxLayout principal
    └── QSplitter vertical
        ├── QTextEdit “editor”              ← HTML editable
        ├── QWidget “configWidget”          ← tabla key/value
        │    ├── QHBoxLayout
        │    │    ├── QPushButton Add
        │    │    └── QPushButton Remove
        │    └── QTableWidget (Key, Value)
        └── QWebEngineView “canvas”         ← render HTML
```

El splitter es **vertical**: el editor queda arriba, la configuración en medio y
la vista web abajo. Los tamaños iniciales son una sugerencia; el usuario puede
arrastrar el divisor. La configuración está oculta inicialmente, por lo que el
espacio visible se reparte entre editor y canvas.

### 6.3 Flujo de renderizado

```text
                         ┌─────────────────────────────┐
                         │ QTextEdit::textChanged      │
                         └──────────────┬──────────────┘
                                        │ start(1000)
                                        ▼
                         ┌─────────────────────────────┐
                         │ m_renderTimer (single shot) │
                         └──────────────┬──────────────┘
                                        │ timeout
                                        ▼
┌──────────────────┐          ┌─────────────────────────┐
│ setHtml(texto)   │─────────▶│ applyHtml()             │
└──────────────────┘          │  · lee el editor        │
                              │  · inyecta document.title│
                              │  · page->setHtml(...)   │
                              └────────────┬────────────┘
                                           │
┌──────────────────┐                       │
│ configTableChanged│──────────────────────┤
└──────────────────┘                       │
                                           │
┌──────────────────┐                       │
│ ergFileSelected  │──────────────────────┤
│ filterChanged    │                       │
│ homeFilterChanged│                       │
└──────────────────┘                       │
                                           ▼
                              ┌─────────────────────────┐
                              │ QWebEnginePage::setHtml  │
                              │ base URL: qrc:/          │
                              └─────────────────────────┘
```

`applyHtml()` hace esencialmente esto:

```cpp
currentHtml = editor->toPlainText();

QString chartTitle = title();
if (chartTitle.isEmpty()) chartTitle = "Html Chart";

// escapa \, ', ", < y >
QString injectScript =
    QString("<script>document.title = '%1';</script>").arg(chartTitle);

canvas->page()->setHtml(currentHtml + injectScript, QUrl("qrc:/"));
```

La base `qrc:/` es necesaria para que el HTML pueda cargar
`qrc:///qtwebchannel/qwebchannel.js`, que es el script que implementa el extremo
JavaScript de `QWebChannel`.

### 6.4 El título del documento

El chart inyecta un pequeño script para fijar `document.title` con el título del
chart. El objetivo es identificarlo fácilmente en el navegador de depuración
remota de QtWebEngine, donde todas las páginas podrían aparecer como “Html
Chart”.

Se escapan:

- `\` → `\\`
- `'` → `\'`
- `"` → `\"`
- `<` → `\x3C`
- `>` → `\x3E`

No se escapan saltos de línea. Además, el script se concatena **después** de
`</html>`, lo que los navegadores toleran pero no es HTML estrictamente
canónico. Son detalles menores, pero conviene tenerlos presentes si se
automatiza la generación del título.

### 6.5 Editor HTML

| Aspecto | Comportamiento |
|---|---|
| Tipo | `QTextEdit` |
| Texto enriquecido | Desactivado (`setAcceptRichText(false)`) |
| Fuente | Courier, tamaño de la fuente por defecto |
| Marco | Sin marco |
| Actualización | Debounce de 1000 ms tras el último cambio |
| Persistencia | `Q_PROPERTY(QString html)` |
| Aplicación | Inmediata al llamar a `setHtml()`; automática tras el debounce |

El debounce evita recargar la página en cada pulsación. El coste es que el
usuario debe esperar aproximadamente un segundo desde que deja de escribir para
ver el resultado.

`setHtml()` hace dos cosas:

1. Copia el texto al editor (`setPlainText`).
2. Llama inmediatamente a `applyHtml()`.

Como `setPlainText()` también puede disparar `textChanged`, es habitual que se
produzca una segunda recarga diferida un segundo después. Es una pequeña
ineficiencia, no un fallo funcional.

### 6.6 Tabla de configuración

La tabla `configTable` tiene dos columnas:

| Columna | Contenido |
|---|---|
| `Key` | Nombre de la clave JSON |
| `Value` | Valor, siempre tratado como texto |

Operaciones:

- **Add**: inserta una fila `new_key` / `value`.
- **Remove**: elimina la fila seleccionada.
- **itemChanged**: reconstruye el objeto JSON completo.

La reconstrucción:

```cpp
QJsonObject obj;
for (int i = 0; i < configTable->rowCount(); ++i) {
    QString key = keyItem->text().trimmed();
    if (!key.isEmpty()) {
        obj[key] = valItem->text();
    }
}
currentChartConfig =
    QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
applyHtml();
```

Consecuencias:

- Las claves vacías se ignoran.
- Si hay claves duplicadas, la última fila gana.
- **Todos los valores son strings** en el JSON resultante, aunque el usuario
  escriba `123`, `true` o `null`.
- `QJsonObject` no conserva el orden de inserción; la tabla se repuebla en el
  orden que use Qt al iterar el objeto (habitualmente ordenado por clave).
- La tabla no admite valores anidados de forma nativa. Si se pega un JSON con
  arrays u objetos, `toVariant().toString()` produce una cadena vacía y el valor
  se pierde al volver a serializar.

`setChartConfigString()` hace el camino inverso:

- vacía la tabla;
- parsea el JSON;
- si es un objeto, crea una fila por clave;
- convierte cada valor a string con `it.value().toVariant().toString()`.

No valida que el JSON sea correcto ni que sea un objeto. Si el string no es
JSON válido, la tabla queda vacía pero `currentChartConfig` conserva el texto
inválido. `getChartConfig()` devuelve `"{}"` solo cuando el string está vacío,
no cuando es inválido.

### 6.7 Visibilidad del editor, configuración y título

| Control | Efecto | Persistencia |
|---|---|---|
| `showEditorBtn` | Muestra/oculta el `QTextEdit` | Sí, `showEditor` |
| `showConfigBtn` | Muestra/oculta el widget de configuración | **No** |
| `showTitleBtn` | Muestra/oculta el título y ajusta márgenes | Sí, `showTitleBar` |

`showEditorChanged()` y `showConfigChanged()` son simples llamadas a
`setVisible()`. `showTitleChanged()` es más complejo porque el título lo pinta
`GcWindow` en el margen superior del chart.

Flujo de `setTitleVisible()`:

1. Bloquea las señales del checkbox.
2. Sincroniza el checkbox.
3. Llama a `GcChartWindow::setShowTitle(visible)`.
4. Si se muestra:
   - usa `m_savedTopMargin` o un fallback de 15 px;
   - aplica `setContentsMargins(0, margen, 0, 0)`.
5. Si se oculta:
   - guarda el margen superior actual si es mayor que 0;
   - aplica `setContentsMargins(0, 0, 0, 0)`.
6. Llama a `update()`.

Además, `HtmlChart::event()` intercepta `ContentsRectChange`, `Resize`, `Show` y
`LayoutRequest`. Si el título está oculto y aún queda margen superior, lo guarda
y lo pone a cero. Es un workaround para evitar una banda vacía cuando el layout
recalcula los márgenes.

Detalles menores:

- El fallback de 15 px no está escalado por DPI.
- `m_savedTopMargin` es estado de instancia, no una propiedad persistida.
- `isTitleVisible()` lee el checkbox, no directamente `GcWindow::showTitle()`.

### 6.8 Paleta y tema

`configChanged()`:

```cpp
QColor bgcolor = GColor(CTRAINPLOTBACKGROUND);
setProperty("color", bgcolor);

QPalette palette;
palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
palette.setColor(QPalette::Base,
                 GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
setPalette(palette);
```

Se aplica al chart y al editor. La tabla de configuración hereda la paleta del
widget contenedor.

Puntos importantes:

- El color de fondo del chart es `CTRAINPLOTBACKGROUND`.
- La paleta del editor usa `CPLOTBACKGROUND`.
- No son necesariamente el mismo color; el editor puede tener un fondo distinto
  al del chart.
- **El contenido web no se estiliza automáticamente.** `is_dark_background` es
  solo un flag que el HTML puede consultar. La plantilla por defecto muestra el
  valor, pero no cambia su CSS en función de él.
- El flag se introdujo en `ecdcc72c5` usando `CPLOTBACKGROUND` y se corrigió en
  `a23009ab6` para usar `CTRAINPLOTBACKGROUND`, que es el fondo real del chart.

### 6.9 Propiedades del chart

| Propiedad | Tipo | Getter / Setter | `USER` | Uso |
|---|---|---|---:|---|
| `html` | `QString` | `getHtml` / `setHtml` | Sí | Código HTML del editor. |
| `splitterState` | `QByteArray` | `getSplitterState` / `setSplitterState` | Sí | Pretende guardar el estado del splitter. |
| `showEditor` | `bool` | `isEditorVisible` / `setEditorVisible` | Sí | Visibilidad del editor. |
| `chartConfig` | `QString` | `getChartConfigString` / `setChartConfigString` | Sí | Configuración key/value serializada como JSON. |
| `showTitleBar` | `bool` | `isTitleVisible` / `setTitleVisible` | Sí | Visibilidad del título. |

Propiedades base relevantes, heredadas de `GcWindow`/`GcChartWindow`:

- `title`, `subtitle`;
- `style`, `widthFactor`, `heightFactor`, `resizable`;
- `color` (no es `USER`, se gestiona desde el chart).

No existe una propiedad para `showConfig`; por eso la visibilidad del panel de
configuración no se guarda.

---

## 7. Bridges y API JavaScript

### 7.1 `HtmlChartBridge` (`window.chart`)

Es un objeto mínimo, propiedad del chart:

```cpp
class HtmlChartBridge : public QObject
{
    Q_OBJECT
public:
    explicit HtmlChartBridge(HtmlChart *chart, QObject *parent = nullptr);
    Q_INVOKABLE QString getChartConfig() const;

private:
    HtmlChart *m_chart;
};
```

Su única función es evitar exponer el `HtmlChart` completo a JavaScript. Un
documento HTML solo puede leer la configuración del chart, no cambiar el editor,
el splitter ni otras propiedades internas.

`getChartConfig()` devuelve:

- `"{}"` si `currentChartConfig` está vacío;
- el JSON compacto actual en caso contrario.

### 7.2 `HtmlTrainingBridge` (`window.gc`)

Es el puente global del `Context`. Su API pública es pequeña:

| Miembro | Tipo | Descripción |
|---|---|---|
| `getConfig()` | `Q_INVOKABLE QString` | Configuración de atleta, zonas, límites y tema, como JSON. |
| `getPlannedRoute()` | `Q_INVOKABLE QString` | Ruta/workout actual como JSON, o string vacío si no hay ruta. |
| `telemetry(QString)` | señal | Muestra de telemetría a ~1 Hz. |
| `plannedRouteChanged(QString)` | señal | Cambio de workout/ruta. |
| `stateChanged(QString)` | señal | `start`, `pause`, `resume` o `stop`. |

No expone setters. JavaScript no puede modificar el estado del entrenamiento a
través del puente.

### 7.3 Registro en `QWebChannel`

En el constructor de `HtmlChart`:

```cpp
m_webChannel = new QWebChannel(this);

HtmlTrainingBridge *bridge = context->getHtmlTrainingBridge();
if (bridge) {
    m_webChannel->registerObject("gc", bridge);
}

HtmlChartBridge *chartBridge = new HtmlChartBridge(this, this);
m_webChannel->registerObject("chart", chartBridge);

canvas->page()->setWebChannel(m_webChannel);
```

| Nombre | Objeto | Ámbito |
|---|---|---|
| `gc` | `HtmlTrainingBridge` | Compartido por todos los charts del `Context`. |
| `chart` | `HtmlChartBridge` | Específico de cada `HtmlChart`. |

La página debe cargar `qwebchannel.js` y crear el canal:

```js
new QWebChannel(qt.webChannelTransport, function (channel) {
    const gc = channel.objects.gc;
    const chart = channel.objects.chart;

    gc.getConfig(function (json) {
        const config = JSON.parse(json);
        // ...
    });

    chart.getChartConfig(function (json) {
        const chartConfig = JSON.parse(json);
        // ...
    });

    gc.telemetry.connect(function (json) {
        const data = JSON.parse(json);
        // ...
    });

    gc.plannedRouteChanged.connect(function (json) {
        const route = json ? JSON.parse(json) : null;
        // ...
    });

    gc.stateChanged.connect(function (state) {
        // state: "start" | "pause" | "resume" | "stop"
    });
});
```

> **Convención de QtWebChannel:** los métodos remotos que devuelven un valor
> reciben un callback como último argumento. Las señales se consumen con
> `.connect(callback)`. Los valores transportados aquí son cadenas; la mayoría
> son JSON, salvo `stateChanged`, que es un estado plano.

### 7.4 Contrato de datos

| API | Formato | Garantía |
|---|---|---|
| `gc.getConfig()` | JSON object | Siempre JSON válido; puede ser `{}` o contener solo los campos disponibles. |
| `gc.getPlannedRoute()` | JSON object o `""` | **No siempre es JSON válido**: el caso “sin workout” devuelve string vacío. |
| `gc.telemetry` | JSON object | Siempre JSON válido. |
| `gc.plannedRouteChanged` | JSON object o `""` | Igual que `getPlannedRoute()`. |
| `gc.stateChanged` | string | Uno de los cuatro estados; no es JSON. |
| `chart.getChartConfig()` | JSON object | `"{}"` si está vacío; puede devolver JSON inválido si se cargó así. |

Recomendación práctica para JavaScript:

```js
function parseMaybeJson(value) {
    if (!value) return null;
    try { return JSON.parse(value); }
    catch (e) {
        console.warn("JSON inválido:", e, value);
        return null;
    }
}
```

### 7.5 Nombres de las claves

Las claves de telemetría provienen de `RealtimeData::seriesSymbol()`. Son
nombres en inglés, estables y no traducidos, pero **contienen espacios y
apóstrofos**:

```js
const power = data["Power"];
const heartRate = data["Heart Rate"];
const wbal = data["W' bal"];
const leftRight = data["Left/Right Balance"];
```

No se debe usar notación de punto para estas claves.

La nomenclatura es heterogénea:

- `Power`, `Heart Rate`, `Target Power` (Title Case, con espacios);
- `Current Position`, `notification_text` (mezcla de espacios y snake_case);
- `max_power_w`, `is_dark_background` (snake_case);
- `zoneLows`, `zoneNames`, `maxHr` (camelCase).

Es funcional, pero convendría estabilizar un esquema de nombres si el API se
considera público.

---

## 8. Telemetría

### 8.1 Origen de los datos

`TrainSidebar` actualiza la telemetría con un `gui_timer` de **200 ms**
(`REFRESHRATE`) y emite:

```cpp
context->notifyTelemetryUpdate(rtData);
```

Eso produce hasta **5 actualizaciones por segundo**. El puente impone su propio
throttle de **1 segundo**, por lo que la página recibe como máximo una muestra
por segundo.

### 8.2 Construcción de la muestra

`HtmlTrainingBridge::onTelemetryUpdate()`:

```cpp
QJsonObject tel;

for (RealtimeData::DataSeries series : RealtimeData::listDataSeries()) {
    if (series == RealtimeData::None) continue;
    tel[RealtimeData::seriesSymbol(series)] = rt.value(series);
}

qint64 currentPosition = m_context ? m_context->getNow() : 0;
tel["Current Position"] = static_cast<double>(currentPosition);
tel["notification_text"] = m_currentMessage;

m_pendingTelemetrySample =
    QString::fromUtf8(QJsonDocument(tel).toJson(QJsonDocument::Compact));
m_hasPendingTelemetry = true;

if (!m_telemetryThrottle->isActive()) {
    m_telemetryThrottle->start();
}
```

Puntos clave:

- Se recorren **todas** las series de `listDataSeries()`.
- `None` se omite.
- La clave es `seriesSymbol(series)`, no `seriesName(series)`.
- El valor es `rt.value(series)`.
- Se añaden dos claves extra: `Current Position` y `notification_text`.
- El JSON se serializa en formato compacto.
- La muestra se guarda como *pendiente*; no se emite inmediatamente.

### 8.3 Throttle de 1 Hz

```text
telemetryUpdate(rt)  ──▶ construir JSON
                         pending = JSON
                         hasPending = true
                              │
                              │ si el timer no está activo
                              ▼
                    m_telemetryThrottle.start(1000)
                              │
                              │ timeout
                              ▼
                    onTelemetryThrottle()
                         ├── si hasPending:
                         │      emit telemetry(pending)
                         │      hasPending = false
                         └── timer.stop()
```

Características:

- Es un throttle **con última muestra**: si llegan cinco actualizaciones durante
  el segundo, solo se emite la última.
- La primera muestra se retrasa hasta el primer timeout (aproximadamente 1 s).
- Después de emitir, el timer se detiene. La siguiente muestra vuelve a
  arrancarlo.
- No es un `singleShot`, pero el slot lo detiene explícitamente.
- El JSON se construye en cada actualización de entrada (5 Hz), aunque solo se
  emita una de cada cinco. Es un pequeño desperdicio de CPU.

### 8.4 Ejemplo de telemetría

Ejemplo reducido (el objeto real incluye todas las series de la lista):

```json
{
  "Altitude": 650,
  "Cadence": 90,
  "Current Position": 4921,
  "Distance": 2.34,
  "Heart Rate": 150,
  "Lap": 1,
  "Power": 250,
  "Target Power": 260,
  "Time": 59824,
  "W' bal": 15000,
  "notification_text": "Next interval in 2 minutes"
}
```

El orden real de las claves no está garantizado por el contrato; no se debe
depender de él.

### 8.5 `Current Position`: el campo ambiguo

`Current Position` se calcula con:

```cpp
qint64 currentPosition = m_context ? m_context->getNow() : 0;
```

Pero `Context::getNow()` tiene **dos semánticas distintas** según el modo de
entrenamiento:

| Tipo de workout | Valor de `Current Position` | Unidad |
|---|---|---|
| ERG | `load_msecs`, tiempo transcurrido del workout | milisegundos |
| Slope / GPS | `displayWorkoutDistance * 1000` | metros |

Por eso el nombre es engañoso: no es una posición universal, sino un escalar
cuyo significado depende del modo. En un workout ERG representa tiempo; en uno
de pendiente o GPS representa distancia.

El cambio se introdujo en `b1b186390` sustituyendo el antiguo
`erg_time_msecs`. La intención era disponer de un campo común para ERG y slope,
pero el contrato no incluye el modo ni la unidad.

Para comparar con otros datos:

- ERG: `Current Position / 1000.0` → segundos.
- GPS: `Current Position / 1000.0` → kilómetros.
- El perfil `workout.profile` usa milisegundos en `x`.
- El `elevProfile` usa kilómetros en `x`.

Cualquier consumidor necesita saber el tipo de workout para interpretarlo. Una
mejora evidente sería añadir `workout_mode` y/o campos separados
`elapsed_msecs` y `route_distance_m`.

### 8.6 Notificaciones

El puente escucha:

- `Context::setNotification(QString, int)`:
  - guarda el mensaje en `m_currentMessage`;
  - si `timeout > 0`, arranca `m_notificationTimer` en segundos;
  - si `timeout <= 0`, detiene el timer pero mantiene el mensaje.
- `Context::clearNotification()`:
  - limpia el mensaje;
  - detiene el timer.

El mensaje se incluye en cada muestra como `notification_text`.

El timer de notificación es independiente del de telemetría:

- al expirar, pone `m_currentMessage = ""`;
- no emite una muestra inmediata;
- el siguiente sample de telemetría llevará el texto vacío.

### 8.7 Estados

Las señales de estado son:

| Señal del `Context` | Valor emitido |
|---|---|
| `start` | `"start"` |
| `pause` | `"pause"` |
| `unpause` | `"resume"` |
| `stop` | `"stop"` |

`onStop()` además:

- descarta la muestra pendiente;
- detiene el throttle;
- limpia `m_currentMessage`;
- emite `stateChanged("stop")`.

No se emite un estado inicial al cargar la página. Si el chart se abre o se
recarga mientras el entrenamiento ya está en marcha, la página no recibe
`"start"` y su plantilla puede seguir mostrando el estado por defecto
(“Stopped”). Es una de las limitaciones más visibles del diseño.

---

## 9. Workout y ruta

### 9.1 Disparador

`HtmlTrainingBridge` escucha:

```cpp
connect(m_context,
        static_cast<void (Context::*)(ErgFile*)>(&Context::ergFileSelected),
        this,
        &HtmlTrainingBridge::onErgFileSelected);
```

El `static_cast` es necesario porque `Context` tiene señales sobrecargadas para
`ErgFile*` y `ErgFileBase*`.

Si `file == nullptr`:

```cpp
m_plannedRoute = QStringLiteral("");
emit plannedRouteChanged(m_plannedRoute);
```

Es decir, “sin ruta” se representa con **string vacío**, no con un JSON vacío.

Si hay fichero, se construye un objeto con tres bloques:

```text
{
  "workout":    { ... },
  "geojson":    { ... },
  "elevProfile": [ ... ]
}
```

### 9.2 Bloque `workout`

Siempre incluye:

| Campo | Tipo | Origen |
|---|---|---|
| `type` | string | `file->typeString()` |
| `filename` | string | `file->filename()` |
| `tags` | array de strings | `file->tags()` |

Para `ErgFileType::erg`, además:

| Campo | Tipo | Origen |
|---|---|---|
| `duration_msecs` | number | `file->duration()` |
| `name` | string | `file->name()` |
| `description` | string | `file->description()` |
| `profile` | array de `[x, y]` | `file->Points`: `x` en ms, `pt.val` en vatios |

Ejemplo:

```json
{
  "workout": {
    "type": "ERG",
    "filename": "/home/user/workouts/tempo.erg",
    "duration_msecs": 3600000,
    "name": "Tempo Ride",
    "description": "3x10 min Tempo",
    "profile": [
      [0, 150],
      [60000, 150],
      [60000, 250],
      [300000, 250]
    ],
    "tags": ["Tempo", "Intervals"]
  }
}
```

Limitaciones de este bloque:

- `name`, `description`, `duration_msecs` y `profile` solo se incluyen para
  `ErgFileType::erg` (formatos `erg`, `erg2` y `mrc`). Los workouts de pendiente
  (`.crs`) y los de tipo `code` no los reciben, aunque tengan metadatos o
  perfil.
- El campo `originalFilename` **no se envía**, aunque la plantilla por defecto
  intenta leerlo (`workout.originalFilename`) y termina usando `filename` como
  fallback.
- `profile` no se incluye para workouts no ERG, aunque tengan un perfil de
  carga.

### 9.3 Filtrado GPS

Si `file->hasGPS()` es verdadero:

1. Se recorre `file->Points`.
2. Se construye un `geolocation` con latitud, longitud y altitud.
3. Se conserva el punto solo si `IsReasonableGeoLocation()` es verdadero.
4. Si no queda ningún punto válido, se devuelve una `FeatureCollection` vacía y
   `elevProfile: []`.

`IsReasonableGeoLocation()` exige:

- latitud no nula y entre −90 y 90, ambos inclusive;
- longitud no nula y entre −180 y 180, ambos inclusive;
- altitud entre −1000 (incluido) y 10000 m (excluido).

El filtrado evita coordenadas `0,0` o altitudes absurdas, pero puede eliminar
tramos completos si el GPS es deficiente.

### 9.4 Perfil de elevación y pendiente

Para cada punto válido se construye una entrada de `elevProfile`:

```text
[ km, elevación_m, pendiente_pct, longitud, latitud ]
```

Algoritmo en `b1b186390`:

1. Se parte del primer punto válido, con `km = 0.0`, `pendiente = 0.0`.
2. Se acumulan distancias con **Haversine** entre puntos consecutivos.
3. Se agrupan puntos hasta acumular al menos `MIN_SEGMENT_METERS = 5.0` metros
   (o hasta el último punto).
4. La pendiente se calcula como:

   ```text
   pendiente = (altura_fin − altura_inicio) / distancia_acumulada × 100
   ```

5. Se redondea:
   - `km` a 3 decimales;
   - elevación a 1 decimal;
   - pendiente a 1 decimal.

La agrupación evita picos de pendiente producidos por pares de puntos casi
coincidentes.

> **Corrección posterior:** `ef01151a5` sustituye Haversine por
> `ErgFilePoint::x` (la distancia real del workout) y normaliza respecto al
> primer punto válido. Es la corrección correcta para que `elevProfile` coincida
> con la telemetría de distancia.

### 9.5 GeoJSON

Se construye una `FeatureCollection` con dos tipos de `Feature`:

#### Líneas

Una `LineString` por cada segmento agregado:

```json
{
  "type": "Feature",
  "properties": { "slope": 2.5 },
  "geometry": {
    "type": "LineString",
    "coordinates": [
      [-3.7038, 40.4168],
      [-3.7040, 40.4170]
    ]
  }
}
```

Las coordenadas son `[longitud, latitud]`, como exige GeoJSON.

#### Marcadores

Cada 5 km se interpola un `Point`:

```json
{
  "type": "Feature",
  "properties": { "km": 5, "is_marker": true },
  "geometry": {
    "type": "Point",
    "coordinates": [-3.7050, 40.4180]
  }
}
```

La interpolación se hace linealmente entre los dos puntos que cruzan el umbral
de 5 km. El campo `km` es entero (`5`, `10`, `15`, …).

### 9.6 Ejemplo completo de ruta

```json
{
  "workout": {
    "type": "ERG",
    "filename": "/path/to/workout.erg",
    "duration_msecs": 3600000,
    "name": "Tempo Ride",
    "description": "3x10min Tempo intervals",
    "profile": [
      [0, 150],
      [60000, 150],
      [60000, 250],
      [300000, 250]
    ],
    "tags": ["Tempo", "Intervals"]
  },
  "geojson": {
    "type": "FeatureCollection",
    "features": [
      {
        "type": "Feature",
        "properties": { "slope": 2.5 },
        "geometry": {
          "type": "LineString",
          "coordinates": [
            [-3.7038, 40.4168],
            [-3.7040, 40.4170]
          ]
        }
      },
      {
        "type": "Feature",
        "properties": { "km": 5, "is_marker": true },
        "geometry": {
          "type": "Point",
          "coordinates": [-3.7050, 40.4180]
        }
      }
    ]
  },
  "elevProfile": [
    [0.0, 650.0, 0.0, -3.7038, 40.4168],
    [0.005, 650.5, 2.5, -3.7040, 40.4170]
  ]
}
```

### 9.7 Flujo completo de ruta

```text
ergFileSelected(file)
   │
   ├── file == nullptr ──▶ m_plannedRoute = ""
   │                        emit plannedRouteChanged("")
   │
   └── file != nullptr
        │
        ├── workout = { type, filename, tags }
        │     └── si type == erg:
        │           + duration_msecs, name, description, profile
        │
        ├── hasGPS?
        │     ├── no ──▶ geojson vacío + elevProfile []
        │     └── sí
        │           ├── filtrar puntos válidos
        │           ├── si no hay ──▶ geojson vacío + elevProfile []
        │           └── construir:
        │                 · elevProfile (km, ele, slope, lon, lat)
        │                 · LineString por segmento ≥ 5 m
        │                 · Point cada 5 km
        │
        └── serializar compacto
             m_plannedRoute = JSON
             emit plannedRouteChanged(JSON)
```

### 9.8 Complejidad y rendimiento

El procesamiento es **O(n)** sobre los puntos del workout, pero se ejecuta en el
hilo GUI y genera bastantes objetos `QJsonArray`/`QJsonObject`. Para rutas GPX
muy grandes (decenas o cientos de miles de puntos) puede producir una pausa
perceptible al seleccionar el workout.

Además, el `geojson` duplica información:

- las coordenadas aparecen en las líneas;
- la elevación/distancia aparecen otra vez en `elevProfile`;
- los marcadores añaden puntos extra.

Para rutas largas, `m_plannedRoute` puede ser un string grande. No hay
compresión ni carga asíncrona.

---

## 10. Configuración y zonas

### 10.1 Estructura

`getConfig()` devuelve un objeto JSON con:

```text
{
  "max_power_w": 250,
  "max_hr_bpm": 200,
  "is_dark_background": true,
  "power": { ... },   // opcional
  "hr": { ... }       // opcional
}
```

`power` y `hr` solo aparecen si el atleta tiene zonas válidas para la fecha
actual.

### 10.2 Zonas de potencia

Se obtienen con:

```cpp
const QString sportTag = RideFile::sportTag("Bike");
const QDate today = QDate::currentDate();
const Zones *pz = m_context->athlete->zones(sportTag);
const int range = pz->whichRange(today);
```

Campos:

| Campo | Tipo | Método |
|---|---|---|
| `cp` | number | `getCP(range)` |
| `ftp` | number | `getFTP(range)` |
| `wprime` | number | `getWprime(range)` |
| `pmax` | number | `getPmax(range)` |
| `aetp` | number | `getAeT(range)` |
| `zoneLows` | array de int | `getZoneLows(range)` |
| `zoneNames` | array de string | `getZoneNames(range)` |
| `zoneDescriptions` | array de string | `getZoneDescriptions(range)` |
| `zoneColors` | array de string | `zoneColor(i, numZones)` |

`zoneColors` se genera para `i = 0 .. numZones - 1` y se serializa como
`#rrggbb`.

### 10.3 Zonas de frecuencia cardiaca

Se obtienen con `athlete->hrZones(sportTag)`. Campos:

| Campo | Tipo | Método |
|---|---|---|
| `lthr` | number | `getLT(range)` |
| `aethr` | number | `getAeT(range)` |
| `restHr` | number | `getRestHr(range)` |
| `maxHr` | number | `getMaxHr(range)` |
| `zoneLows` | array de int | `getZoneLows(range)` |
| `zoneNames` | array de string | `getZoneNames(range)` |
| `zoneTrimps` | array de double | `getZoneTrimps(range)` |
| `zoneDescriptions` | array de string | `getZoneDescriptions(range)` |
| `zoneColors` | array de string | `hrZoneColor(i, numZones)` |

`zoneDescriptions` fue añadido a `HrZones` en `7c1290e4a`; `Zones` ya lo tenía.
Sin ese commit, el puente no podría compilar la llamada para FC.

### 10.4 Límites `max_power_w` y `max_hr_bpm`

Se usan como topes de escala para gauges o gráficos. No son necesariamente el
CP/FTP ni el máximo real del atleta.

#### `max_power_w`

| Prioridad | Condición | Valor |
|---:|---|---|
| 1 | `getPmax(range) > 0` | `pmax` |
| 2 | último `zoneHigh > 0` | `zoneHighs.last()` |
| 3 | hay `zoneLows` | `ceil(zoneLows.last() * 1.15)` |
| 4 | sin datos | `250` |

#### `max_hr_bpm`

| Prioridad | Condición | Valor |
|---:|---|---|
| 1 | `getMaxHr(range) > 0` | `maxHr` configurado |
| 2 | último `zoneHigh > 0` | `zoneHighs.last()` |
| 3 | `getLT(range) > 0` | `ceil(lthr * 1.10)` |
| 4 | sin datos | `200` |

### 10.5 Ejemplo de configuración

```json
{
  "max_power_w": 900,
  "max_hr_bpm": 190,
  "is_dark_background": true,
  "power": {
    "cp": 290,
    "ftp": 285,
    "wprime": 20000,
    "pmax": 900,
    "aetp": 220,
    "zoneLows": [0, 160, 200, 240, 280, 320, 360],
    "zoneNames": ["Z1", "Z2", "Z3", "Z4", "Z5", "Z6", "Z7"],
    "zoneDescriptions": [
      "Active Recovery",
      "Endurance",
      "Tempo",
      "Threshold",
      "VO2 Max",
      "Anaerobic Capacity",
      "Neuromuscular Power"
    ],
    "zoneColors": ["#ffffff", "#00ff00", "#ffff00", "#ffaa00", "#ff0000", "#aa0000", "#ff00ff"]
  },
  "hr": {
    "lthr": 170,
    "aethr": 150,
    "restHr": 50,
    "maxHr": 190,
    "zoneLows": [0, 115, 140, 155, 170, 180],
    "zoneNames": ["Z1", "Z2", "Z3", "Z4", "Z5"],
    "zoneTrimps": [0.9, 1.1, 1.2, 2.0, 5.0],
    "zoneDescriptions": ["Recovery", "Endurance", "Tempo", "Threshold", "VO2 Max"],
    "zoneColors": ["#ffffff", "#00ff00", "#ffff00", "#ffaa00", "#ff0000"]
  }
}
```

### 10.6 Limitaciones de configuración

- **Solo ciclismo:** `sportTag = RideFile::sportTag("Bike")`. Un workout de
  carrera, natación o remo recibe las zonas de bici si existen.
- **Fecha actual:** `whichRange(QDate::currentDate())`, no la fecha del workout
  ni la de la ride. Un workout planificado para otra fecha puede usar un rango
  de zonas distinto.
- **Sin notificación de cambios:** si el atleta cambia zonas o tema, la página
  no recibe un evento. `configChanged()` solo actualiza la paleta; no vuelve a
  cargar el HTML ni emite una señal de configuración.
- **Valores opcionales:** si no hay atleta o rango válido, `power`/`hr` se
  omiten. El HTML debe comprobar su existencia.
- **Tipos:** `max_power_w` y `max_hr_bpm` son enteros; el resto de métricas
  pueden ser enteros o doubles según la API de zonas.

---

## 11. Persistencia en perspectivas

### 11.1 Cómo guarda GoldenCheetah un chart

`Perspective::toXml()` recorre las propiedades del `QMetaObject` y guarda las
que tienen `USER true`:

```text
<layout ...>
  <chart id="58" ...>
    <property name="html" type="QString" value="..." />
    <property name="splitterState" type="QByteArray" value="" />
    <property name="showEditor" type="bool" value="0" />
    <property name="chartConfig" type="QString" value="..." />
    <property name="showTitleBar" type="bool" value="1" />
  </chart>
</layout>
```

El tipo se guarda en el atributo `type`, pero el valor se escribe con una cadena
de `if` que solo cubre:

- `int`
- `double`
- `QDate`
- `QTime`
- `QString`
- `bool`
- `LTMSettings`

### 11.2 El problema de `splitterState`

`splitterState` es `QByteArray`, pero **no hay rama para `QByteArray`** en
`Perspective::toXml()`, en `ViewParser::startElement()` ni en
`Perspective::importChart()`.

Resultado:

1. Al guardar una perspectiva, se escribe la etiqueta
   `<property name="splitterState" ...>` pero el valor queda vacío.
2. Al cargar, `ViewParser` ignora la propiedad porque el tipo no está
   contemplado.
3. El tamaño del splitter nunca se restaura.

El fixture `Train HTMLTestTelemetry.gchartset` incluye:

```xml
<property name="splitterState" type="QByteArray" value="" />
```

Lo que confirma el diagnóstico: la propiedad existe, pero viaja vacía.

`GcChartWindow::serializeChartToQTextStream()` (exportación `.gchart`/nube)
tiene el mismo problema: omite las propiedades `QByteArray`, por lo que
`splitterState` ni siquiera aparece en `PROPERTIES`.

Posibles soluciones:

- guardar el estado como `QString` base64 y decodificarlo en el setter;
- añadir una rama `QByteArray` a la serialización de perspectivas y charts;
- usar una propiedad `QString` separada solo para persistencia.

### 11.3 Otras ausencias de persistencia

| Elemento | ¿Se guarda? | Motivo |
|---|---:|---|
| Texto HTML | Sí | `html` es `QString`. |
| Configuración key/value | Sí | `chartConfig` es `QString`. |
| Editor visible | Sí | `showEditor` es `bool`. |
| Título visible | Sí | `showTitleBar` es `bool`. |
| Panel de configuración visible | **No** | No existe propiedad. |
| Estado del splitter | **No** | `QByteArray` no soportado. |
| Scroll del editor | **No** | No es propiedad. |
| Estado JavaScript de la página | **No** | Se pierde al recargar. |

### 11.4 Orden de carga y propiedades

Hay dos caminos de carga y no aplican las propiedades en el mismo orden:

| Camino | Uso | Orden |
|---|---|---|
| `ViewParser::startElement()` | Perspectivas `.gchartset` | Orden del XML. |
| `Perspective::importChart()` | Diálogo Import Chart (`.gchart`) | `QMap`, ordenado por clave. |

En el fixture, `html` aparece **antes** que `chartConfig`. Por tanto, al cargar
una perspectiva, `setHtml()` se ejecuta con `currentChartConfig` vacío y la
configuración se aplica después, mientras la página aún está cargando.

En `importChart()`, en cambio, `chartConfig` va antes que `html` porque el
`QMap` está ordenado por clave.

El resultado visible suele ser correcto porque `QWebEnginePage::setHtml()` y la
creación del canal son asíncronos: cuando el JavaScript llama a
`chart.getChartConfig()`, el parseo ya ha terminado. Pero es una dependencia de
temporalidad, no una garantía:

- `setChartConfigString()` no llama a `applyHtml()` por sí mismo.
- `setHtml()` sí llama a `applyHtml()`.
- Si se cambiara el orden o la página ejecutara el callback antes de terminar el
  parseo, la página podría cargar con `{}`.

Es una dependencia implícita que conviene documentar o eliminar.

---

## 12. Integración con RealtimeData, Zones y HrZones

### 12.1 `RealtimeData`

La telemetría se apoya en tres métodos:

| Método | Función |
|---|---|
| `listDataSeries()` | Lista estática de series que se exportan. |
| `value(series)` | Devuelve el valor de una serie. |
| `seriesSymbol(series)` | Devuelve la clave JSON estable. |
| `seriesName(series)` | Devuelve el nombre de presentación (traducido); el puente **no** lo usa. |

`listDataSeries()` se inicializa de forma perezosa:

```cpp
static QList<DataSeries> seriesList;

if (seriesList.count() == 0) {
    seriesList << None;
    seriesList << Time;
    // ...
}
return seriesList;
```

Es una lista manual. Añadir una serie al enum **no** la añade automáticamente a
la lista; hay que editar el método.

### 12.2 Cambios de `e064b7180`

Ese commit:

- añade `case Wbal: return wbal;` a `value()`;
- añade `RightPCO` y `LeftPCO` a `seriesName()`;
- añade `RightPCO` y `LeftPCO` a `seriesSymbol()`.

Antes de ese commit, `Wbal` ya estaba en `listDataSeries()`, pero `value(Wbal)`
no tenía caso y devolvía el valor por defecto (`0`). El puente de entonces aún
no recorría la lista: emitía `wbal` con `rt.getWbal()`. La corrección llegó
antes que el bucle de `b1b186390`; sin ella, ese bucle habría publicado
`"W' bal": 0`.

Sin embargo, **`LeftPCO` y `RightPCO` no se añadieron a `listDataSeries()`**.
Por tanto, aunque `value()` y `seriesSymbol()` sepan manejarlos, el bucle de
telemetría nunca los visita y no se emiten.

Lo mismo ocurre con `Position` (posición del ciclista), que tampoco está en la
lista.

### 12.3 Consecuencia práctica

Para que una serie llegue al HTML deben coincidir tres cosas:

1. Estar en `listDataSeries()`.
2. Tener un caso en `value()`.
3. Tener un caso en `seriesSymbol()`.

Si falta cualquiera de las tres, la serie no aparece o aparece con un valor
incorrecto. Es un acoplamiento manual fácil de olvidar.

### 12.4 `HrZones` y `Zones`

`7c1290e4a` añade a `HrZones`:

```cpp
QList<QString> HrZones::getZoneDescriptions(int rnum) const
{
    if (rnum < 0 || rnum >= ranges.size()) return QList<QString>();

    const HrZoneRange &range = ranges[rnum];
    QList<QString> return_values;

    for (int i = 0; i < range.zones.size(); i++) {
        return_values.append(ranges[rnum].zones[i].desc);
    }

    return return_values;
}
```

`Zones` ya tenía el método equivalente. El commit también corrige el comentario
de `Zones::getZoneDescriptions()` (“zone names” → “zone descriptions”).

### 12.5 Colores

Los arrays `zoneColors` se generan con:

- `zoneColor(i, numZones)` para potencia;
- `hrZoneColor(i, numZones)` para FC.

Se serializan como nombres de color (`#rrggbb`). El HTML puede usarlos
directamente en CSS o en bibliotecas de gráficos.

### 12.6 Build

`src/src.pro` ya incluye los módulos:

```text
webenginecore webenginewidgets webchannel positioning webenginequick core5compat
```

La implementación solo añade:

- `Train/HtmlTrainingBridge.h` a `HEADERS`;
- `Train/HtmlChart.h` a `HEADERS`;
- `Train/HtmlChart.cpp` a `SOURCES`;
- `Train/HtmlTrainingBridge.cpp` a `SOURCES`.

No se añade una dependencia nueva de módulos Qt.

---

## 13. Pruebas y fixture

### 13.1 Fixture

`test/charts/Train HTMLTestTelemetry.gchartset` fue añadido en `b1b186390`.

Es una perspectiva XML con un chart:

```xml
<layout name="HTMLTestTelemetry" style="2" type="1" ...>
  <chart id="58" name="" title="HTML Test Telemetry">
    <property name="title" type="QString" value="HTML Test Telemetry" />
    <property name="html" type="QString" value="..." />
    <property name="splitterState" type="QByteArray" value="" />
    <property name="showEditor" type="bool" value="0" />
    <property name="chartConfig" type="QString" value="" />
    <property name="showTitleBar" type="bool" value="1" />
  </chart>
</layout>
```

Características:

- `showEditor = 0`: el chart se abre mostrando solo la vista web.
- `html`: plantilla rica con metadatos del workout, flag `dark` y una rejilla
  dinámica de telemetría.
- `chartConfig`: vacío.
- `splitterState`: vacío (la propiedad no se persiste).
- `showTitleBar`: visible.

### 13.2 Qué prueba y qué no

El fixture sirve para una prueba **manual/visual**:

- que el chart se registra con id 58;
- que el HTML se carga;
- que `getConfig()` y `getPlannedRoute()` responden;
- que la rejilla de telemetría se rellena;
- que el estado cambia con `start/pause/stop`.

No es un test automatizado:

- no hay aserciones sobre el JSON;
- no se compara el HTML generado;
- no se prueba la serialización de propiedades;
- no se prueba el throttle;
- no se prueba la ruta con GPS;
- no se prueba el caso “sin workout”.

### 13.3 Plantilla del fixture frente a la plantilla por defecto

En `b1b186390`, la plantilla **por defecto** del chart seguía siendo la mínima
de `b6e7f9305`:

```html
<div>Speed: <b id="speed">0.0</b> km/h</div>
<div>Power: <b id="power">0</b> W</div>
<div>HR: <b id="hr">0</b> bpm</div>
<div>State: <b id="state">Stopped</b></div>
```

Su script leía `d.speed_kmh`, `d.power_w` y `d.hr_bpm`, las claves fijas
anteriores a `b1b186390`. Ese mismo commit sustituyó la telemetría por
`listDataSeries()` + `seriesSymbol()` y el puente pasó a emitir `Speed`, `Power`
y `Heart Rate`. La plantilla no se actualizó a la vez, así que mostraba ceros
hasta que `4c6430a2c` la reemplazó.

El fixture, en cambio, ya usaba una plantilla más avanzada con metadatos y
rejilla dinámica, compatible con las claves nuevas. Esa plantilla sirvió de
referencia para la nueva plantilla por defecto de `4c6430a2c`, que adopta la
misma idea de metadatos + rejilla dinámica.

Por eso el fixture actual (sin cambios desde `b1b186390`) está **desfasado**
respecto a la plantilla por defecto de HEAD.

---

## 14. Decisiones de diseño y motivaciones

| Decisión | Motivación | Contrapartida |
|---|---|---|
| `HtmlChart` hereda de `GcChartWindow` | Integrarse con perspectivas, título, estilo y exportación. | Hay que gestionar el título y los márgenes manualmente. |
| Puente `gc` en `Context` | Una sola fuente de verdad y reutilización entre charts. | Estado compartido y señales difundidas a todas las páginas. |
| Puente `chart` separado | Minimizar la superficie expuesta a JavaScript. | Dos objetos que documentar. |
| Usar `QWebChannel` | Puente nativo Qt/JS, sin servidor HTTP. | Los valores viajan como cadenas (casi siempre JSON); no hay objetos JS nativos. |
| Throttle a 1 Hz | Evitar saturar el renderer y la UI. | Hasta 1 s de latencia y primera muestra retrasada. |
| `seriesSymbol()` como clave | Evitar traducciones y tener claves estables. | Nombres con espacios, apóstrofos y convenciones heterogéneas. |
| `QJsonDocument::Compact` | Reducir tamaño del payload. | JSON difícil de leer en logs. |
| Recargar con `setHtml()` | Simple, reproducible y sin estado residual. | Destruye el estado JS, cierra conexiones y reinicia la página. |
| Ruta precalculada al seleccionar workout | Evita calcularla en cada frame. | Coste O(n) en el hilo GUI y payload grande. |
| `Current Position` único | Unificar ERG y slope en un solo campo. | Ambigüedad de unidades (ms vs m). |
| Zonas solo de bici | El caso de uso principal es ciclismo. | Running/swimming reciben datos incorrectos o ausentes. |
| `USER true` en las propiedades | Persistencia automática para los tipos soportados por `Perspective`. | `QByteArray` queda fuera del soporte real. |

---

## 15. Limitaciones y riesgos

### 15.1 Limitaciones funcionales

| # | Limitación | Impacto | Evidencia |
|---:|---|---|---|
| 1 | No se emite la ruta inicial | Si el puente se crea con un workout ya seleccionado (primer chart HTML del contexto), `getPlannedRoute()` devuelve `""` hasta el siguiente `ergFileSelected`. | El constructor del puente no llama a `onErgFileSelected(m_context->currentErgFile())`. |
| 2 | No se emite el estado inicial | Una página abierta durante el entrenamiento no recibe `"start"`; puede mostrar “Stopped”. | Solo hay señales en `start/pause/resume/stop`; no hay `getState()`. |
| 3 | No hay replay de la última muestra | Tras una recarga, la página espera hasta 1 s para recibir telemetría. | `m_pendingTelemetrySample` no se expone al cargar. |
| 4 | `Current Position` ambiguo | Milisegundos en ERG, metros en slope/GPS; sin unidad ni modo. | `Context::getNow()` se alimenta de `load_msecs` o `displayWorkoutDistance * 1000`. |
| 5 | `listDataSeries()` omite PCO y Position | `LeftPCO`, `RightPCO` y `Position` no se emiten. | La lista no los incluye, aunque `value()`/`seriesSymbol()` sí. |
| 6 | `originalFilename` no se envía | La plantilla no puede mostrar el nombre original; usa `filename`. | `onErgFileSelected()` no escribe ese campo. |
| 7 | Metadatos solo ERG | `name`, `description`, `duration_msecs` y `profile` faltan en workouts de pendiente (`.crs`) y de tipo `code`. | Condición `file->type() == ErgFileType::erg`. |
| 8 | Zonas solo bici y fecha actual | Atletas de otros deportes o workouts con otra fecha usan datos incorrectos. | `sportTag("Bike")` y `QDate::currentDate()`. |
| 9 | Configuración no notificada | Cambios de zonas/tema no llegan a la página. | No hay señal `configChanged` en el puente. |
| 10 | `chartConfig` solo strings | Booleanos, números y arrays pierden su tipo. | `valItem->text()` y `toVariant().toString()`. |
| 11 | `chartConfig` inválido no se valida | `chart.getChartConfig()` puede devolver JSON inválido. | `setChartConfigString()` no comprueba `doc.isObject()`. |
| 12 | `showConfig` no persiste | El panel de configuración vuelve a estar oculto al cargar. | No hay `Q_PROPERTY`. |
| 13 | `splitterState` no persiste | El tamaño del splitter se pierde. | `QByteArray` no soportado por `Perspective`, `ViewParser` ni `serializeChartToQTextStream`. |

### 15.2 Limitaciones de la actualización

| # | Limitación | Impacto |
|---:|---|---|
| 14 | `setHtml()` recarga toda la página | Se pierde el estado JavaScript, scroll, conexiones WebSocket, variables y listeners. |
| 15 | La recarga ocurre al cambiar el workout | `HtmlChart` conecta `ergFileSelected` a `applyHtml()`, además de la señal del puente. |
| 16 | El orden de conexión es frágil | `applyHtml()` se conecta antes que el puente a `ergFileSelected`; la recarga se lanza antes de actualizar `m_plannedRoute`. Funciona por la asincronía de `setHtml()`, pero no es una garantía explícita. |
| 17 | La recarga ocurre al cambiar filtros | `filterChanged` y `homeFilterChanged` están conectados a `applyHtml()` aunque el chart no use filtros. |
| 18 | El editor recarga tras 1 s de inactividad | Es cómodo, pero no hay actualización incremental. |
| 19 | El JSON se construye a 5 Hz y se emite a 1 Hz | Trabajo de serialización innecesario. |
| 20 | La ruta se procesa en el hilo GUI | Rutas muy grandes pueden congelar la UI al seleccionarlas. |
| 21 | El payload de ruta puede ser grande | Coordenadas duplicadas entre `geojson` y `elevProfile`; sin compresión. |

### 15.3 Limitaciones de la plantilla por defecto

La plantilla actual (posterior a `4c6430a2c`) mejora mucho la experiencia, pero
tiene detalles:

- El formateador numérico comprueba si la clave contiene `"msec"` para dividir
  por 1000. Las claves nuevas como `Time`, `Lap Time`, `Section Time Remaining`
  o `Current Position` **no contienen** `"msec"`, así que se muestran como
  números crudos.
- Muestra el flag `dark`, pero no adapta el CSS al tema.
- Lee `workout.originalFilename`, que el puente no envía.
- No se suscribe a `plannedRouteChanged`; solo llama a `getPlannedRoute()` al
  cargar. La recarga completa oculta el problema, pero no es un diseño
  incremental.
- No usa `chart.getChartConfig()`; la configuración específica del chart queda
  disponible pero no se muestra.

### 15.4 Riesgos de seguridad

El chart ejecuta HTML/JavaScript arbitrario dentro de un `QWebEngineView` y
habilita:

```cpp
canvas->page()->settings()->setAttribute(
    QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
```

Implicaciones:

- El HTML puede cargar recursos remotos (CDNs, APIs, mapas).
- También puede hacer peticiones desde el contexto local.
- No hay sanitización del HTML introducido o importado.
- Un chart compartido por otra persona debe tratarse como **código de
  confianza**.

No es un fallo en sí: la funcionalidad requiere ejecutar HTML del usuario. Pero
conviene documentarlo y, si se importan charts de terceros, advertir al usuario.

### 15.5 Deuda menor

- Includes posiblemente innecesarios tras `b1b186390`: `QDateTime`, `QTimeZone`,
  `QDebug`, `QPointF`.
- `Q_ASSERT(m_context != nullptr)` seguido de `if (m_context)` es redundante.
- `try/catch (std::exception)` alrededor de la creación del `QWebChannel` no
  aporta mucho: Qt no suele lanzar excepciones en ese punto.
- `setHtml()` provoca una recarga inmediata y, probablemente, otra diferida por
  `setPlainText()` + `textChanged`.
- `m_savedTopMargin` usa un fallback de 15 px no escalado por DPI.
- `getChartConfig()` y `getChartConfigString()` tienen comportamientos distintos
  para el caso vacío (`"{}"` vs `""`), lo que puede confundir.

---

## 16. Oportunidades de mejora

### 16.1 Prioridad alta

1. **Sembrar el estado inicial**
   - En el constructor del puente, llamar a
     `onErgFileSelected(m_context->currentErgFile())` si hay workout.
   - Exponer `getState()` o reemitir el estado actual al cargar la página.
   - Exponer la última muestra de telemetría (`getLastTelemetry()` o replay al
     conectar).

2. **Separar “cargar documento” de “actualizar datos”**
   - `setHtml()` solo cuando cambie el HTML.
   - Los cambios de configuración, workout o filtros deberían actualizar datos
     sin recargar la página.
   - Usar `runJavaScript()` o una señal JS para inyectar el nuevo JSON.

3. **Añadir unidades y modo al contrato**
   - `workout_mode`: `"erg"`, `"slope"`, `"gps"`.
   - `elapsed_msecs` y `route_distance_m` como campos separados.
   - Unidades explícitas en los nombres (`power_w`, `speed_kmh`, etc.) o un
     objeto `units`.

4. **Completar `listDataSeries()`**
   - Añadir `LeftPCO`, `RightPCO` y `Position`.
   - Revisar que todas las series del enum tengan `value()` y `seriesSymbol()`.
   - Considerar un test que recorra el enum y verifique la cobertura.

5. **Persistir `splitterState`**
   - Añadir soporte `QByteArray` a `Perspective` y `serializeChartToQTextStream`,
     o guardar base64 en una propiedad `QString`.

### 16.2 Prioridad media

6. **Persistir `showConfig`** con una nueva `Q_PROPERTY`.
7. **Validar `chartConfig`** y devolver `"{}"` si el JSON no es un objeto.
8. **Conservar tipos** en la configuración: editor JSON completo o columnas
   tipadas (`string`, `number`, `bool`).
9. **Añadir `originalFilename`** al bloque `workout`.
10. **Incluir metadatos y perfil para workouts no ERG** cuando estén
    disponibles.
11. **Usar el deporte y la fecha del workout** para seleccionar zonas, o
    exponer todos los deportes.
12. **Emitir una señal `configChanged`** para que la página actualice tema y
    zonas sin recargar.

### 16.3 Prioridad baja

13. **Inyectar `document.title` en `<head>`** en lugar de concatenar después de
    `</html>`.
14. **Escapar saltos de línea** en el título.
15. **Eliminar las conexiones a `filterChanged`/`homeFilterChanged`** si no se
    usan.
16. **Construir el JSON una sola vez por segundo**, no en cada actualización de
    5 Hz.
17. **Mover el procesamiento de rutas grandes a un worker** o cachear el
    resultado.
18. **Actualizar el fixture** a la plantilla actual.
19. **Añadir tests automatizados** de:
    - serialización de telemetría;
    - fallback de `max_power_w`/`max_hr_bpm`;
    - ruta con y sin GPS;
    - casos `nullptr` y sin workout;
    - persistencia de propiedades.

---

## 17. Evolución posterior a b1b186390

Esta sección separa los cambios posteriores para que el análisis del diseño
original no se contamine con correcciones puntuales.

### 17.1 `a23009ab6` — flag de tema

Cambia:

```cpp
obj["is_dark_background"] = GCColor::isDark(GColor(CPLOTBACKGROUND));
```

por:

```cpp
obj["is_dark_background"] = GCColor::isDark(GColor(CTRAINPLOTBACKGROUND));
```

Motivo: `HtmlChart` usa `CTRAINPLOTBACKGROUND` como color de fondo del chart.
El flag debe derivarse del mismo color para que el HTML reciba un valor
coherente.

### 17.2 `ef01151a5` — distancia del perfil

Problema: `elevProfile` acumulaba distancias con Haversine entre puntos GPS,
mientras que la telemetría y el workout usan `ErgFilePoint::x`. Eso producía
desajustes entre la posición real y el perfil.

Solución:

- eliminar `deg2rad()` y `haversineDistance()`;
- usar `p2.x - routeStartMeters` para la distancia acumulada;
- usar `p2.x - segStartDist` para el segmento;
- usar `p2.x - p1.x` para interpolar los marcadores.

Resultado: `elevProfile` y los marcadores quedan alineados con la distancia real
del workout.

### 17.3 `4c6430a2c` — plantilla por defecto

Sustituye la plantilla mínima de `b1b186390` por una genérica:

- cabecera con nombre, tipo, descripción, ficheros, duración y tags;
- flag `dark`;
- rejilla dinámica de todas las claves de telemetría;
- estado;
- llamadas a `getPlannedRoute()` y `getConfig()` con comprobaciones de nulidad.

Es la plantilla que se ve actualmente al crear un chart HTML nuevo.

### 17.4 `ed32e98dc` — Video + HTML

Añade `VideoHtmlWindow` (id 59), que combina un vídeo con una capa HTML. Reutiliza
el mismo `HtmlTrainingBridge` del `Context`:

```cpp
if (context->getHtmlTrainingBridge()) {
    m_webChannel->registerObject(QStringLiteral("gc"),
                                 context->getHtmlTrainingBridge());
}
```

Esto valida la decisión de poner el puente en `Context`: el mismo contrato `gc`
sirve para dos charts distintos. El análisis detallado de `VideoHtmlWindow` queda
fuera del alcance de este documento.

---

## 18. Conclusiones

La implementación de `b1b186390` es un MVP coherente y bien separado:

- **Vista:** `HtmlChart` integra editor, configuración y WebEngine en un
  `GcChartWindow`.
- **Puente:** `HtmlTrainingBridge` centraliza datos y estado; `HtmlChartBridge`
  limita la superficie expuesta.
- **Contrato:** JSON compacto, claves derivadas de `RealtimeData`, ruta GeoJSON
  y configuración de zonas.
- **Rendimiento:** throttle a 1 Hz y ruta precalculada.
- **Integración:** registro id 58, build y fixture.

El punto débil no es la separación de clases, sino el **modelo de actualización**:
la página se trata como un documento que se recarga, no como una aplicación con
estado. Eso explica la mayoría de las limitaciones prácticas (estado inicial,
replay, pérdida de conexiones, recarga por filtros).

La segunda deuda es el **contrato de datos**: `Current Position` mezcla
unidades, las claves no declaran unidades y `listDataSeries()` requiere
mantenimiento manual. Son problemas de API, no de arquitectura, y se pueden
corregir de forma incremental.

En conjunto, el diseño es una base sólida para dashboards HTML dentro de
GoldenCheetah. Con un estado inicial bien sembrado, actualizaciones sin recarga
y un contrato con unidades explícitas, pasaría de ser un prototipo potente a una
API estable para la comunidad.

---

## Anexo A. Series de telemetría

`listDataSeries()` contiene 62 entradas; `None` se omite, por lo que se emiten
**61 claves de series**. Con `Current Position` y `notification_text`, añadidas
manualmente, el objeto final tiene **63 claves**. La tabla siguiente refleja el
estado de HEAD y la correspondencia con `seriesSymbol()`.

| Enum `RealtimeData` | Clave JSON |
|---|---|
| `None` | `None` (omitida) |
| `Time` | `Time` |
| `Lap` | `Lap` |
| `LapTime` | `Lap Time` |
| `Distance` | `Distance` |
| `Watts` | `Power` |
| `Speed` | `Speed` |
| `Cadence` | `Cadence` |
| `HeartRate` | `Heart Rate` |
| `Load` | `Target Power` |
| `BikeScore` | `BikeScore` |
| `SkibaVI` | `Skiba Variability Index` |
| `BikeStress` | `BikeStress` |
| `XPower` | `XPower` |
| `IsoPower` | `Iso Power` |
| `RI` | `Relative Intensity` |
| `IF` | `Intensity Factor` |
| `VI` | `Variability Index` |
| `Joules` | `kJoules` |
| `Wbal` | `W' bal` |
| `SmO2` | `Hb O2 Saturation` |
| `tHb` | `Total Hb Mass` |
| `HHb` | `Deoxy Hb` |
| `O2Hb` | `Oxy Hb` |
| `Rf` | `Respiratory Frequency` |
| `RMV` | `Ventilation` |
| `VO2` | `VO2` |
| `VCO2` | `VCO2` |
| `RER` | `Respiratory Exchange Ratio` |
| `TidalVolume` | `Tidal Volume` |
| `FeO2` | `Fraction O2 Expired` |
| `AvgWatts` | `Average Power` |
| `AvgSpeed` | `Average Speed` |
| `AvgCadence` | `Average Cadence` |
| `AvgHeartRate` | `Average Heartrate` |
| `AvgWattsLap` | `Lap Power` |
| `AvgSpeedLap` | `Lap Speed` |
| `AvgCadenceLap` | `Lap Cadence` |
| `AvgHeartRateLap` | `Lap Heartrate` |
| `VirtualSpeed` | `Virtual Speed` |
| `AltWatts` | `Alternate Power` |
| `LRBalance` | `Left/Right Balance` |
| `LapTimeRemaining` | `Lap Time Remaining` |
| `LeftTorqueEffectiveness` | `Left Torque Effectiveness` |
| `RightTorqueEffectiveness` | `Right Torque Effectiveness` |
| `LeftPedalSmoothness` | `Left Pedal Smoothness` |
| `RightPedalSmoothness` | `Right Pedal Smoothness` |
| `Slope` | `Slope` |
| `LapDistance` | `Lap Distance` |
| `LapDistanceRemaining` | `Lap Distance Remaining` |
| `ErgTimeRemaining` | `Section Time Remaining` |
| `Latitude` | `Latitude` |
| `Longitude` | `Longitude` |
| `Altitude` | `Altitude` |
| `RouteDistance` | `Route Distance` |
| `DistanceRemaining` | `Distance Remaining` |
| `Temp` | `Temperature` |
| `CoreTemp` | `Core Temp` |
| `SkinTemp` | `Skin Temp` |
| `HeatStrain` | `Heat Strain` |
| `HeatLoad` | `Estimated Heat Load` |
| `VAM` | `VAM` |

Series del enum que **no** están en `listDataSeries()` y, por tanto, no se
emiten:

- `Position`
- `RightPCO`
- `LeftPCO`

> `Current Position` y `notification_text` no son series del enum: se añaden
> manualmente en `onTelemetryUpdate()`.

---

## Anexo B. Referencias de código

| Referencia | Contenido |
|---|---|
| `src/Train/HtmlChart.h` | Declaración de `HtmlChart` y `HtmlChartBridge`; propiedades. |
| `src/Train/HtmlChart.cpp` | Construcción de UI, editor, tabla, WebEngine, WebChannel, `applyHtml()`, paleta y título. |
| `src/Train/HtmlTrainingBridge.h` | API de `gc`, señales, slots y estado interno. |
| `src/Train/HtmlTrainingBridge.cpp` | `getConfig()`, `getPlannedRoute()`, telemetría, ruta, notificaciones y estados. |
| `src/Core/Context.h` | `getHtmlTrainingBridge()` y `m_HtmlTrainingBridge`. |
| `src/Core/Context.cpp` | Creación perezosa del puente y conexión con el ciclo de vida del contexto. |
| `src/Gui/GcWindowRegistry.h` | `GcWindowTypes::HtmlTraining = 58`. |
| `src/Gui/GcWindowRegistry.cpp` | Registro “HTML Chart” y factoría. |
| `src/Charts/GoldenCheetah.h` | `GcWindow`, `GcChartWindow`, `setShowTitle`, `title()`. |
| `src/Charts/GoldenCheetah.cpp` | Layout base, pintura del título, serialización de charts. |
| `src/Gui/Perspective.cpp` | Serialización/importación de propiedades `USER`. |
| `src/Train/RealtimeData.h` | Enum `DataSeries` y API de valores. |
| `src/Train/RealtimeData.cpp` | `listDataSeries()`, `value()`, `seriesName()`, `seriesSymbol()`. |
| `src/Metrics/Zones.h`, `src/Metrics/Zones.cpp` | Zonas de potencia, descripciones y colores. |
| `src/Metrics/HrZones.h`, `src/Metrics/HrZones.cpp` | Zonas de FC, descripciones y `zoneTrimps`. |
| `src/Gui/Colors.h`, `src/Gui/Colors.cpp` | `GColor`, `GCColor::isDark`, `zoneColor`, `hrZoneColor`. |
| `src/Train/TrainSidebar.cpp` | Emisión de `telemetryUpdate`, `notifySetNow`, estados. |
| `src/FileIO/LocationInterpolation.h` | `geolocation::IsReasonableGeoLocation()`. |
| `test/charts/Train HTMLTestTelemetry.gchartset` | Fixture manual del chart HTML. |
| `src/src.pro` | Headers, sources y módulos Qt. |

---

## Anexo C. Glosario

| Término | Significado |
|---|---|
| **GcChartWindow** | Clase base de los charts de GoldenCheetah; integra layout, título y serialización. |
| **Perspectiva** | Configuración guardada de una vista: charts, tamaños, propiedades y disposición. |
| **QtWebChannel** | Mecanismo de Qt para exponer objetos C++ a JavaScript y viceversa. |
| **QWebEngineView** | Widget de Qt basado en Chromium para renderizar HTML. |
| **RealtimeData** | Estructura con la telemetría instantánea de la sesión. |
| **ErgFile** | Workout con puntos de tiempo/distancia, carga, altitud y GPS. |
| **ERG** | Workout con objetivo de potencia (vatios) frente a tiempo. |
| **Slope / GPS** | Workout con objetivo de pendiente o recorrido geográfico frente a distancia. |
| **W' bal** | Balance de trabajo anaeróbico (W prime balance). |
| **PCO** | *Pedal Centre Offset*; desplazamiento del centro de presión del pedal. |
| **GeoJSON** | Formato JSON para geometrías; aquí `FeatureCollection`, `LineString` y `Point`. |
| **elevProfile** | Array `[km, elevación, pendiente, lon, lat]` con el perfil de la ruta. |
| **Throttle** | Limitación de frecuencia de emisión; aquí 1 muestra por segundo. |

---

> **Fin del documento.** Si se modifica el contrato JSON o se añaden series a
> `RealtimeData`, conviene actualizar el Anexo A y la sección de limitaciones.
