(() => {
  let lastRect = {};
  let port = null;

  let installedObjects = {};
  let rightCollapseButton = null;
  let isCollapsed = false;

  const ignoredPages = {
    "settings": true,
    "payments": true,
    "inventory": true,
    "messages": true,
    "subscriptions": true,
    "friends": true,
    "directory": true,
  };

  let findChatDiv = () => document.getElementsByClassName("right-column")[0];
  let findRightCollapse = () => document.getElementsByClassName("right-column__toggle-visibility")[0];
  let findRightColumn = () => document.getElementsByClassName("channel-page__right-column")[0];
  let findNavBar = () => document.getElementsByClassName("top-nav__menu")[0];

  // logging function
  function log(str) {
    console.log("Chatterino Native: " + str);
  }

  // install events
  function installChatterino() {
    log("trying to install events");

    let retry = false;

    // right collapse button
    if (!installedObjects.rightCollapse) {
      retry = true;

      let x = findRightCollapse();

      if (x != undefined) {
        rightCollapseButton = x;

        x.addEventListener("mouseup", () => {
          let y = findChatDiv();

          if (parseInt(y.style.width) == 0) {
            y.style.width = "340px";
            isCollapsed = false;
          } else {
            y.style.width = 0;
            isCollapsed = true;
          }
        });

        installedObjects.rightCollapse = true;
      }
    }

    // right column
    if (!installedObjects.rightColumn && installedObjects.rightCollapse) {
      let x = findChatDiv();

      if (x != undefined && x.children.length >= 2) {
        x.children[0].innerHTML = "<div style='width: 340px; height: 100%; justify-content: center; display: flex; flex-direction: column; text-align: center; color: #999; user-select: none; background: #222;'>" +
          "Disconnected from the chatterino extension.<br><br>Please focus the window or refresh the page." +
          "</div>";

        installedObjects.rightColumn = true;
      } else {
        retry = true;
      }
    }

    // nav bar
    if (!installedObjects.topNav) {
      if (rightCollapseButton) {
        let x = findNavBar();

        x.addEventListener("mouseup", () => {
          if (!isCollapsed) {
            let collapse = findRightCollapse();
            collapse.click();
          }
        });

        installedObjects.topNav = true;
      } else {
        retry = true;
      }
    }

    // retry if needed
    if (retry) {
      setTimeout(installChatterino, 1000);
    } else {
      log("installed all events");
    }
  }

  // query the rect of the chat
  function queryChatRect() {
    if (!matchChannelName(window.location.href)) return;

    let element = findChatDiv();

    if (element === undefined) {
      log("failed to find chat div");
      return;
    }

    let rect = element.getBoundingClientRect();

    /* if (
      lastRect.left == rect.left &&
      lastRect.right == rect.right &&
      lastRect.top == rect.top &&
      lastRect.bottom == rect.bottom
    ) {
      // log("skipped sending message");
      return;
    } */
    lastRect = rect;

    let data = {
      rect: rect,
    };

    isCollapsed = rect.width == 0;

    try {
      chrome.runtime.sendMessage(data);
    } catch {
      // failed to send a message to the runtime -> maybe the extension got reloaded
      // alert("reload the page to re-enable chatterino native");
    }
  }

  function queryChatRectLoop() {
    let t1 = performance.now();
    queryChatRect();
    let t2 = performance.now();
    console.log("queryCharRect " + (t2 - t1) + "ms");
    // setTimeout(queryCharRectLoop, 500);
  }

  // return channel name if it should contain a chat or undefined
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


  // event listeners
  window.addEventListener("load", () => setTimeout(queryChatRect, 1000));
  window.addEventListener("resize", queryChatRect);
  window.addEventListener("focus", queryChatRect);
  window.addEventListener("mouseup", () => setTimeout(queryChatRect, 10));
  window.addEventListener("hashchange", () => {
    installedObjects = {};
    installChatterino();
  });

  //
  log("hello there in the dev tools 👋");

  queryChatRectLoop();
  installChatterino();
})()
