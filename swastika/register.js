// Owner: Swastika - Register panel. The Trie lives in the C++ backend; this file only shows its answers.
let chosenPin = null;

async function onDestInput() {
  chosenPin = null;
  const list = $("suggest");
  list.replaceChildren();
  const hits = await call("suggest", { prefix: $("dest").value.trim() });
  hits.forEach(h => {
    const li = el("li", h.pin + "  " + h.name);
    li.onclick = () => { $("dest").value = h.pin + " " + h.name; chosenPin = h.pin; list.replaceChildren(); };
    list.append(li);
  });
}

async function updateEstimate() {
  const w = +$("weight").value, d = +$("distance").value;
  if (!(w > 0 && d > 0)) { $("estimate").textContent = ""; return; }
  const q = await call("quote", { service: $("service").value, weight: w, distance: d });
  $("estimate").textContent = "Estimated charge: " + money(q.cost);
}

function setupRegister() {
  $("dest").oninput = onDestInput;
  ["service", "weight", "distance"].forEach(id => { $(id).oninput = updateEstimate; });
  $("btn-register").onclick = () => act(async () => {
    const p = await call("register", { pincode: chosenPin, service: $("service").value, weight: +$("weight").value, distance: +$("distance").value });
    $("dest").value = ""; chosenPin = null;
    return p;
  }, p => "Registered " + p.id);
  updateEstimate();
}
