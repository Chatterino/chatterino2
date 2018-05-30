const ignoredPages = {
  "settings": true,
  "payments": true,
  "inventory": true,
  "messages": true,
  "subscriptions": true,
  "friends": true,
  "directory": true,
};

/// return channel name if it should contain a chat
function matchChannelName(url) {
  if (!url)
    return undefined;

  const match = url.match(/^https?:\/\/(www\.)?twitch.tv\/([a-zA-Z0-9_]+)\/?$/);

  let channelName;
  if (match && (channelName = match[2], !ignoredPages[channelName])) {
    return channelName;
  }

  return undefined;
}


const appName = "com.chatterino.chatterino";
let port = null;

// gets the port for communication with chatterino
function getPort() {
  if (port) {
    return port;
  } else {
    // TODO: add cooldown
    connectPort();

    return port;
  }
}

/// connect to port
function connectPort() {
  port = chrome.runtime.connectNative(appName);
  console.log("port connected");
  let connected = true;

  port.onMessage.addListener(function (msg) {
    console.log(msg);
  });
  port.onDisconnect.addListener(function () {
    console.log("port disconnected");

    port = null;
  });

  let sendPing = () => {
    if (connected) {
      port.postMessage({ ping: true });
    } else {
      setTimeout(sendPing, 5000);
    }
  }
}


// tab activated
chrome.tabs.onActivated.addListener((activeInfo) => {
  chrome.tabs.get(activeInfo.tabId, (tab) => {
    if (!tab || !tab.url) return;

    chrome.windows.get(tab.windowId, {}, (window) => {
      if (!window.focused) return;

      if (window.state == "fullscreen") {
        tryDetach(tab.windowId);
        return;
      }

      console.log("onActivated");
      onTabSelected(tab.url, tab);
    });
  });
});

// url changed
chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
  if (!tab.highlighted)
    return;

  chrome.windows.get(tab.windowId, {}, (window) => {
    if (!window.focused) return;
    if (window.state == "fullscreen") {
      tryDetach(tab.windowId);
      return;
    }

    console.log("onUpdated");
    onTabSelected(tab.url, tab);
  });
});

// tab detached
chrome.tabs.onDetached.addListener((tabId, detachInfo) => {
  console.log("onDetached");
  tryDetach(detachInfo.oldWindowId);
});

// tab closed
chrome.windows.onRemoved.addListener((windowId) => {
  console.log("onRemoved");
  tryDetach(windowId);
});

// window selected
chrome.windows.onFocusChanged.addListener((windowId) => {
  console.log(windowId);
  if (windowId == -1) return;

  // this returns all tabs when the query fails
  chrome.tabs.query({ windowId: windowId, highlighted: true }, (tabs) => {
    if (tabs.length === 1) {
      let tab = tabs[0];

      chrome.windows.get(tab.windowId, (window) => {
        if (window.state == "fullscreen") {
          tryDetach(tab.windowId);
          return;
        }

        console.log("onFocusChanged");
        onTabSelected(tab.url, tab);
      });
    }
  });
});


// attach or detach from tab
function onTabSelected(url, tab) {
  let channelName = matchChannelName(url);

  if (channelName) {
    // chrome.windows.get(tab.windowId, {}, (window) => {
    //   // attach to window
    //   tryAttach(tab.windowId, {
    //     name: channelName,
    //     yOffset: window.height - tab.height,
    //   });
    // });
  } else {
    // detach from window
    tryDetach(tab.windowId);
  }
}

// receiving messages from the inject script
chrome.runtime.onMessage.addListener((message, sender, callback) => {
  console.log(message);

  // is tab highlighted
  if (!sender.tab.highlighted) return;

  // is window focused
  chrome.windows.get(sender.tab.windowId, {}, (window) => {
    if (!window.focused) return;
    if (window.state == "fullscreen") {
      tryDetach(sender.tab.windowId);
      return;
    }

    // get zoom value
    chrome.tabs.getZoom(sender.tab.id, (zoom) => {
      let size = {
        width: Math.floor(message.rect.width * zoom),
        height: Math.floor(message.rect.height * zoom),
      };

      console.log(zoom);

      // attach to window
      tryAttach(sender.tab.windowId, {
        name: matchChannelName(sender.tab.url),
        size: size,
      });
    });
  });
});


// attach chatterino to a chrome window
function tryAttach(windowId, data) {
  data.action = "select";
  data.attach = true;
  data.type = "twitch";
  data.winId = "" + windowId;

  let port = getPort();

  if (port) {
    port.postMessage(data);
  }
}

// detach chatterino from a chrome window
function tryDetach(windowId) {
  let port = getPort();

  console.log("tryDetach");

  if (port) {
    port.postMessage({
      action: "detach",
      winId: "" + windowId
    })
  }
}
