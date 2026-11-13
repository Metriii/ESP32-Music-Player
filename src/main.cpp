// ESP32 Music Player — buzzer ativo (GPIO 18) controlado por página web hospedada na ESP32.
// Frontend (frontend/) e músicas ficam no LittleFS; este firmware serve a página e a API.
// Músicas = padrões rítmicos (som/silêncio) salvos em LittleFS (/songs/<id>.txt).
// O usuário pode enviar novos arquivos pela página. Reprodução não bloqueante com millis().

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// ---------- Configuração ----------
static const char *AP_SSID = "ESP32-Music-Player";
static const char *AP_PASS = "esp32music";
static const uint8_t BUZZER_PIN = 18;
// Polaridade do módulo: HIGH liga o buzzer. Inverta se o módulo for ativo em LOW.
static const uint8_t BUZZER_ON = HIGH;
static const uint8_t BUZZER_OFF = (BUZZER_ON == HIGH) ? LOW : HIGH;

static const size_t MAX_STEPS = 512;       // intervalos por música
static const size_t MAX_FILE_BYTES = 4096; // tamanho máximo do arquivo
static const size_t MAX_SONGS = 20;
static const uint16_t MIN_MS = 10, MAX_MS = 10000;
static const size_t ID_MAX = 16, NAME_MAX_LEN = 32;
static const char *SONG_DIR = "/songs";

WebServer server(80);

// ---------- Músicas padrão (gravadas no LittleFS na primeira execução) ----------
// Durações em ms. Índice par = som ligado, índice ímpar = silêncio.
struct SeedSong {
  const char *id;
  const char *name;
  const uint16_t *pattern;
  uint8_t count;
};

static const uint16_t MARIO[] PROGMEM = {120, 60, 120, 120, 120, 120, 100, 60, 120, 120, 240, 240, 240, 480};
static const uint16_t HARRY[] PROGMEM = {200, 100, 300, 100, 150, 100, 250, 150, 200, 100, 400, 300};
static const uint16_t PIRATES[] PROGMEM = {150, 80, 150, 80, 150, 160, 150, 80, 300, 160, 150, 80, 150, 80, 300, 400};
static const uint16_t FUR[] PROGMEM = {100, 60, 100, 60, 100, 60, 100, 60, 100, 60, 200, 100, 100, 60, 100, 300};

#define SEED(id, name, arr) {id, name, arr, sizeof(arr) / sizeof(arr[0])}
static const SeedSong SEEDS[] = {
    SEED("mario", "Super Mario", MARIO),
    SEED("harry", "Harry Potter", HARRY),
    SEED("pirates", "Piratas do Caribe", PIRATES),
    SEED("fur", "Fur Elise", FUR),
};

// ---------- Armazenamento ----------
// id válido: 1 a 16 caracteres entre a-z, 0-9, '-' e '_' (vira o nome do arquivo).
static bool validId(const String &id) {
  if (id.length() == 0 || id.length() > ID_MAX) {
    return false;
  }
  for (size_t i = 0; i < id.length(); i++) {
    char c = id[i];
    bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
    if (!ok) {
      return false;
    }
  }
  return true;
}

static String songPath(const String &id) {
  return String(SONG_DIR) + "/" + id + ".txt";
}


// Remove caracteres que quebrariam o JSON/HTML e limita o tamanho.
static String cleanName(const String &raw) {
  String out;
  for (size_t i = 0; i < raw.length() && out.length() < NAME_MAX_LEN; i++) {
    char c = raw[i];
    bool printable = c >= 32 && c != 127;
    bool unsafe = c == '"' || c == '\\' || c == '<' || c == '>' || c == '&';
    if (printable && !unsafe) {
      out += c;
    }
  }
  out.trim();
  return out;
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isSeparator(char c) {
  return c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Formato: 1ª linha opcional "# Nome"; depois durações em ms separadas por vírgula/espaço/linha.
// Retorna false se inválido (quantidade ímpar, fora de faixa, vazio...).
static bool parseSong(const String &text, String &name, uint16_t *out, size_t &count) {
  count = 0;
  name = "";
  size_t i = 0;
  size_t len = text.length();

  // Linha de nome opcional
  if (len > 0 && text[0] == '#') {
    int lineEnd = text.indexOf('\n');
    size_t end = (lineEnd < 0) ? len : (size_t)lineEnd;
    name = cleanName(text.substring(1, end));
    i = end;
  }

  // Durações
  while (i < len) {
    char c = text[i];

    if (isSeparator(c)) {
      i++;
      continue;
    }
    if (!isDigit(c)) {
      return false;  // caractere inesperado
    }

    unsigned long value = 0;
    while (i < len && isDigit(text[i])) {
      value = value * 10 + (text[i] - '0');
      if (value > MAX_MS) {
        return false;
      }
      i++;
    }
    if (value < MIN_MS || count >= MAX_STEPS) {
      return false;
    }
    out[count++] = (uint16_t)value;
  }

  // Precisa alternar som/silêncio até o fim: quantidade par e não vazia.
  return count >= 2 && count % 2 == 0;
}

static bool readSongFile(const String &id, String &name, uint16_t *out, size_t &count) {
  File f = LittleFS.open(songPath(id), "r");
  if (!f || f.size() > MAX_FILE_BYTES) {
    return false;
  }
  String text = f.readString();
  f.close();

  if (!parseSong(text, name, out, count)) {
    return false;
  }
  if (name.length() == 0) {
    name = id;
  }
  return true;
}

static bool writeSongFile(const String &id, const String &name, const uint16_t *p, size_t n) {
  File f = LittleFS.open(songPath(id), "w");
  if (!f) {
    return false;
  }
  f.print("# ");
  f.println(name);
  for (size_t i = 0; i < n; i++) {
    f.print(p[i]);
    f.print(i + 1 < n ? "," : "\n");
  }
  f.close();
  return true;
}

// Grava as músicas padrão que ainda não existem no LittleFS.
static void seedSongs() {
  if (!LittleFS.exists(SONG_DIR)) {
    LittleFS.mkdir(SONG_DIR);
  }
  for (const SeedSong &s : SEEDS) {
    if (LittleFS.exists(songPath(s.id))) {
      continue;
    }
    uint16_t buf[32];
    size_t n = s.count < 32 ? s.count : 32;
    for (size_t i = 0; i < n; i++) buf[i] = pgm_read_word(&s.pattern[i]);
    writeSongFile(s.id, s.name, buf, n);
  }
}

static size_t countSongs() {
  size_t n = 0;
  File dir = LittleFS.open(SONG_DIR);
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    n++;
  }
  return n;
}

// ---------- Estado da reprodução ----------
static uint16_t pattern[MAX_STEPS];
static size_t patternCount = 0;
static String currentId, currentName;
static bool playing = false;
static bool buzzerOn = false;
static size_t stepIndex = 0;
static unsigned long stepStart = 0;

static void setBuzzer(bool on) {
  buzzerOn = on;
  digitalWrite(BUZZER_PIN, on ? BUZZER_ON : BUZZER_OFF);
}

// Carrega a música do arquivo e começa a tocar imediatamente (interrompe a anterior).
static bool startSong(const String &id) {
  String name;
  uint16_t tmp[MAX_STEPS];
  size_t n;
  if (!readSongFile(id, name, tmp, n)) {
    return false;
  }
  memcpy(pattern, tmp, n * sizeof(uint16_t));
  patternCount = n;
  currentId = id;
  currentName = name;
  playing = true;
  stepIndex = 0;
  stepStart = millis();
  setBuzzer(true);
  return true;
}

static void stopSong() {
  playing = false;
  setBuzzer(false);
}

// Chamada a cada volta do loop(): troca som/silêncio quando o intervalo atual termina.
static void updatePlayer() {
  if (!playing) {
    return;
  }
  if (millis() - stepStart < pattern[stepIndex]) {
    return;
  }
  stepIndex = (stepIndex + 1) % patternCount;  // reinicia em loop
  stepStart = millis();
  setBuzzer(stepIndex % 2 == 0);
}

// ---------- Frontend (LittleFS) ----------
// Os arquivos de frontend/ são gravados na placa com `pio run -t uploadfs`
// e servidos a partir da raiz do LittleFS (/index.html, /style.css, /app.js).

// ---------- Rotas HTTP ----------
static void sendJson(int code, const String &body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

static void sendError(int code, const char *msg) {
  sendJson(code, String("{\"error\":\"") + msg + "\"}");
}

static String statusJson() {
  if (!playing) {
    return F("{\"playing\":false}");
  }
  return String("{\"playing\":true,\"id\":\"") + currentId + "\",\"name\":\"" + currentName + "\"}";
}

// Lê e valida ?id=. Em caso de erro já responde 400 e devolve false.
static bool readIdArg(String &id) {
  if (!server.hasArg("id") || server.arg("id").length() == 0) {
    sendError(400, "id ausente");
    return false;
  }
  id = server.arg("id");
  return true;
}

static bool songExists(const String &id) {
  return validId(id) && LittleFS.exists(songPath(id));
}

// Envia um arquivo do LittleFS (frontend).
static void sendFile(const char *path, const char *mime) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    server.send(404, "text/plain", "Frontend ausente. Rode: pio run -t uploadfs");
    return;
  }
  server.streamFile(f, mime);
  f.close();
}

static void handleRoot() {
  sendFile("/index.html", "text/html");
}

static void handleStyle() {
  sendFile("/style.css", "text/css");
}

static void handleScript() {
  sendFile("/app.js", "application/javascript");
}

static void handlePlay() {
  String id;
  if (!readIdArg(id)) {
    return;
  }
  if (!songExists(id)) {
    return sendError(404, "musica nao encontrada");
  }
  if (!startSong(id)) {
    return sendError(500, "arquivo da musica invalido");
  }
  sendJson(200, statusJson());
}

static void handleStop() {
  stopSong();
  sendJson(200, statusJson());
}

static void handleStatus() {
  sendJson(200, statusJson());
}

// Nome do arquivo sem a pasta: "/songs/mario.txt" -> "mario.txt"
static String baseName(const String &path) {
  int slash = path.lastIndexOf('/');
  return (slash >= 0) ? path.substring(slash + 1) : path;
}

static void handleSongs() {
  String json = "[";
  bool first = true;

  File dir = LittleFS.open(SONG_DIR);
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    String fileName = baseName(f.name());
    if (!fileName.endsWith(".txt")) {
      continue;
    }
    String id = fileName.substring(0, fileName.length() - 4);
    if (!validId(id)) {
      continue;
    }

    // Nome = 1ª linha "# Nome"; se não houver, usa o id.
    String name = id;
    if (f.peek() == '#') {
      String title = cleanName(f.readStringUntil('\n').substring(1));
      if (title.length() > 0) {
        name = title;
      }
    }

    if (!first) {
      json += ",";
    }
    first = false;
    json += "{\"id\":\"" + id + "\",\"name\":\"" + name + "\"}";
  }

  json += "]";
  sendJson(200, json);
}

static void handleUpload() {
  String id;
  if (!readIdArg(id)) {
    return;
  }
  if (!validId(id)) {
    return sendError(400, "id invalido");
  }
  if (!server.hasArg("plain")) {
    return sendError(400, "corpo ausente");
  }
  String body = server.arg("plain");
  if (body.length() > MAX_FILE_BYTES) {
    return sendError(413, "arquivo grande demais");
  }

  // Valida o conteúdo antes de gravar qualquer coisa.
  String titleInFile;
  uint16_t buf[MAX_STEPS];
  size_t n;
  if (!parseSong(body, titleInFile, buf, n)) {
    return sendError(400, "formato invalido (par, 10-10000 ms)");
  }

  // Nome: ?name= > "# Nome" do arquivo > id
  String name = server.hasArg("name") ? cleanName(server.arg("name")) : "";
  if (name.length() == 0) {
    name = (titleInFile.length() > 0) ? titleInFile : id;
  }

  if (!LittleFS.exists(songPath(id)) && countSongs() >= MAX_SONGS) {
    return sendError(409, "limite de musicas atingido");
  }
  if (playing && currentId == id) {
    stopSong();  // vai ser sobrescrita
  }
  if (!writeSongFile(id, name, buf, n)) {
    return sendError(500, "falha ao gravar");
  }
  sendJson(200, String("{\"id\":\"") + id + "\",\"name\":\"" + name + "\"}");
}

static void handleDelete() {
  String id;
  if (!readIdArg(id)) {
    return;
  }
  if (!songExists(id)) {
    return sendError(404, "musica nao encontrada");
  }
  if (playing && currentId == id) {
    stopSong();
  }
  LittleFS.remove(songPath(id));
  sendJson(200, F("{\"deleted\":true}"));
}

static void handleNotFound() {
  sendError(404, "rota nao encontrada");
}

// ---------- Inicialização ----------
static void setupBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  setBuzzer(false);
}

static void setupStorage() {
  if (!LittleFS.begin(true)) {  // true: formata se ainda não existir
    Serial.println("Falha ao montar LittleFS");
    return;
  }
  seedSongs();
}

static void setupWifi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP: ");
  Serial.print(AP_SSID);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());  // 192.168.4.1
}

static void setupServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/app.js", HTTP_GET, handleScript);
  server.on("/play", HTTP_GET, handlePlay);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/songs", HTTP_GET, handleSongs);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/delete", HTTP_GET, handleDelete);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  setupBuzzer();
  setupStorage();
  setupWifi();
  setupServer();
}

void loop() {
  server.handleClient();
  updatePlayer();
}
