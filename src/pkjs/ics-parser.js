// Minimal iCalendar (RFC 5545) parser: extracts VEVENTs and expands simple
// recurrences (DAILY/WEEKLY) within a lookahead window. Not a full RRULE
// implementation -- covers the common cases produced by Google/Outlook/iCloud
// calendar feeds for "what's coming up next" purposes.

var DAY_MS = 24 * 60 * 60 * 1000;

function unfoldLines(text) {
  // RFC 5545: continuation lines start with a single space or tab.
  return text.replace(/\r\n[ \t]/g, '').replace(/\n[ \t]/g, '').split(/\r\n|\n/);
}

function parseDateValue(value, params) {
  // value forms: 20260926T140000Z, 20260926T140000, 20260926 (all-day)
  var isUTC = /Z$/.test(value);
  var m = value.match(/^(\d{4})(\d{2})(\d{2})(T(\d{2})(\d{2})(\d{2}))?/);
  if (!m) {
    return null;
  }
  var year = parseInt(m[1], 10);
  var month = parseInt(m[2], 10) - 1;
  var day = parseInt(m[3], 10);
  var hour = m[5] ? parseInt(m[5], 10) : 0;
  var minute = m[6] ? parseInt(m[6], 10) : 0;
  var second = m[7] ? parseInt(m[7], 10) : 0;
  var allDay = !m[4];

  var date;
  if (isUTC) {
    date = new Date(Date.UTC(year, month, day, hour, minute, second));
  } else {
    // Treat as local time (TZID handling is out of scope for this simple parser).
    date = new Date(year, month, day, hour, minute, second);
  }
  return { date: date, allDay: allDay };
}

function parseLineParams(rawKey) {
  var parts = rawKey.split(';');
  var params = {};
  for (var i = 1; i < parts.length; i++) {
    var kv = parts[i].split('=');
    if (kv.length === 2) {
      params[kv[0]] = kv[1];
    }
  }
  return params;
}

function parseRRule(value) {
  var rule = {};
  var parts = value.split(';');
  for (var i = 0; i < parts.length; i++) {
    var kv = parts[i].split('=');
    if (kv.length === 2) {
      rule[kv[0]] = kv[1];
    }
  }
  return rule;
}

function expandRecurrence(startInfo, rrule, horizonEnd) {
  var occurrences = [];
  var freq = rrule.FREQ;
  var interval = rrule.INTERVAL ? parseInt(rrule.INTERVAL, 10) : 1;
  var count = rrule.COUNT ? parseInt(rrule.COUNT, 10) : null;
  var until = null;
  if (rrule.UNTIL) {
    var untilInfo = parseDateValue(rrule.UNTIL, {});
    if (untilInfo) {
      until = untilInfo.date;
    }
  }

  if (freq !== 'DAILY' && freq !== 'WEEKLY') {
    // Unsupported recurrence frequency for expansion: just return the base
    // occurrence so at least the original event can surface if upcoming.
    occurrences.push(startInfo.date);
    return occurrences;
  }

  var stepMs = (freq === 'DAILY' ? 1 : 7) * DAY_MS * interval;
  var cursor = new Date(startInfo.date.getTime());
  var generated = 0;
  var maxIterations = 500;

  while (cursor.getTime() <= horizonEnd.getTime() && maxIterations > 0) {
    maxIterations--;
    if (until && cursor.getTime() > until.getTime()) {
      break;
    }
    occurrences.push(new Date(cursor.getTime()));
    generated++;
    if (count && generated >= count) {
      break;
    }
    cursor = new Date(cursor.getTime() + stepMs);
  }

  return occurrences;
}

// Parses raw ICS text and returns an array of { title, start, allDay }
// entries whose start time falls between `now` and `horizonEnd`.
function parseICS(icsText, now, horizonEnd) {
  var lines = unfoldLines(icsText);
  var events = [];

  var inEvent = false;
  var summary = '';
  var dtstart = null;
  var rrule = null;

  for (var i = 0; i < lines.length; i++) {
    var line = lines[i];
    if (line === 'BEGIN:VEVENT') {
      inEvent = true;
      summary = '';
      dtstart = null;
      rrule = null;
      continue;
    }
    if (line === 'END:VEVENT') {
      if (inEvent && dtstart) {
        if (rrule) {
          var occurrences = expandRecurrence(dtstart, rrule, horizonEnd);
          for (var o = 0; o < occurrences.length; o++) {
            if (occurrences[o].getTime() >= now.getTime() &&
                occurrences[o].getTime() <= horizonEnd.getTime()) {
              events.push({ title: summary, start: occurrences[o], allDay: dtstart.allDay });
            }
          }
        } else if (dtstart.date.getTime() >= now.getTime() &&
                   dtstart.date.getTime() <= horizonEnd.getTime()) {
          events.push({ title: summary, start: dtstart.date, allDay: dtstart.allDay });
        }
      }
      inEvent = false;
      continue;
    }
    if (!inEvent) {
      continue;
    }

    var colonIndex = line.indexOf(':');
    if (colonIndex === -1) {
      continue;
    }
    var rawKey = line.substring(0, colonIndex);
    var value = line.substring(colonIndex + 1);
    var key = rawKey.split(';')[0];

    if (key === 'SUMMARY') {
      summary = value.replace(/\\,/g, ',').replace(/\\n/gi, ' ').replace(/\\\\/g, '\\');
    } else if (key === 'DTSTART') {
      var params = parseLineParams(rawKey);
      dtstart = parseDateValue(value, params);
    } else if (key === 'RRULE') {
      rrule = parseRRule(value);
    }
  }

  events.sort(function (a, b) {
    return a.start.getTime() - b.start.getTime();
  });

  return events;
}

module.exports = {
  parseICS: parseICS
};
