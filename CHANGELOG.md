# Histórico de versões

Este projeto segue o [Versionamento Semântico](https://semver.org/lang/pt-BR/) (`MAIOR.MENOR.CORREÇÃO`).

## [v2.1.0] — 2026-09-25

### ⚠️ Atenção ao atualizar
- **Senha do OTA:** deixou de ser fixa no código (`Teste123` na v2.0.0). Agora o OTA **sempre** exige senha, que começa como `senha` e é trocada pelo painel (card **Configurações da Placa**). No PC, a senha usada para gravar fica no arquivo local `ota_senha.ini` (fora do Git).
- O portal Wi-Fi (`ESP32-Config`) agora serve **só** para escolher a rede; ele não pede mais a senha do OTA.
- Depois de gravar o firmware, envie também o novo `data/index.html` (pela página `/recovery`).

### Adicionado
- **Suporte ao PlatformIO** (`platformio.ini`) com instalação automática das bibliotecas e ambientes `esp32dev` (cabo USB) e `esp32ota` (Wi-Fi).
- **GitHub Actions**: confere a compilação a cada push e anexa `firmware.bin` e `littlefs.bin` em cada Release.
- **Versão do firmware** embutida na compilação e exibida no painel.
- **Página de recuperação** embutida no firmware (`/recovery`): funciona mesmo com o `index.html` salvo quebrado; permite reenviar o HTML ou apagar a página salva.
- Rota **`/reset-html`** para apagar o `index.html` salvo direto pelo navegador.
- Card **Configurações da Placa**: rede, IP, sinal Wi-Fi, versão do firmware e troca da senha do OTA (exige a senha atual e vale na hora, sem reiniciar).
- Rotas **`/set-ota-password`** e **`/device-info`**.
- Botão **Salvar configurações** no modo automático, com aviso de alterações não salvas.
- Status mostra quando o modo automático está **fora do horário**.
- `.gitignore` do projeto.

### Alterado
- **Timer manual tem prioridade:** ligar o ventilador manualmente com o modo automático habilitado agora funciona; o automático volta sozinho quando o timer termina.
- Ligar o modo automático **cancela o timer manual** que estiver rodando.
- A contagem regressiva do painel passa a seguir o que a placa informa (aparece também quando o ventilador é ligado por outro aparelho).
- O ventilador é desligado antes de abrir o portal Wi-Fi.
- Uso de `JsonDocument` (ArduinoJson 7) no lugar de `DynamicJsonDocument`.

### Corrigido
- **Sensação térmica**: substituída a fórmula sem base física pelo índice de calor do NWS/NOAA; no frio com umidade alta a primeira versão da fórmula dava valores absurdos (ex.: 10 °C / 90 % → 27,8 °C; agora 9,4 °C).
- **Horário "5:00" sem zero à esquerda** fazia o modo automático ficar ativo quase o dia todo (comparação de texto). Horários agora são normalizados, inclusive os já salvos.
- **Leitura do DHT**: removido o laço de "3 leituras" que não respeitava o intervalo do sensor; cache próprio para a umidade; DHT inicializado antes do uso.
- Contagem regressiva **nunca aparecia** (bloco duplicado com IDs repetidos e escondido).
- Status do painel **travava** depois de clicar em Ligar/Desligar.
- Standby aparecia como **"undefined min"**.
- Alterar temperatura, tempos ou horários do modo automático **não salvava**.
- Formulário do modo automático mostrava valores diferentes dos salvos na placa.
- **1h 0min virava 1h 5min** no timer manual; aviso para tempos acima de 24 h.
- Unidades °C e % apareciam e sumiam; 0 °C era tratado como "sem sinal".
- A contagem sumia ao recarregar a página.
- `/remaining` podia devolver um número gigante (conta negativa em `unsigned`).
- A senha do OTA digitada no portal aberto pelo botão Wi-Fi era ignorada, e o campo vazio apagava a senha salva.
- Mensagem do portal Wi-Fi prometia recarregar a página, o que nunca funcionava (a placa sai da rede).
- Estrutura HTML: `<div>` sem fechar e botão de upload fora do container.

## [v2.0.0] — 2025-11-20
- Reestruturação total para arquitetura orientada a objetos (managers de relé, DHT, NTP, OTA e servidor web).

## [1.0.0]
- Versão inicial simples.
