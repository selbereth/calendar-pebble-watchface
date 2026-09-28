// Reads the next 11 upcoming events from the Pebble app's built-in Calendar plugin
// (calendar/event) and pushes them to the watch. This is the phone's own calendar
// sync (whatever calendars are enabled in the Pebble app's Calendar screen) --
// no ICS URL or companion app needed.
//
// Requires the mobile app's experimental plugin API (v1.14.0+): Settings > Debug >
// Show debug options > "Use experimental plugins".

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

function configPage() {
  return '<!doctype html><html><head><meta charset="utf-8">' +
    '<meta name="viewport" content="width=device-width, initial-scale=1">' +
    '<title>Calendar Watchface</title>' +
    '<style>:root{color-scheme:light dark}' +
    'body{font:16px system-ui,-apple-system,sans-serif;margin:0;padding:20px;line-height:1.4}' +
    'h1{font-size:20px}ol{padding-left:20px}li{margin-bottom:10px}</style></head><body>' +
    '<h1>Calendar Watchface</h1>' +
    '<p>Shows your next 11 upcoming events, read from the Pebble app’s own calendar sync.</p>' +
    '<ol>' +
    '<li>In the Pebble app: <b>Settings &rarr; Debug &rarr; Show debug options</b>, then enable ' +
    '<b>Use experimental plugins</b> (this feature is still experimental).</li>' +
    '<li>Make sure your calendars are synced and enabled in the Pebble app’s <b>Calendar</b> ' +
    'screen.</li>' +
    '</ol>' +
    '<p>No further setup needed here — close this page.</p>' +
    '</body></html>';
}

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL('data:text/html;charset=utf-8,' + encodeURIComponent(configPage()));
});

// Weather + sunrise/sunset from Open-Meteo (no API key), using the phone's location.
// Units are Fahrenheit; change TEMP_UNIT to 'celsius' for Celsius.
var TEMP_UNIT = 'fahrenheit';
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
    '&temperature_unit=' + TEMP_UNIT + '&timezone=auto&forecast_days=1';
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
  subscribe();
  updateWeather();
  setInterval(updateWeather, WEATHER_REFRESH_MS);
});
