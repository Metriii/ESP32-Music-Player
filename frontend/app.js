var songs = [];    // músicas listadas pela ESP32: [{id, name}]
var activeId = ""; // id da música tocando ("" = nenhuma)

function $(id) {
  return document.getElementById(id);
}

/* ---------- Mensagens e conexão ---------- */

function setConn(ok) {
  $("conn").textContent = ok ? "conectado" : "offline";
  $("conn").className = ok ? "on" : "";
}

function msg(text) {
  $("msg").textContent = text;
}

// Falha de rede ou resposta que não é JSON (ex.: sem ESP32 por trás) = sem conexão.
function onError(e) {
  if (e instanceof TypeError || e instanceof SyntaxError) {
    setConn(false);
    msg("Sem conexão");
    return;
  }
  msg(e.message);
}

/* ---------- Comunicação com a ESP32 ---------- */

// fetch que devolve o JSON e lança erro quando o HTTP não é 2xx.
function api(url, options) {
  return fetch(url, options).then(function (r) {
    return r.json().then(function (data) {
      setConn(true);
      if (!r.ok) throw new Error(data.error || "erro");
      return data;
    });
  });
}

/* ---------- Interface ---------- */

// Atualiza "Tocando" e destaca o card da música atual.
function render(status) {
  activeId = status.playing ? status.id : "";
  $("cur").textContent = status.playing ? status.name : "nenhuma";

  songs.forEach(function (s) {
    var card = $("s-" + s.id);
    card.className = "card" + (s.id === activeId ? " active" : "");
  });
}

// Monta um card: nome + botão Tocar + botão apagar.
function createCard(song) {
  var card = document.createElement("div");
  card.className = "card";
  card.id = "s-" + song.id;

  var name = document.createElement("span");
  name.textContent = song.name; // textContent: nunca interpreta HTML

  var play = document.createElement("button");
  play.textContent = "Tocar";
  play.onclick = function () {
    api("/play?id=" + song.id).then(render).catch(onError);
  };

  var del = document.createElement("button");
  del.textContent = "×";
  del.className = "x";
  del.title = "Apagar";
  del.onclick = function () {
    if (!confirm("Apagar " + song.name + "?")) return;
    api("/delete?id=" + song.id).then(loadSongs).catch(onError);
  };

  card.appendChild(name);
  card.appendChild(play);
  card.appendChild(del);
  return card;
}

function buildList() {
  var list = $("list");
  list.textContent = "";
  songs.forEach(function (s) {
    list.appendChild(createCard(s));
  });
}

/* ---------- Ações ---------- */

// Busca a lista de músicas e o estado atual.
function loadSongs() {
  return api("/songs")
    .then(function (data) {
      songs = data;
      buildList();
      return api("/status");
    })
    .then(render)
    .catch(onError);
}

// A cada 2 s sincroniza o estado com a ESP32.
function poll() {
  api("/status")
    .then(function (status) {
      var unknown = status.playing && !$("s-" + status.id);
      if (unknown) return loadSongs(); // música nova que a página não conhece
      render(status);
    })
    .catch(function () {
      setConn(false);
    });
}

function upload() {
  var file = $("file").files[0];
  if (!file) return msg("Escolha um arquivo");
  if (file.size > 4096) return msg("Arquivo maior que 4 KB");

  var fileName = file.name.replace(/\.[^.]*$/, "");
  var id = fileName.toLowerCase().replace(/[^a-z0-9_-]/g, "-").slice(0, 16) || "musica";
  var name = $("nm").value || fileName;

  file.text()
    .then(function (text) {
      return api("/upload?id=" + id + "&name=" + encodeURIComponent(name), {
        method: "POST",
        headers: { "Content-Type": "text/plain" },
        body: text
      });
    })
    .then(function () {
      msg("Enviado: " + id);
      $("file").value = "";
      $("nm").value = "";
      return loadSongs();
    })
    .catch(onError);
}

/* ---------- Início ---------- */

$("stop").onclick = function () {
  api("/stop").then(render).catch(onError);
};
$("up").onclick = upload;

loadSongs();
setInterval(poll, 2000);
