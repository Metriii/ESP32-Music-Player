# Regras do frontend

- Somente HTML, CSS e JavaScript puro, em `frontend/` (`index.html`, `style.css`, `app.js`). São o `data_dir` do PlatformIO e vão ao LittleFS com `pio run -t uploadfs`.
- Sem frameworks, CDNs, fontes/ícones externos, imagens, blur/backdrop-filter ou animações pesadas.
- Tema escuro azul-marinho, detalhes em azul, fonte do sistema, layout centralizado e responsivo.
- Elementos obrigatórios: cabeçalho ESP32 PLAYER, indicador de conexão, lista de músicas, botão Tocar por música, música atual, botão Parar.
- `fetch()` nativo; `/status` a cada 2 s; falhas de rede não podem travar a página.
- A lista de músicas vem de `/songs`; nomes inseridos via `textContent` (nunca `innerHTML`).
