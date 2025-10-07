document.getElementById("btn").addEventListener("click", async () => {
  const site = document.getElementById("site").value.trim();
  const login = document.getElementById("login").value.trim();
  const out = document.getElementById("out");
  out.textContent = "Please wait...";
  try {
    const resp = await chrome.runtime.sendMessage({
      cmd: "get_password",
      site,
      login,
    });
    if (!resp) throw new Error("no-response");
    if (resp.ok && resp.password !== undefined) {
      out.textContent = resp.password;
    } else {
      out.textContent = "Error: " + (resp.error || JSON.stringify(resp));
    }
  } catch (e) {
    out.textContent = "Failed: " + e.message;
  }
});
