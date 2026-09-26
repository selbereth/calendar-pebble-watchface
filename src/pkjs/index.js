// Reads the next 3 upcoming events from the Pebble app's built-in Calendar plugin
// (calendar/event) and pushes them to the watch. This is the phone's own calendar
// sync (whatever calendars are enabled in the Pebble app's Calendar screen) --
// no ICS URL or companion app needed.
//
// Requires the mobile app's experimental plugin API (v1.14.0+): Settings > Debug >
// Show debug options > "Use experimental plugins".

var MAX_EVENTS = 3;
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

  var days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  return days[date.getDay()] + ' ' + timeStr;
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
    dict.EVENT_2_TITLE = '';
    dict.EVENT_2_TIME = '';
    dict.EVENT_3_TITLE = '';
    dict.EVENT_3_TIME = '';
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
    '<p>Shows your next 3 upcoming events, read from the Pebble app’s own calendar sync.</p>' +
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

Pebble.addEventListener('ready', function () {
  subscribe();
});
