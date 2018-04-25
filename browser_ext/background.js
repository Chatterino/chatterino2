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


/// Connect to port
function connectPort() {
  port = chrome.runtime.connectNative("com.chatterino.chatterino");
  console.log("port connected");

  port.onMessage.addListener(function (msg) {
    console.log(msg);
  });
  port.onDisconnect.addListener(function () {
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

    matchUrl(tab.url, tab);
  });
});

chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
  if (!tab.highlighted)
    return;

  matchUrl(changeInfo.url, tab);
});


/// Misc
function matchUrl(url, tab) {
  if (!url)
    return;

  const match = url.match(/^https?:\/\/(www\.)?twitch.tv\/([a-zA-Z0-9]+)\/?$/);

  let channelName;

  console.log(tab);

  if (match && (channelName = match[2], !ignoredPages[channelName])) {
    console.log("channelName " + channelName);
    console.log("winId " + tab.windowId);

    chrome.windows.get(tab.windowId, {}, (window) => {
      let yOffset = window.height - tab.height;

      let port = getPort();
      if (port) {
        port.postMessage({
          action: "select",
          attach: true,
          type: "twitch",
          name: channelName,
          winId: "" + tab.windowId,
          yOffset: yOffset
        });
      }
    });
  } else {
    let port = getPort();
    if (port) {
      port.postMessage({
        action: "detach",
        winId: "" + tab.windowId
      })
    }
  }
}
