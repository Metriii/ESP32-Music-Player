# AGENTS.md — ESP32 Music Player

## 1. Contexto do projeto

O ESP32 Music Player é um projeto simples que transforma uma ESP32 em um pequeno servidor web para controlar a reprodução de músicas por meio de um buzzer ativo.

A ESP32 hospedará sua própria página web. O usuário poderá acessar a interface pelo celular ou computador, selecionar uma música e controlar sua reprodução.

Toda a aplicação deverá funcionar diretamente na ESP32, sem servidores externos.

## 2. Tecnologias obrigatórias

- C++.
- PlatformIO.
- Arduino Framework.
- ESP32 DevKit.
- WiFi.h.
- WebServer.h.
- HTML5.
- CSS3.
- JavaScript puro.

Não utilizar React, Angular, Vue, Node.js ou frameworks frontend.

Não adicionar bibliotecas externas sem necessidade técnica justificada.

## 3. Hardware

O projeto utiliza:

- ESP32 DevKit.
- Buzzer ativo.
- GPIO 18 para acionamento do buzzer.
- GND para aterramento.

O buzzer é ativo e possui frequência interna. Portanto, não é possível reproduzir melodias com notas musicais variadas como em um buzzer passivo.

A primeira versão deverá trabalhar com padrões rítmicos de sons e pausas.

Não substituir o buzzer ativo por outro componente sem autorização.

Verificar a necessidade de transistor conforme a corrente exigida pelo buzzer.

## 4. Arquitetura

A ESP32 será responsável por todas as funcionalidades:

1. Criar uma rede Wi-Fi própria.
2. Hospedar o servidor HTTP.
3. Disponibilizar a interface web.
4. Receber os comandos enviados pelo frontend.
5. Identificar a música selecionada.
6. Controlar o buzzer.
7. Gerenciar o estado da reprodução.

### Configuração Wi-Fi

- SSID: `ESP32-Music-Player`
- Senha: `esp32music`
- IP: `192.168.4.1`
- Porta: `80`

Utilizar Wi-Fi Access Point para permitir acesso direto à ESP32, sem depender de roteador ou internet.

## 5. Frontend — prioridade para leveza

Esta é uma das principais regras do projeto.

A ESP32 possui recursos limitados. Portanto, o frontend deverá ser extremamente leve.

### Tecnologias

Utilizar exclusivamente:

- HTML.
- CSS.
- JavaScript puro.

### Restrições

Não utilizar:

- React, Angular ou Vue.
- Bootstrap ou Tailwind.
- jQuery.
- Bibliotecas de animação.
- Font Awesome ou bibliotecas de ícones.
- Google Fonts.
- Imagens externas.
- Scripts externos.
- CDNs.
- Animações pesadas.
- Efeitos de blur ou backdrop-filter.
- Componentes visuais desnecessários.

### Design

Criar uma interface minimalista e responsiva.

Características:

- Tema escuro.
- Fundo em azul-marinho ou cinza escuro.
- Detalhes em azul.
- Cards simples.
- Botões intuitivos.
- Tipografia padrão do sistema.
- Layout centralizado.
- Boa visualização em celulares.

### Elementos obrigatórios

1. Cabeçalho com o nome ESP32 PLAYER.
2. Indicador de conexão.
3. Lista de músicas.
4. Botão de reprodução para cada música.
5. Identificação da música atual.
6. Botão para parar a reprodução.

Não implementar recursos visuais sem utilidade funcional.

### Armazenamento do frontend

O frontend fica em arquivos separados na pasta `frontend/` (`index.html`, `style.css`, `app.js`) e é gravado no LittleFS da ESP32 com `pio run -t uploadfs` (`data_dir = frontend` no `platformio.ini`). O firmware serve esses arquivos com `server.streamFile()` nas rotas `/`, `/style.css` e `/app.js`.

O mesmo LittleFS (nativo do framework) guarda as músicas em `/songs/`. Não utilizar SPIFFS.

Atenção: `uploadfs` regrava a partição inteira e apaga as músicas enviadas pelo usuário; as 4 músicas padrão são recriadas na próxima inicialização.

O frontend deverá funcionar completamente offline.

## 6. Biblioteca musical

Criar inicialmente quatro opções:

| ID | Música |
|---|---|
| mario | Super Mario |
| harry | Harry Potter |
| pirates | Piratas do Caribe |
| fur | Fur Elise |

As músicas ficam em `/songs/<id>.txt` no LittleFS: 1ª linha opcional `# Nome`, depois durações em ms (som,pausa,som,pausa...), quantidade par, 10–10000 ms, até 512 intervalos e 4 KB por arquivo, até 20 músicas. As quatro músicas padrão são gravadas no LittleFS na primeira execução. O usuário pode enviar e apagar músicas pela página.

Cada música deverá possuir:

- Identificador único.
- Nome.
- Sequência de durações.
- Quantidade de intervalos.

Organizar os dados de maneira que novas músicas possam ser adicionadas facilmente.

Os padrões deverão alternar entre som ligado e silêncio.

Não apresentar os padrões sonoros como reprodução fiel das trilhas originais.

## 7. Controle do buzzer

Utilizar GPIO 18.

O acionamento deverá ser feito por nível lógico, considerando a polaridade do módulo.

- HIGH: ligado, quando compatível com o módulo.
- LOW: desligado, quando compatível com o módulo.

Implementar uma função responsável por iniciar a reprodução e outra por interrompê-la.

### Reprodução não bloqueante

Requisito obrigatório: não utilizar `delay()` para controlar as músicas.

Utilizar:

- `millis()`.
- Índice do intervalo atual.
- Tempo de início do intervalo.
- Estado do buzzer.
- Estado da reprodução.

O controle deverá ocorrer continuamente no `loop()`.

A ESP32 deverá continuar atendendo às requisições HTTP durante a reprodução.

### Comportamentos esperados

- Selecionar uma nova música interrompe a anterior.
- A nova música começa imediatamente.
- O botão parar desliga o buzzer.
- Ao terminar a sequência, a música reinicia em loop.
- O estado do player deve permanecer sincronizado com a interface.

## 8. API HTTP

Implementar as seguintes rotas:

| Método | Endpoint | Responsabilidade |
|---|---|---|
| GET | `/` | Servir o frontend |
| GET | `/play?id=mario` | Iniciar uma música |
| GET | `/stop` | Interromper a música |
| GET | `/status` | Consultar estado atual |
| GET | `/songs` | Listar músicas (`[{"id","name"}]`) |
| POST | `/upload?id=x&name=Nome` | Enviar música (corpo text/plain; 400 formato inválido, 413 grande demais, 409 limite) |
| GET | `/delete?id=x` | Apagar música |

### Resposta de reprodução

```json
{
  "playing": true,
  "id": "mario",
  "name": "Super Mario"
}
```

### Resposta de interrupção

```json
{
  "playing": false
}
```

### Regras

- Validar os parâmetros recebidos.
- Retornar HTTP 400 quando o ID estiver ausente.
- Retornar HTTP 404 para músicas inexistentes.
- Retornar JSON válido.
- Evitar respostas desnecessariamente grandes.
- Não confiar em parâmetros enviados pelo frontend sem validação.

## 9. Comunicação frontend e firmware

O JavaScript deverá utilizar `fetch()` nativo.

Ao selecionar uma música:

1. Enviar GET para `/play?id=IDENTIFICADOR`.
2. Verificar a resposta HTTP.
3. Atualizar a música atual.
4. Destacar o card selecionado.

Ao interromper:

1. Enviar GET para `/stop`.
2. Atualizar a interface.
3. Remover o destaque da música.

Consultar `/status` a cada 2 segundos para sincronizar o estado.

Tratar falhas de conexão sem travar a página.

## 10. Organização do código

Manter uma estrutura simples:

```text
ESP32-Music-Player/
│
├── AGENTS.md
├── CLAUDE.md
├── README.md
├── platformio.ini
│
├── backend/            (ferramenta opcional no PC, Python)
│   ├── music.py
│   └── musicas/        (arquivos .txt de exemplo)
│
├── frontend/           (gravado no LittleFS com `pio run -t uploadfs`)
│   ├── index.html
│   ├── style.css
│   └── app.js
│
└── src/
    └── main.cpp        (firmware ESP32: Wi-Fi, rotas HTTP, músicas, buzzer)
```

`backend/music.py` é uma CLI Python (só biblioteca padrão) que roda no PC, conectado ao Wi-Fi da ESP32, e usa as rotas `/songs`, `/upload`, `/delete`, `/play` e `/stop`. Serve só para gerenciar músicas; a ESP32 funciona sem ele. Não faz parte do firmware.

O frontend é editado em `frontend/` (HTML, CSS e JS separados) e enviado à placa com `pio run -t uploadfs`; mudar a interface não exige recompilar o firmware.

Separar as responsabilidades por funções, evitando colocar toda a lógica dentro de `setup()` e `loop()`.

Não criar uma arquitetura excessivamente complexa para este projeto.

## 11. Regras de desenvolvimento

1. Inspecionar os arquivos existentes antes de qualquer alteração.
2. Preservar configurações úteis.
3. Não apagar arquivos sem necessidade.
4. Não adicionar dependências desnecessárias.
5. Não modificar o hardware sem autorização.
6. Não introduzir frameworks frontend.
7. Priorizar legibilidade e simplicidade.
8. Evitar consumo desnecessário de RAM.
9. Armazenar conteúdo estático na flash sempre que possível.
10. Não utilizar operações bloqueantes durante a reprodução.
11. Não afirmar que o hardware foi testado sem acesso físico.
12. Não considerar a compilação como prova de funcionamento do buzzer.

Antes de alterar qualquer configuração, verificar a compatibilidade com a placa ESP32 utilizada.

## 12. Configuração PlatformIO

Utilizar como configuração inicial:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200
upload_speed = 115200
```

Utilizar bibliotecas nativas do framework sempre que possível.

## 13. Testes obrigatórios

Após a implementação, verificar:

- [ ] Projeto compila corretamente.
- [ ] Firmware pode ser enviado à ESP32.
- [ ] Rede Wi-Fi é criada.
- [ ] IP correto é apresentado no Serial Monitor.
- [ ] Página web abre no navegador.
- [ ] Interface funciona em celular.
- [ ] As quatro músicas aparecem.
- [ ] Cada música envia o identificador correto.
- [ ] Buzzer emite os padrões sonoros.
- [ ] É possível trocar de música durante a reprodução.
- [ ] Botão parar interrompe o som.
- [ ] Endpoint `/status` retorna informações corretas.
- [ ] Servidor continua respondendo durante a reprodução.
- [ ] Não existem dependências externas para carregar a página.

## 14. Documentação

Criar ou atualizar o README.md com:

- Descrição do projeto.
- Componentes utilizados.
- Ligações elétricas.
- Configuração do PlatformIO.
- Como compilar.
- Como realizar upload.
- Configuração da rede Wi-Fi.
- Endereço de acesso.
- Rotas HTTP.
- Como adicionar novas músicas.
- Limitações do buzzer ativo.

## 15. Objetivo final

Entregar um ESP32 Music Player funcional, leve e simples.

A aplicação deverá permitir que qualquer dispositivo conectado à rede Wi-Fi da ESP32 acesse uma página web e controle o buzzer.

A prioridade máxima é:

1. Funcionamento correto.
2. Baixo consumo de memória.
3. Interface leve.
4. Comunicação confiável.
5. Facilidade de manutenção e expansão.

Não complicar o projeto com tecnologias que não agreguem valor real à solução.