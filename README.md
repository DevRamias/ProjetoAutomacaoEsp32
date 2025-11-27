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
- SSID e senha salvos no **SPIFFS**, carregando tudo ao reiniciar.  

### 🌐 Web Server Integrado
- Página local para:
  - Controle do ventilador  
  - Status da conexão  
  - Configuração de Wi-Fi  
- Interface simples acessível via navegador, sem necessidade de aplicativos externos.

---

## 🧩 Ambiente de Desenvolvimento

Este projeto utiliza:

- **Placa:** ESP32 WROOM  
- **IDE:** Arduino IDE  
- **Sistema de Arquivos:** SPIFFS  
- **Bibliotecas principais:**
  - WiFi.h  
  - WebServer.h  
  - FS.h / SPIFFS.h  
  - Demais bibliotecas do ESP32 Arduino Core

> Uma documentação mais detalhada do ambiente será adicionada em breve, incluindo instruções, dependências e esquema de hardware.

---

## 📦 Estrutura do Projeto

Uma documentação mais aprofundada será adicionada futuramente na pasta `/docs`, contendo:

- Fluxo lógico do sistema Wi-Fi  
- Organização interna do código  
- Rotas disponíveis no Web Server  
- Esquema elétrico e conexões  

---

## 🛠️ Status do Projeto

O projeto está em desenvolvimento contínuo.  
As versões oficiais podem ser consultadas na aba **Releases**.

- **v1.0.0** — versão inicial simples  
- **v2.0.0** — reestruturação total para arquitetura orientada a objetos  
- Próximas versões já estão planejadas com novos recursos

---

## 📜 Licença

Distribuído sob a **MIT License**.  
Sinta-se à vontade para usar, modificar e distribuir, desde que mantenha os créditos.

---

## 👤 Autor

**Ramias Lopes**  
Criador e desenvolvedor deste projeto.

---
