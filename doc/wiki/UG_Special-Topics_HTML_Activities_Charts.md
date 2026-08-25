# HTML Activities Charts

## Overview
GoldenCheetah allows the integration of custom HTML/JavaScript charts directly into the **Activities** view. This is powered by a Qt WebEngine backend and a `QWebChannel` bridge called `HtmlActivitiesBridge`. 

By embedding an HTML page, developers can leverage modern JavaScript charting libraries (like D3.js, Chart.js, Plotly, or ECharts) while natively accessing the rich dataset of the currently selected Activity or Athlete in GoldenCheetah.

## Context and Architecture
Unlike the Training view HTML charts (which use a push-based model to stream live telemetry), the Activities view uses a **pull-based model**. When an HTML chart is loaded, it establishes a WebSocket-like connection with GoldenCheetah. Once connected, JavaScript can asynchronously query GoldenCheetah for metrics, telemetry series, xdata, and athlete zones.

The bridge exposes a global `gc` object to JavaScript, providing methods that mimic the behavior of the GoldenCheetah Python API, ensuring consistency for developers familiar with Python charts.

## The `gc` JavaScript API

Once the `QWebChannel` is initialized, the `gc` object exposes the following methods. All methods are asynchronous and return JSON-serialized strings that must be parsed using `JSON.parse()`.

### `gc.activity(callback)`
Returns an object containing all standard telemetry time-series, as well as XData (Extended Data) time-series for the currently selected activity. XData series are suffixed with their variable names (e.g., `SmO2_oxy_hb`), and additionally include their own independent `_secs` and `_km` arrays to map their specific sample times and distances.
* **Returns:** `{ "watts": [...], "hr": [...], "SmO2_oxy_hb": [...], "SmO2_secs": [...], "SmO2_km": [...], ... }`

> **WARNING:** Avoid using `gc.activity()` for production charts. When loading long or high-resolution activities, serializing the entire dataset into a single massive JSON string can exceed QWebChannel/IPC memory limits, causing the method initialization to fail silently or crash (`gc.activity is not a function`). Instead, it is highly recommended to fetch only the required columns individually using `gc.series()`.

### `gc.activityMetrics(callback)`
Returns an object containing the computed summary metrics (e.g., TSS, IF, NP, Average Power) and metadata (date, time, sport, custom text fields) of the selected activity.
* **Returns:** `{ "date": "2023-01-01", "time": "12:00:00", "TSS": 120.5, "Average_Power": 210.0, ... }`

### `gc.series(metric, callback)`
Returns an array of data points for a single standard telemetry metric.
* **Arguments:** `metric` (String) - e.g., `"watts"`, `"hr"`, `"cad"`, `"km"`, `"secs"`.
* **Returns:** `[ 150, 155, 160, 162, ... ]`

### `gc.xdataSeries(deviceName, seriesName, callback)`
Returns an array of data points for a specific XData (Extended Data) metric. You can also request the time or distance arrays for the specific XData device by passing `"secs"` or `"km"`.
* **Arguments:** 
  * `deviceName` (String) - e.g., `"SmO2"`
  * `seriesName` (String) - e.g., `"oxy_hb"`, `"secs"`, or `"km"`
* **Returns:** `[ 45.2, 45.3, 45.1, ... ]`

### `gc.athlete(callback)`
Returns an object with basic physical and personal properties of the current athlete.
* **Returns:** `{ "name": "John Doe", "gender": "male", "weight": 75.0, "height": 180.0, "dob": "1990-01-01" }`

### `gc.athleteZones(dateStr, sportFilter, callback)`
Returns a dataframe-like object containing historical zone configurations (FTP, CP, W', LTHR, Pace) sorted by date.
* **Arguments:** 
  * `dateStr` (String, Optional) - Filter by a specific date (ISO format `"YYYY-MM-DD"`). Pass `""` for all history.
  * `sportFilter` (String, Optional) - Filter by sport (e.g., `"Bike"`, `"Run"`). Pass `""` for all sports.
* **Returns:** 
  ```json
  {
    "dates": ["2022-01-01", "2023-01-01"],
    "sports": ["Bike", "Bike"],
    "ftp": [250.0, 265.0],
    "wprime": [15000.0, 16000.0],
    "zoneslow": [[0, 140, 190, 230, 265, 300, 350], [0, 145, 195, 240, 280, 315, 370]],
    ...
  }
  ```

## Event Listeners

The bridge emits a signal when the user selects a different activity in GoldenCheetah, allowing the chart to automatically refresh.

* **`gc.activityChanged.connect(callback)`**: Fired whenever the selected activity changes.

## Chart Configuration Persistence (The `chart` object)

In addition to the `gc` object, GoldenCheetah exposes a `chart` object on the `channel.objects` property which can be used to interact with the chart's configuration state and GoldenCheetah's native "Perspectives" system.

This is especially useful for persisting user preferences (e.g., dropdown selections, customized thresholds, layout modes) across GoldenCheetah restarts.

* **`chart.getChartConfig(callback)`**: Asynchronously returns a JSON-serialized string of the saved configuration.
* **`chart.setChartConfig(jsonString)`**: Asynchronously saves a JSON-serialized string to GoldenCheetah's perspective configuration.

**Example Usage:**
```javascript
let chartBridge = null;

new QWebChannel(qt.webChannelTransport, function(channel) {
    gc = channel.objects.gc;
    chartBridge = channel.objects.chart; // Access the chart object

    // Load saved configuration
    if (chartBridge && typeof chartBridge.getChartConfig === 'function') {
        chartBridge.getChartConfig(function(configStr) {
            let config = JSON.parse(configStr || "{}");
            console.log("Loaded saved state:", config);
        });
    }
});

function saveState(stateObj) {
    if (chartBridge && typeof chartBridge.setChartConfig === 'function') {
        chartBridge.setChartConfig(JSON.stringify(stateObj));
    }
}
```

## Basic HTML Example

To use the API, you must include `qwebchannel.js` (injected automatically or loaded locally) and initialize the channel.

```html
<!DOCTYPE html>
<html>
<head>
    <title>Basic GC Activity Chart</title>
    <!-- qwebchannel.js is provided by QtWebEngine automatically in the environment -->
    <script type="text/javascript" src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <script type="text/javascript">
        var gc = null;

        // Helper to wrap async bridge calls in Promises
        function fetchSeries(metricName) {
            return new Promise(resolve => {
                if (!gc || typeof gc.series !== 'function') {
                    resolve([]);
                    return;
                }
                gc.series(metricName, function(response) {
                    try {
                        let data = JSON.parse(response);
                        resolve(data || []);
                    } catch(e) {
                        resolve([]);
                    }
                });
            });
        }

        function loadChartData() {
            if (!gc) return;
            
            // Fetch summary metrics
            gc.activityMetrics(function(response) {
                var metrics = JSON.parse(response);
                document.getElementById('summary').innerText = 
                    "Date: " + (metrics.date || "N/A") + " | TSS: " + (metrics.TSS || 0).toFixed(1);
            });

            // Fetch telemetry series in parallel
            Promise.all([
                fetchSeries("watts"),
                fetchSeries("hr")
            ]).then(([watts, hr]) => {
                document.getElementById('data').innerText = 
                    "Loaded " + watts.length + " power samples and " + hr.length + " hr samples.";
            });
        }

        window.onload = function() {
            // Initialize the QWebChannel
            new QWebChannel(qt.webChannelTransport, function(channel) {
                // The 'gc' object is exposed by HtmlActivitiesBridge
                gc = channel.objects.gc;
                
                // Load data for the first time
                loadChartData();

                // Listen for activity changes to refresh data (with safeguard)
                if (gc && gc.activityChanged && typeof gc.activityChanged.connect === 'function') {
                    gc.activityChanged.connect(function() {
                        console.log("Activity changed in GoldenCheetah. Refreshing...");
                        loadChartData();
                    });
                }
            });
        }
    </script>
</head>
<body>
    <h1>Activity Data</h1>
    <div id="summary">Loading metrics...</div>
    <div id="data">Loading series...</div>
</body>
</html>
```
