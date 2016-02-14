Pebble.addEventListener("ready", function (e) {
  console.log("helloworld");
});

Pebble.addEventListener("appmessage", function (e) {
  //App is ready to receive JS messages
  console.log(JSON.stringify(e));
  switch (e.payload.MESSAGE_TYPE) {
    case 105:
      postSleep(e);
      break;
    case 106:
      postWake(e);
      break;
  }
});

var baseUrl = "http://695ded68.ngrok.com/";

function postSleep(e) {
  var req = new XMLHttpRequest();
  var isAsleep = e.payload.KEY_ASLEEP;
  var params = "sleep=" + isAsleep;
  console.log("params: " + params);
  var url = baseUrl + "sleep";
  req.open("POST", url, true);
  req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  req.onload = function (e) {
    console.log("got something: " + JSON.stringify(e));
    if (req.readyState == 4 && req.state == 200) {
      console.log("response OK");
    }
  };
  req.send(params);
}

function postWake(e) {
  var req = new XMLHttpRequest();
  var hour = e.payload.KEY_WAKE_HOUR;
  var minute = e.payload.KEY_WAKE_MINUTE;
  var params = "wake=" + hour + ":" + minute;
  console.log("params: " + params);
  var url = baseUrl + "wake";
  req.open("POST", url, true);
  req.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
  req.onload = function (e) {
    console.log("got something: " + JSON.stringify(e));
    Pebble.showSimpleNotificationOnPebble("Climate Connect", "Confirmed! Enjoy your sleep!");
    console.log("req: " + JSON.stringify(req));
    if (req.readyState == 4 && req.state == 200) {
      console.log("response OK");
    }
  };
  req.send(params);
}