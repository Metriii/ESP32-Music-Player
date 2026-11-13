# CLAUDE.md — ESP32 Music Player

Leia `AGENTS.md` antes de qualquer alteração: ele é a especificação completa do projeto.

## Resumo
ESP32 em modo Access Point (`ESP32-Music-Player` / `esp32music`, `192.168.4.1:80`) serve uma página web
leve que controla um buzzer ativo no GPIO 18 com padrões rítmicos (som/silêncio) de 4 músicas.

## Estrutura
- `platformio.ini` — env `esp32dev`, Arduino framework.
- `src/main.cpp` — firmware ESP32: rotas (`/`, `/play`, `/stop`, `/status`, `/songs`, `/upload`, `/delete`), músicas padrão (seed) e buzzer.
- `frontend/` — `index.html`, `style.css`, `app.js`. É o `data_dir` do PlatformIO: vai para o LittleFS com `pio run -t uploadfs`.
- `.claude/rules/` — regras de firmware e frontend.

## Comandos
- `pio run` — compilar
- `pio run -t upload` — enviar o firmware à ESP32
- `pio run -t uploadfs` — enviar o frontend (`frontend/`) ao LittleFS (apaga as músicas enviadas)
- `pio device monitor` — Serial (115200)

## Regras-chave
- Sem `delay()` na reprodução; usar `millis()`.
- Frontend sem frameworks/CDNs; servido do LittleFS (arquivos separados em `frontend/`).
- Sem bibliotecas externas; LittleFS guarda o frontend e as músicas (`/songs/<id>.txt`, upload pela página). Sem SPIFFS.
- Não afirmar teste de hardware sem acesso físico.
- Responder em português.
