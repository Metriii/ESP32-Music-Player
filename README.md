# ESP32 Music Player

A ESP32 cria sua própria rede Wi-Fi e hospeda uma página web para escolher e parar padrões rítmicos tocados em um buzzer ativo. Tudo roda na ESP32, sem internet ou servidor externo.

## Estrutura
```text
frontend/   index.html, style.css, app.js
src/        main.cpp (firmware ESP32)
platformio.ini
```
A pasta `frontend/` é gravada no LittleFS da ESP32 (`pio run -t uploadfs`) e servida pelo firmware; mudar a interface não exige recompilar. Para visualizar a interface sem a ESP32, abra `frontend/index.html` no navegador (sem a placa, a lista de músicas fica vazia).

## Componentes
- ESP32 DevKit
- Buzzer ativo
- (Opcional) transistor NPN + resistor de base ~1 kΩ, se o buzzer exigir mais corrente que o GPIO fornece (~12 mA recomendado)

## Ligações
| Buzzer | ESP32 |
|---|---|
| + (sinal) | GPIO 18 |
| − | GND |

Se o módulo for ativo em LOW, altere `BUZZER_ON` em `src/main.cpp`.

## PlatformIO
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 115200
```

## Compilar e enviar
```bash
pio run                # compilar o firmware
pio run -t upload      # enviar o firmware
pio run -t uploadfs    # enviar o frontend (pasta frontend/) ao LittleFS
pio device monitor   # Serial Monitor
```

> Rode os **dois** uploads na primeira vez. `uploadfs` regrava a partição de arquivos e **apaga as músicas enviadas por você** (as 4 padrão são recriadas ao ligar). Sem o `uploadfs`, a página mostra "Frontend ausente".

## Acesso
- SSID: `ESP32-Music-Player`
- Senha: `esp32music`
- URL: http://192.168.4.1 (porta 80)

## Rotas HTTP
| Método | Rota | Descrição |
|---|---|---|
| GET | `/` | Interface web |
| GET | `/play?id=mario` | Inicia música (400 sem `id`, 404 se inexistente) |
| GET | `/stop` | Para a reprodução |
| GET | `/status` | Estado atual |
| GET | `/songs` | Lista as músicas salvas |
| POST | `/upload?id=x&name=Nome` | Envia música (corpo `text/plain`) |
| GET | `/delete?id=x` | Apaga música |

Respostas: `{"playing":true,"id":"mario","name":"Super Mario"}` ou `{"playing":false}`.

Músicas padrão: `mario`, `harry`, `pirates`, `fur` (gravadas no LittleFS na primeira execução).

## Adicionar uma música (upload)
Na página, em **Enviar música**, escolha um arquivo `.txt` e clique em Enviar. O nome do arquivo (minúsculo, sem acentos, até 16 caracteres) vira o id. Enviar com o mesmo id substitui a música; o botão × apaga.

Formato do arquivo:
```text
# Minha Musica
120,60,120,120,240,240,240,480
```
- 1ª linha opcional com `# Nome`.
- Durações em ms separadas por vírgula, espaço ou linha: som, pausa, som, pausa...
- Quantidade **par**, cada valor entre 10 e 10000 ms, até 512 valores, arquivo até 4 KB, até 20 músicas.

Os arquivos ficam em `/songs/` no LittleFS e sobrevivem a reinicializações e a novos uploads do firmware (`upload`), mas são apagados por `uploadfs`.

## Limitações
O buzzer ativo tem frequência interna fixa: só é possível variar duração de sons e pausas, não notas. Os padrões são ritmos inspirados nas músicas, **não** reproduções fiéis. Compilar não prova que o buzzer funciona; é necessário testar no hardware.
