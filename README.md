Este README ainda está em desenvolvimento. Mais detalhes serão adicionados em breve.

# 📡 Projeto de Automação com ESP32

Automação residencial simples usando ESP32, com controle de ventilador, sistema Wi-Fi inteligente com fallback para Access Point e interface web integrada para configuração e operação.  
O objetivo é criar um módulo autônomo capaz de conectar-se automaticamente à rede, permitir configuração via AP quando necessário e oferecer controle rápido de dispositivos conectados.

---

## ✨ Funcionalidades Principais

### 🔌 Controle de Dispositivos
- Liga e desliga o ventilador.  
- Modo temporizador simples (“timer”).  
- Base preparada para expansão com novos relés e funções futuras.  
- Modo Automático: aciona ou desliga o ventilador conforme temperatura e horário configurados.

### 🌡 Monitoramento de Temperatura
- Leitura da temperatura ambiente via sensor **DHT11**.  
- Informação exibida em tempo real na interface web.  
- Integrada ao Modo Automático para decisões de ligar/desligar.

### 🕒 Sincronização Automática de Horário (NTP)
- Obtém data e hora corretas via servidor NTP.  
- Mantém o funcionamento baseado em horários sempre preciso, sem necessidade de ajuste manual.

### 📶 Conexão Wi-Fi Inteligente e Configuração
- Conecta automaticamente à última rede salva.  
- Caso não consiga, ativa um **Access Point** próprio.  
- Interface web para configuração de novas redes Wi-Fi.  
- Busca redes disponíveis para facilitar a escolha.  
- SSID e senha salvos no **LittleFS**, carregando tudo ao reiniciar.  

### 🌐 Web Server Integrado
- Página local para:
  - Controle do ventilador  
  - Status da conexão  
  - Configuração de Wi-Fi  
- Interface simples acessível via navegador, sem necessidade de aplicativos externos.

### 📲 Atualização OTA (Over-The-Air)

- Atualização de firmware **sem cabo USB**, pela rede Wi-Fi (ArduinoOTA + mDNS em `esp32.local`).
- O OTA **sempre exige senha**. A primeira senha é `senha`; troque pelo painel web, no card **Configurações da Placa** (vale na hora, sem reiniciar).
- A senha fica salva no LittleFS (`/ota_config.json`) e **não** passa mais pelo portal do WiFiManager.
- Para gravar via OTA com o PlatformIO: `pio run -t upload --upload-port esp32.local`.

---

## 🧩 Ambiente de Desenvolvimento

- **Placa:** ESP32 WROOM (`board = esp32dev`)
- **IDE recomendada:** VS Code + extensão **PlatformIO**
- **Sistema de Arquivos:** **LittleFS** (arquivos da pasta `data/`)
- **Bibliotecas:** instaladas e atualizadas automaticamente pelo PlatformIO, declaradas em `lib_deps` no `platformio.ini`:
  - ArduinoJson
  - WiFiManager
  - NTPClient
  - DHT sensor library + Adafruit Unified Sensor

> Credenciais não ficam no código: a rede Wi-Fi é configurada pelo portal do WiFiManager e a senha do OTA pelo painel web (card **Configurações da Placa**). No PC, a senha usada para gravar fica no arquivo local `ota_senha.ini`, que está no `.gitignore`.

### 🚀 Como compilar e gravar

Abra a pasta do repositório (`ProjetoAutomacaoEsp32`) no VS Code com a extensão PlatformIO instalada e rode os comandos com o terminal **na raiz do repositório** — é onde fica o `platformio.ini` (o PlatformIO não procura o arquivo em pastas acima):

```bash
pio run                  # apenas compila (valida o codigo)
pio run -t upload        # compila e grava o firmware na ESP32
pio run -t uploadfs      # envia a pasta data/ (index.html) para o LittleFS
pio device monitor       # abre o monitor serial (115200)
pio pkg update           # atualiza plataforma e bibliotecas
pio pkg outdated         # mostra o que esta desatualizado
pio run -e esp32ota -t upload    # grava o firmware por Wi-Fi (OTA), sem cabo
pio run -e esp32ota -t uploadfs  # envia a pasta data/ para o LittleFS por Wi-Fi
```

> Na barra inferior do VS Code ficam os botões da extensão PlatformIO: ✓ compilar, → gravar e 🔌 monitor serial.

> **O Arduino IDE continua funcionando:** o projeto está no formato de sketch (`.ino`, `.cpp` e `.h` na mesma pasta). Basta abrir a pasta `ProjetoQuartoEsp32` no Arduino IDE — nesse caso, as bibliotecas listadas acima precisam ser instaladas pelo Gerenciador de Bibliotecas.

### 📡 Gravando sem cabo (OTA)

O env `esp32ota` já está configurado no `platformio.ini` (`upload_protocol = espota` + `upload_port = esp32.local`):

```bash
pio run -e esp32ota -t upload     # grava o firmware pelo Wi-Fi
pio run -e esp32ota -t uploadfs   # grava a pasta data/ (index.html) pelo Wi-Fi
```

**Firewall do Windows:** o `espota` precisa que a placa conecte de volta no PC. Sem uma regra de entrada liberada, a gravação falha com `No response from the ESP`. Rode uma vez, em um PowerShell **como Administrador** (ajuste o IP da placa se ela mudar):

```powershell
New-NetFirewallRule -DisplayName 'PlatformIO OTA (espota) - ESP32' -Direction Inbound -Action Allow -Program "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -RemoteAddress 192.168.1.65 -Profile Any
```

**Senha do OTA:** a placa sempre exige senha. Na primeira vez ela é `senha`. Para trocar:
1. No painel (`http://esp32.local`), card **Configurações da Placa**: informe a senha atual e a nova e clique em **Salvar senha**.
2. No PC, abra o arquivo `ota_senha.ini` (na raiz do projeto, ao lado do `platformio.ini`) e troque o valor de `password`. É dele que o PlatformIO tira a senha ao gravar pelo Wi-Fi (`upload_flags = --auth=...`). Se o arquivo não existir, o PlatformIO usa `senha`.

Se esquecer a senha: com o cabo USB, rode **Upload Filesystem Image** no ambiente `esp32dev` (`pio run -e esp32dev -t uploadfs`). Isso regrava o LittleFS (a senha volta a ser `senha` e o painel volta ao `data/index.html`; as configurações do modo automático voltam ao padrão). A rede Wi-Fi é mantida.

> Se a placa estiver com um firmware antigo compilado pelo **core 3.x** do ESP32 (autenticação SHA-256/PBKDF2), o `espota.py` que acompanha o pacote do PlatformIO usa MD5 e falha com `No response from the ESP` mesmo com o firewall liberado. Nesse caso grave uma vez por USB (`pio run -t upload`) ou use o `espota.py` atualizado do repositório oficial (`arduino-esp32/tools/espota.py`), que tenta SHA-256 e faz fallback para MD5.

---

## 📦 Estrutura do Projeto

```text
ProjetoAutomacaoEsp32/                 # abra ESTA pasta no VS Code
├── platformio.ini                     # placa, filesystem e bibliotecas (lib_deps)
├── README.md
└── ProjetoQuartoEsp32/                # pasta do sketch (PlatformIO e Arduino IDE)
    ├── ProjetoQuartoEsp32.ino         # sketch principal (setup/loop)
    ├── DHTManager.h / .cpp            # leitura do DHT11 com cache
    ├── NTPManager.h / .cpp            # data e hora via NTP
    ├── OTAManager.h / .cpp            # atualização OTA
    ├── RelayManager.h / .cpp          # relé: modo manual e automático
    ├── WebServerManager.h / .cpp      # servidor web e rotas HTTP
    └── data/index.html                # interface web enviada ao LittleFS
```

### 🌐 Rotas do Web Server

| Rota | Método | Função |
| --- | --- | --- |
| `/` | GET | Interface web (`index.html` do LittleFS) |
| `/start?duration=min` | GET | Liga o ventilador por N minutos |
| `/stop` | GET | Desliga o ventilador |
| `/status` | GET | Estado do relé |
| `/remaining` | GET | Tempo restante do timer manual |
| `/time` | GET | Hora atual (NTP) |
| `/sensor-data` | GET | Temperatura e umidade do DHT11 |
| `/system` | GET | Heap, uptime e modelo do chip |
| `/flash-info` | GET | Uso de flash e do LittleFS |
| `/wificonfig` | GET | Abre o portal de configuração Wi-Fi |
| `/get-auto-settings` | GET | Configurações do modo automático |
| `/set-auto-settings` | POST | Salva o modo automático |
| `/upload-html` | POST | Substitui o `index.html` do LittleFS |

---

## 🛠️ Status do Projeto

O projeto está em desenvolvimento contínuo.  
As versões oficiais podem ser consultadas na aba **Releases**.

- **v1.0.0** — versão inicial simples  
- **v2.0.0** — reestruturação total para arquitetura orientada a objetos  
- **v2.1.0** — PlatformIO, GitHub Actions, página de recuperação, configurações da placa, senha do OTA pelo painel e várias correções (sensação térmica, modo automático, painel)  

Detalhes de cada versão no [CHANGELOG.md](CHANGELOG.md).

---

## 📜 Licença

Distribuído sob a **MIT License**.  
Sinta-se à vontade para usar, modificar e distribuir, desde que mantenha os créditos.

---

## 👤 Autor

**Ramias Lopes**  
Criador e desenvolvedor deste projeto.

---
