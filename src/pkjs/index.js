// Reads the next 11 upcoming events from the Pebble app's built-in Calendar plugin
// (calendar/event) and pushes them to the watch. This is the phone's own calendar
// sync (whatever calendars are enabled in the Pebble app's Calendar screen) --
// no ICS URL or companion app needed. Needs the Calendar permission; no debug setting.

var MAX_EVENTS = 11;
var subscription = null;

function pad(n) {
  return n < 10 ? '0' + n : '' + n;
}

function formatEventTime(instance) {
  var startsAt = instance.properties.starts_at || {};
  var allDay = instance.properties.all_day &&
    instance.properties.all_day.boolean && instance.properties.all_day.boolean.value;

  if (allDay) {
    return 'All day';
  }
  if (!startsAt.timestamp) {
    return (startsAt.shortText && startsAt.shortText.text) || '';
  }

  var date = new Date(startsAt.timestamp.value * 1000);
  var now = new Date();
  var sameDay = date.getFullYear() === now.getFullYear() &&
    date.getMonth() === now.getMonth() &&
    date.getDate() === now.getDate();
  var timeStr = pad(date.getHours()) + ':' + pad(date.getMinutes());

  if (sameDay) {
    return timeStr;
  }

  // Not today: one-character marker instead of a day name (the time column is narrow).
  return '+' + timeStr;
}

function nameOf(instance) {
  var name = instance.properties.name;
  return (name && (name.shortText || name.longText) &&
    (name.shortText ? name.shortText.text : name.longText.text)) || '';
}

function sendEvents(instances) {
  var dict = {};

  if (!instances || instances.length === 0) {
    dict.EVENT_1_TITLE = 'No upcoming events';
    dict.EVENT_1_TIME = '';
    for (var j = 2; j <= MAX_EVENTS; j++) {
      dict['EVENT_' + j + '_TITLE'] = '';
      dict['EVENT_' + j + '_TIME'] = '';
    }
  } else {
    for (var i = 0; i < MAX_EVENTS; i++) {
      var instance = instances[i];
      dict['EVENT_' + (i + 1) + '_TITLE'] = instance ? nameOf(instance) : '';
      dict['EVENT_' + (i + 1) + '_TIME'] = instance ? formatEventTime(instance) : '';
    }
  }

  Pebble.sendAppMessage(dict, function () {}, function (e) {
    console.log('Calendar: sendAppMessage failed: ' + JSON.stringify(e));
  });
}

function subscribe() {
  if (subscription) {
    subscription.unsubscribe();
  }
  subscription = Pebble.subscribeToSource({
    category: 'calendar',
    item: 'event',
    properties: ['name', 'starts_at', 'all_day'],
    onData: function (envelope) {
      sendEvents(envelope.instances.slice(0, MAX_EVENTS));
    },
    onError: function (err) {
      console.log('Calendar: calendar/event error: ' + err.code);
      if (err.code === 'PERMISSION_DENIED') {
        sendEvents([{
          properties: { name: { shortText: { text: 'Calendar permission needed' } } },
        }]);
      } else {
        sendEvents([]);
      }
    },
  });
}

// Settings, kept on the phone and sent to the watch (which also persists them).
//   timeFormat: 0 = 12-hour, 1 = 24-hour, 2 = follow the watch's own setting
//   dateStyle:  0 = words ("Nov 9"), 1 = numbers ("11/9")
//   dayFirst:   0 = month first, 1 = day first
//   separator:  0 = "/", 1 = "." (written "9.11."), 2 = "-"; numbers only
//   weekday:    1 = show the weekday, 0 = hide it
//   invert:     0 = white on black, 1 = black on white
//   tempUnit:   0 = Fahrenheit, 1 = Celsius (phone side only: the weather text is built here)
var DEFAULT_SETTINGS = { timeFormat: 0, dateStyle: 0, dayFirst: 0, separator: 0, weekday: 1,
  invert: 0, tempUnit: 0 };

function loadSettings() {
  var saved = {};
  try {
    saved = JSON.parse(localStorage.getItem('settings') || '{}') || {};
  } catch (e) {
    saved = {};
  }
  var out = {};
  for (var k in DEFAULT_SETTINGS) {
    out[k] = typeof saved[k] === 'number' ? saved[k] : DEFAULT_SETTINGS[k];
  }
  return out;
}

function saveSettings(settings) {
  try {
    localStorage.setItem('settings', JSON.stringify(settings));
  } catch (e) {
    console.log('Calendar: could not save settings: ' + e);
  }
}

function sendSettings(settings) {
  Pebble.sendAppMessage({
    TIME_FORMAT: settings.timeFormat,
    DATE_STYLE: settings.dateStyle,
    DATE_DAY_FIRST: settings.dayFirst,
    DATE_SEPARATOR: settings.separator,
    SHOW_WEEKDAY: settings.weekday,
    INVERT: settings.invert,
  }, function () {}, function (e) {
    console.log('Calendar: settings sendAppMessage failed: ' + JSON.stringify(e));
  });
}

function configPage(settings) {
  function opts(name, choices) {
    return choices.map(function (c) {
      return '<label><input type="radio" name="' + name + '" value="' + c[0] + '"' +
        (settings[name] === c[0] ? ' checked' : '') + '> ' + c[1] + '</label>';
    }).join('');
  }
  return '<!doctype html><html><head><meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width, initial-scale=1">' +
    '<title>Calendar Watchface</title>' +
    '<style>:root{color-scheme:light dark}' +
    'body{font:16px system-ui,-apple-system,sans-serif;margin:0;padding:20px;line-height:1.4}' +
    'h1{font-size:20px}h2{font-size:14px;text-transform:uppercase;letter-spacing:.05em;' +
    'opacity:.7;margin:24px 0 6px}label{display:block;padding:8px 0}' +
    'button{font:inherit;width:100%;padding:12px;margin-top:24px;border:0;border-radius:8px;' +
    'background:#d33;color:#fff}small{opacity:.7}</style></head><body>' +
    '<h1>Calendar Watchface</h1>' +
    '<form id="f">' +
    '<h2>Time</h2>' +
    opts('timeFormat', [[0, '12-hour'], [1, '24-hour'], [2, 'Follow the watch setting']]) +
    '<h2>Date</h2>' +
    opts('dateStyle', [[0, 'Words (Nov 9)'], [1, 'Numbers (11/9)']]) +
    '<h2>Date order</h2>' +
    opts('dayFirst', [[0, 'Month first (Nov 9, 11/9)'], [1, 'Day first (9 Nov, 9/11)']]) +
    '<h2>Number separator</h2>' +
    opts('separator', [[0, '11/9'], [1, '9.11.'], [2, '11-9']]) +
    '<small>Only used when the date is numbers.</small>' +
    '<h2>Weekday</h2>' +
    opts('weekday', [[1, 'Show the weekday'], [0, 'Hide the weekday']]) +
    '<h2>Colours</h2>' +
    opts('invert', [[0, 'White on black'], [1, 'Black on white (inverted)']]) +
    '<h2>Temperature</h2>' +
    opts('tempUnit', [[0, 'Fahrenheit (°F)'], [1, 'Celsius (°C)']]) +
    '<button type="submit">Save</button></form>' +
    '<p><small>Events come from the calendars synced in the Pebble app’s Calendar ' +
    'screen.</small></p>' +
    '<script>document.getElementById("f").addEventListener("submit",function(e){' +
    'e.preventDefault();var o={};["timeFormat","dateStyle","dayFirst","separator","weekday",' +
    '"invert","tempUnit"].forEach(function(n){o[n]=parseInt(document.querySelector(' +
    '"input[name="+n+"]:checked").value,10)});' +
    'document.location="pebblejs://close#"+encodeURIComponent(JSON.stringify(o))});</script>' +
    '</body></html>';
}

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL('data:text/html;charset=utf-8,' +
    encodeURIComponent(configPage(loadSettings())));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    return;
  }
  try {
    var chosen = JSON.parse(decodeURIComponent(e.response));
    var settings = loadSettings();
    for (var k in DEFAULT_SETTINGS) {
      if (typeof chosen[k] === 'number') {
        settings[k] = chosen[k];
      }
    }
    saveSettings(settings);
    sendSettings(settings);
    updateWeather();  // the unit may have changed
  } catch (err) {
    console.log('Calendar: bad settings response: ' + err);
  }
});

// Weather + sunrise/sunset from Open-Meteo (no API key), using the phone's location.
var WEATHER_REFRESH_MS = 30 * 60 * 1000;

function clockPart(isoLocal) {
  // Open-Meteo returns e.g. "2026-09-28T06:52" already in the location's local time.
  return isoLocal.split('T')[1];
}

function sendWeather(data) {
  var round = Math.round;
  var dict = {
    WEATHER: round(data.current.temperature_2m) + '\u00b0',
    RANGE: round(data.daily.temperature_2m_max[0]) + '\u00b0\n' +
      round(data.daily.temperature_2m_min[0]) + '\u00b0',
    SUN: clockPart(data.daily.sunrise[0]) + '\n' + clockPart(data.daily.sunset[0]),
  };
  Pebble.sendAppMessage(dict, function () {}, function (e) {
    console.log('Calendar: weather sendAppMessage failed: ' + JSON.stringify(e));
  });
}

function fetchWeather(pos) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + pos.coords.latitude + '&longitude=' + pos.coords.longitude +
    '&current=temperature_2m' +
    '&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset' +
    '&temperature_unit=' + (loadSettings().tempUnit === 1 ? 'celsius' : 'fahrenheit') +
    '&timezone=auto&forecast_days=1';
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    try {
      sendWeather(JSON.parse(xhr.responseText));
    } catch (e) {
      console.log('Calendar: weather parse failed: ' + e);
    }
  };
  xhr.onerror = function () {
    console.log('Calendar: weather request failed');
  };
  xhr.open('GET', url);
  xhr.send();
}

function updateWeather() {
  navigator.geolocation.getCurrentPosition(fetchWeather, function (err) {
    console.log('Calendar: location error: ' + err.message);
  }, { timeout: 30000, maximumAge: 15 * 60 * 1000 });
}

Pebble.addEventListener('ready', function () {
  sendSettings(loadSettings());
  subscribe();
  updateWeather();
  setInterval(updateWeather, WEATHER_REFRESH_MS);
});
