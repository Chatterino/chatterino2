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

/// Connect to port

let port = null;

function connectPort() {
  port = chrome.runtime.connectNative("com.chatterino.chatterino");
  console.log("port connected");

  port.onMessage.addListener(function(msg) {
    console.log(msg);
  });
  port.onDisconnect.addListener(function() {
    console.log("port disconnected");

    port = null;
  });
}

function getPort() {
  if (port) {
    return port;
  } else {
    // TODO: add cooldown
    connectPort();

    return port;
  }
}

/// Tab listeners

chrome.tabs.onActivated.addListener((activeInfo) => {
  chrome.tabs.get(activeInfo.tabId, (tab) => {
    if (!tab)
      return;

    if (!tab.url)
      return;

    matchUrl(tab.url);
  });
});

chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
  if (!tab.highlighted)
    return;

  matchUrl(changeInfo.url);
});


/// Misc

function matchUrl(url) {
  if (!url)
    return;

  const match = url.match(/^https?:\/\/(www\.)?twitch.tv\/([a-zA-Z0-9]+)\/?$/);

  if (match) {
    const channelName = match[2];

    if (!ignoredPages[channelName]) {
      selectChannel(channelName);
    }
  }
}

function selectChannel(channelName) {
  console.log("select" + channelName);

  let port = getPort();
  if (port) {
    port.postMessage({action: "select", type: "twitch", name: channelName});
  }
}
