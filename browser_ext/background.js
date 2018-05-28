const ignoredPages = {
  "settings": true,
  "payments": true,
  "inventory": true,
  "messages": true,
  "subscriptions": true,
  "friends": true,
  "directory": true,
};

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

  port.onMessage.addListener(function (msg) {
    console.log(msg);
  });
  port.onDisconnect.addListener(function () {
    console.log("port disconnected");

    port = null;
  });
}


// tab activated
chrome.tabs.onActivated.addListener((activeInfo) => {
  console.log(0)
  chrome.tabs.get(activeInfo.tabId, (tab) => {
    console.log(1)
    if (!tab || !tab.url) return;

    console.log(2)
    chrome.windows.get(tab.windowId, {}, (window) => {
      if (!window.focused) return;
      console.log(3)

      onTabSelected(tab.url, tab);
    });
  });
});

// url changed
chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
  if (!tab.highlighted)
    return;

  onTabSelected(changeInfo.url, tab);
});

// tab detached
chrome.tabs.onDetached.addListener((tabId, detachInfo) => {
  tryDetach(detachInfo.oldWindowId);
});

// tab closed
chrome.windows.onRemoved.addListener((windowId) => {
  tryDetach(windowId);
});

// window selected
chrome.windows.onFocusChanged.addListener((windowId) => {
  chrome.tabs.query({windowId: windowId, highlighted: true}, (tabs) => {
    if (tabs.length >= 1) {
      let tab = tabs[0];

      onTabSelected(tab.url, tab);
    }
  });
});


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

// attach or detach from tab
function onTabSelected(url, tab) {
  let channelName = matchChannelName(url);

  if (channelName) {
    chrome.windows.get(tab.windowId, {}, (window) => {
      // attach to window
      tryAttach(tab.windowId, {
        name: channelName,
        yOffset: window.height - tab.height,
      });
    });
  } else {
    // detach from window
    tryDetach(tab.windowId);
  }
}

// receiving messages from the inject script
function registerTheGarbage() {
  chrome.runtime.onMessage.addListener((message, sender, callback) => {
    // is tab highlighted
    if (!sender.tab.highlighted) return;

    // is window focused
    chrome.windows.get(sender.tab.windowId, {}, (window) => {
      if (!window.focused) return;

      // get zoom value
      chrome.tabs.getZoom(sender.tab.id, (zoom) => {
        let size = {
          width: message.rect.width * zoom,
          height: message.rect.height * zoom,
        };

        // attach to window
        tryAttach(sender.tab.windowId, {
          name: matchChannelName(sender.tab.url),
          size: size,
        })
      });
    });
  });
}

function registerLoop() {
  // loop until the runtime objects exists because I can't be arsed to figure out the proper way to do this
  if (chrome.runtime === undefined) {
    setTimeout(registerLoop(), 100);
    return;
  }

  registerTheGarbage();
}
registerLoop();

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

  if (port) {
    port.postMessage({
      action: "detach",
      winId: "" + windowId
    })
  }
}
