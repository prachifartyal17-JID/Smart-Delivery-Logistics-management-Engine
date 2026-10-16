// Owner: Swastika - Route panel (doubly linked list). Cancelling a stop is O(1) in the backend.
function renderRoute(s) {
  const box = $("route-body");
  box.replaceChildren();
  s.route.forEach((name, i) => {
    if (i) box.append(el("span", " > ", "arrow"));
    box.append(el("span", name, "stop"));
  });
  const sel = $("stop-select");
  sel.replaceChildren();
  s.route.slice(1).forEach(n => sel.append(new Option(n, n)));
}

function setupRoute() {
  $("btn-cancel").onclick = () => act(() => call("cancelStop", { name: $("stop-select").value }), r => "Cancelled " + r.name);
}
