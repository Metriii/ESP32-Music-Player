# Regras do firmware

- Nunca usar `delay()` na reprodução: usar `millis()` em `updatePlayer()` chamado no `loop()`.
- Buzzer ativo no GPIO 18; polaridade em `BUZZER_ON`. Não trocar o componente sem autorização.
- Padrões: índice par = som, ímpar = silêncio. Músicas em LittleFS (`/songs/<id>.txt`); validar id (`[a-z0-9_-]{1,16}`) e conteúdo em todo upload.
- `/play` valida `id`: 400 se ausente, 404 se inexistente. Respostas sempre JSON.
- Não afirmar que o hardware foi testado; compilar não prova o funcionamento do buzzer.
