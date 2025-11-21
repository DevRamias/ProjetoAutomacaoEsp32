Este README ainda está em desenvolvimento. Mais detalhes serão adicionados em breve.

### Release v2.0.0 — Grande atualização do sistema de automação do quarto

Esta versão 2.0.0 marca a primeira grande evolução do projeto.
Inclui uma interface web mais completa e funcional, integração com sensores de temperatura e umidade, modo automático totalmente configurável, melhorias internas na estrutura do código e facilidades de manutenção como atualização via rede tanto para o firmware quanto para a interface HTML.

### Novas funcionalidades visuais

- Exibição de horário sincronizado com servidor NTP 

-  Painel de ambiente com:
   - Temperatura
   - Umidade (%)
   - Sensação térmica
- Barra de status indicando:
   - Tempo ligado
   - Tempo desligado (standby)

### Funcionalidades de controle:

**Modo manual**
- Campo para definir tempo ligado (horas e minutos)
- Botão Ligar (baseado no tempo escolhido)
- Botão Desligar
- Botão Wi-Fi para ativar o Access Point e trocar a rede conectada

**Modo Automático**

**Toggle para ativar/desativar com opções:**
- Ligar acima de temperatura definida (°C)
- Tempo que o ventilador permanecerá ligado
- Tempo em standby (desligado)
- Definição de hora de início e hora de fim para repetir ciclos somente dentro da janela configurada

**Informações do sistema (Memoria RAM):**

- Memória livre (bytes)
- Maior bloco disponível (bytes)
- Fragmentação (%)
- Tempo de atividade do ESP32 (horas e minutos)

**Informações de Armazenamento:**

- **Memoria Flash:**
 -Total em MB
 -Usado em MB
 -Livre em MB

- **Sistema de Arquivos (Memória reservada do esp32)**
 -Total em MB
 -Usado em MB
 -Libre em MB

**Funcionalidade de manutenção:**

**- Upload de nova interface HTML:**

O ESP32 vem inicialmente com uma página HTML mínima apenas para permitir o primeiro upload.
A interface recomendada encontra-se em /data/index.html.

**- Atualização de firmware via OTA:**

Disponível quando o ESP32 está na mesma rede local.
Certifique-se de que a biblioteca necessária está instalada.
Host e senha OTA estão no código principal e podem (ou devem) ser alterados se desejar usar essa função.


Considerações finais:

- Se o ESP32 não encontrar uma rede salva, ele sobe um AP chamado "ESP32-Config". Conecte-se e acesse 192.168.4.1.
- Verifique pinos de entrada e saída no código .INO
- Ao modificar a página HTML, revise:
  - As rotas na classe **WebServerManager**
  - O script do arquivo index.html, que depende dessas rotas para funcionar corretamente.
