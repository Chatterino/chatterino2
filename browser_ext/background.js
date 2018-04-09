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

var port = chrome.runtime.connectNative("com.chatterino.chatterino");

port.onMessage.addListener(function(msg) {
  console.log(msg);
});
port.onDisconnect.addListener(function() {
  console.log("Disconnected");
});
port.postMessage({ text: "Hello, my_application" });

function selectChannel(channelName) {
  console.log(channelName);

  port.postMessage({channelName: channelName});

  // chrome.runtime.sendNativeMessage(appName, { "xd": true }, (resp) => {
  //   console.log(resp);
  // })
}

/// add listeners
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
