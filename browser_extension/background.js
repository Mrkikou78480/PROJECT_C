// background.js for Manifest V3 service worker
// Listens for messages and forwards them to the native host, returning the response.

function connectAndSend(payload) {
  return new Promise((resolve) => {
    const port = chrome.runtime.connectNative("com.project_c.passwords");
    let handled = false;
    port.onMessage.addListener((msg) => {
      handled = true;
      resolve({ ok: true, data: msg });
      port.disconnect();
    });
    port.onDisconnect.addListener(() => {
      if (!handled) {
        resolve({ ok: false, error: chrome.runtime.lastError });
      }
    });
    port.postMessage(payload);
  });
}

// Expose a simple message API so tests can call chrome.runtime.sendMessage
chrome.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (!message || !message.cmd) {
    sendResponse({ ok: false, error: "invalid_message" });
    return false;
  }

  if (message.cmd === "find_duplicates") {
    connectAndSend({ cmd: "find_duplicates" }).then((r) => sendResponse(r));
    return true; // indicates async response
  }

  if (message.cmd === "get_password") {
    // Expect {cmd:'get_password', site: '...', login: '...'}
    const payload = {
      cmd: "get_password",
      site: message.site || "",
      login: message.login || "",
    };
    connectAndSend(payload).then((r) => sendResponse(r));
    return true;
  }

  // unsupported
  sendResponse({ ok: false, error: "unknown_cmd" });
  return false;
});

// Log installation for debugging
chrome.runtime.onInstalled.addListener(() => {
  console.log("PROJECT_C bridge (MV3) installed");
});
// Add a click handler for the extension action (MV3)
if (chrome.action && chrome.action.onClicked) {
  chrome.action.onClicked.addListener(() => {
    connectAndSend({ cmd: "find_duplicates" }).then((r) =>
      console.log("action result", r)
    );
  });
}
