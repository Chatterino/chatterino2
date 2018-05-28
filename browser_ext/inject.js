(() => {
  let lastRect = {};

  function log(str) {
    console.log("Chatterino Native: " + str);
  }

  function findChatDiv() {
    return document.getElementsByClassName("right-column")[0];
  }

  function queryChatRect() {
    let element = findChatDiv();

    if (element === undefined) {
      log("failed to find chat div");
      return;
    }

    // element.firstChild.style.opacity = 0;

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

    chrome.runtime.sendMessage(data, (response) => {
      // log("received message response");
      // console.log(response)
    });
  }

  function queryCharRectLoop() {
    let t1 = performance.now();
    queryChatRect();
    let t2 = performance.now();
    console.log("queryCharRect " + (t2 - t1) + "ms");
    setTimeout(queryCharRectLoop, 500);
  }

  queryCharRectLoop();
  window.addEventListener("resize", queryChatRect);

  log("initialized");
})()
