var icsParser = require('./ics-parser');

var LOOKAHEAD_DAYS = 30;
var REFRESH_INTERVAL_MS = 15 * 60 * 1000;
var MAX_EVENTS = 3;

function getIcsUrl() {
  try {
    return localStorage.getItem('icsUrl') || '';
  } catch (e) {
    return '';
  }
}

function setIcsUrl(url) {
  try {
    localStorage.setItem('icsUrl', url);
  } catch (e) {
    // ignore storage errors
  }
}

function pad(n) {
  return n < 10 ? '0' + n : '' + n;
}

function formatEventTime(date, now) {
  var sameDay = date.getFullYear() === now.getFullYear() &&
    date.getMonth() === now.getMonth() &&
    date.getDate() === now.getDate();

  var hours = date.getHours();
  var minutes = pad(date.getMinutes());
  var timeStr = pad(hours) + ':' + minutes;

  if (sameDay) {
    return timeStr;
  }

  var days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
  return days[date.getDay()] + ' ' + timeStr;
}

function sendEvents(events, now) {
  var dict = {};

  if (events.length === 0) {
    dict['EVENT_1_TITLE'] = 'No upcoming events';
    dict['EVENT_1_TIME'] = '';
    dict['EVENT_2_TITLE'] = '';
    dict['EVENT_2_TIME'] = '';
    dict['EVENT_3_TITLE'] = '';
    dict['EVENT_3_TIME'] = '';
  } else {
    for (var i = 0; i < MAX_EVENTS; i++) {
      var evt = events[i];
      dict['EVENT_' + (i + 1) + '_TITLE'] = evt ? evt.title : '';
      dict['EVENT_' + (i + 1) + '_TIME'] = evt ? formatEventTime(evt.start, now) : '';
    }
  }

  Pebble.sendAppMessage(dict, function () {
    console.log('Calendar: events sent to watch');
  }, function (e) {
    console.log('Calendar: failed to send events: ' + JSON.stringify(e));
  });
}

function fetchAndSend() {
  var icsUrl = getIcsUrl();
  var now = new Date();

  if (!icsUrl) {
    sendEvents([], now);
    return;
  }

  var horizonEnd = new Date(now.getTime() + LOOKAHEAD_DAYS * 24 * 60 * 60 * 1000);

  xhrRequest(icsUrl, 'GET', function (responseText) {
    var events;
    try {
      events = icsParser.parseICS(responseText, now, horizonEnd);
    } catch (e) {
      console.log('Calendar: failed to parse ICS feed: ' + e);
      events = [];
    }
    sendEvents(events.slice(0, MAX_EVENTS), now);
  }, function (error) {
    console.log('Calendar: failed to fetch ICS feed: ' + error);
    sendEvents([], now);
  });
}

function xhrRequest(url, type, onSuccess, onError) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    if (xhr.status >= 200 && xhr.status < 300) {
      onSuccess(xhr.responseText);
    } else {
      onError('HTTP ' + xhr.status);
    }
  };
  xhr.onerror = function () {
    onError('network error');
  };
  xhr.open(type, url);
  xhr.send();
}

function buildConfigHtml() {
  var currentUrl = getIcsUrl();
  return '<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">' +
    '<title>Calendar Watchface Settings</title>' +
    '<style>body{font-family:sans-serif;padding:16px;}label{display:block;margin-bottom:8px;font-weight:bold;}' +
    'input{width:100%;padding:8px;font-size:16px;box-sizing:border-box;}' +
    'button{margin-top:16px;padding:10px 16px;font-size:16px;}' +
    'p{color:#555;font-size:13px;}</style></head><body>' +
    '<label for="icsUrl">Calendar feed (.ics) URL</label>' +
    '<input type="text" id="icsUrl" value="' + currentUrl.replace(/"/g, '&quot;') + '" placeholder="https://.../basic.ics">' +
    '<p>Use the secret iCal (.ics) address from your calendar provider (e.g. Google Calendar &rarr; Settings &rarr; ' +
    'Integrate calendar &rarr; Secret address in iCal format).</p>' +
    '<button onclick="save()">Save</button>' +
    '<script>function save(){var url=document.getElementById("icsUrl").value;' +
    'document.location="pebblejs://close#"+encodeURIComponent(JSON.stringify({icsUrl:url}));}</script>' +
    '</body></html>';
}

Pebble.addEventListener('ready', function () {
  console.log('Calendar: PebbleKit JS ready');
  fetchAndSend();
  setInterval(fetchAndSend, REFRESH_INTERVAL_MS);
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL('data:text/html,' + encodeURIComponent(buildConfigHtml()));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    return;
  }
  try {
    var settings = JSON.parse(decodeURIComponent(e.response));
    setIcsUrl(settings.icsUrl || '');
    fetchAndSend();
  } catch (err) {
    console.log('Calendar: failed to parse configuration response: ' + err);
  }
});
