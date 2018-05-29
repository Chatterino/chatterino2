(() => {
  let lastRect = {};
  let port = null;

  function log(str) {
    console.log("Chatterino Native: " + str);
  }

  function findChatDiv() {
    return document.getElementsByClassName("right-column")[0];
  }

  function queryChatRect() {
    if (!matchChannelName(window.location.href)) return;

    let element = findChatDiv();

    if (element === undefined) {
      log("failed to find chat div");
      return;
    }

    if (!element.chatterino) {
      let xd = element.getElementsByClassName("channel-page__right-column")[0]

      if (xd != undefined) {
        element.chatterino = true;
        xd.style.opacity = 0;

        setTimeout(() => {
          xd.innerHTML = "<div style='width: 340px; height: 100%; justify-content: center; display: flex; flex-direction: column; text-align: center; color: #999; user-select: none;'>" +
            "Disconnected from the chatterino extension.<br><br>Please refresh the page." +
            "</div>";
          xd.style.opacity = 1;
        }, 2000);
      }
    }

    let rect = element.getBoundingClientRect();

    // if (
    //   lastRect.left == rect.left &&
    //   lastRect.right == rect.right &&
    //   lastRect.top == rect.top &&
    //   lastRect.bottom == rect.bottom
    // ) {
    //   // log("skipped sending message");
    //   return;
    // }
    lastRect = rect;

    let data = {
      rect: rect,
    };

    try {
      chrome.runtime.sendMessage(data);
    } catch {
      // failed to send a message to the runtime -> maybe the extension got reloaded
      // alert("reload the page to re-enable chatterino native");
    }
  }

  function queryCharRectLoop() {
    let t1 = performance.now();
    queryChatRect();
    let t2 = performance.now();
    console.log("queryCharRect " + (t2 - t1) + "ms");
    // setTimeout(queryCharRectLoop, 500);
  }

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

  queryCharRectLoop();
  window.addEventListener("load", () => {
    setTimeout(queryChatRect, 1000);
  });
  window.addEventListener("resize", queryChatRect);
  window.addEventListener("focus", queryChatRect);
  window.addEventListener("mouseup", () => {
    setTimeout(queryChatRect, 10);
  });

  log("initialized");
})()
