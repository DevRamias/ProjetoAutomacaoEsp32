#!/usr/bin/env python3
# =============================================================================
#  Testes de regressao do ProjetoQuartoEsp32
# -----------------------------------------------------------------------------
#  Garante que mudancas no firmware NAO quebrem o que ja funciona:
#    - gravacao pelo Wi-Fi (OTA) com senha;
#    - portal Wi-Fi (ESP32-Config) aberto pela rota /wificonfig;
#    - mDNS (esp32.local) e todas as rotas do painel;
#    - pagina de recuperacao (/recovery).
#
#  So usa a biblioteca padrao do Python (nada para instalar).
#
#  MODOS
#    python testes/teste_regressao.py estatico
#        Le o codigo-fonte e confere que nada importante sumiu.
#        Nao precisa da placa. Roda tambem no GitHub Actions.
#
#    python testes/teste_regressao.py placa [--host esp32.local] [--firmware v2.6.0]
#        Testa a placa ligada na rede. SO FAZ LEITURAS: nunca liga/desliga o
#        ventilador, nao envia HTML, nao apaga nada e nao abre o portal.
#        Confere: rotas respondendo, JSON com os campos certos, hora do NTP
#        certa, tempo de resposta e o servico de OTA anunciado no mDNS.
#
#    python testes/teste_regressao.py placa --testar-upload
#        Envia o data/index.html do repositorio pelo /upload-html e confere que
#        a placa devolve a pagina identica (troca a pagina da placa!).
#
#    python testes/teste_regressao.py placa --testar-portal
#        ATENCAO: abre o portal Wi-Fi de verdade. O ventilador e DESLIGADO e a
#        placa sai da rede por ate 3 minutos (volta sozinha). O teste confere
#        que a rede "ESP32-Config" aparece (Windows, via netsh) e que a placa
#        volta para a rede depois. Use so quando quiser validar o portal.
#
#  Saida: codigo 0 = tudo certo; 1 = alguma verificacao falhou.
# =============================================================================
import argparse
import json
import re
import socket
import statistics
import struct
import subprocess
import sys
import time
import urllib.request
from datetime import datetime, timedelta, timezone
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
SRC = RAIZ / "ProjetoQuartoEsp32"

falhas = []
avisos = []


def ok(msg):
    print(f"  [OK]    {msg}")


def falha(msg):
    print(f"  [FALHA] {msg}")
    falhas.append(msg)


def aviso(msg):
    print(f"  [AVISO] {msg}")
    avisos.append(msg)


def checar(condicao, msg_ok, msg_falha=None):
    if condicao:
        ok(msg_ok)
    else:
        falha(msg_falha or msg_ok)
    return condicao


def ler(nome):
    return (SRC / nome).read_text(encoding="utf-8", errors="replace")


def sem_comentarios(codigo):
    """Remove comentarios C/C++ para nao achar coisas que so estao comentadas."""
    codigo = re.sub(r"/\*.*?\*/", "", codigo, flags=re.S)
    return re.sub(r"//[^\n]*", "", codigo)


def corpo_funcao(codigo, assinatura):
    """Devolve o corpo { ... } da primeira funcao cuja assinatura casa com o regex."""
    m = re.search(assinatura + r"\s*\{", codigo)
    if not m:
        return None
    i = m.end()
    nivel = 1
    while i < len(codigo) and nivel:
        if codigo[i] == "{":
            nivel += 1
        elif codigo[i] == "}":
            nivel -= 1
        i += 1
    return codigo[m.end():i - 1]


# Rotas que o painel, a recuperacao e o app Android usam. Se alguma sumir, quebra.
ROTAS = {
    "/": "HTTP_GET",
    "/start": "HTTP_GET",
    "/stop": "HTTP_GET",
    "/time": "HTTP_GET",
    "/status": "HTTP_GET",
    "/wificonfig": "HTTP_GET",
    "/remaining": "HTTP_GET",
    "/system": "HTTP_GET",
    "/flash-info": "HTTP_GET",
    "/sensor-data": "HTTP_GET",
    "/upload-html": "HTTP_POST",
    "/recovery": "HTTP_GET",
    "/reset-html": "HTTP_ANY",
    "/set-ota-password": "HTTP_POST",
    "/device-info": "HTTP_GET",
    "/set-auto-settings": "HTTP_POST",
    "/get-auto-settings": "HTTP_GET",
    "/state": "HTTP_GET",
}


# =============================================================================
#  MODO ESTATICO
# =============================================================================
def teste_estatico():
    ino = sem_comentarios(ler("ProjetoQuartoEsp32.ino"))
    web = sem_comentarios(ler("WebServerManager.cpp"))
    ota = sem_comentarios(ler("OTAManager.cpp"))
    ini = (RAIZ / "platformio.ini").read_text(encoding="utf-8")

    print("\n== OTA (gravacao pelo Wi-Fi) ==")
    loop = corpo_funcao(ino, r"void\s+loop\s*\(\s*\)") or ""
    setup = corpo_funcao(ino, r"void\s+setup\s*\(\s*\)") or ""
    checar("otaManager.handle()" in loop, "loop() chama otaManager.handle()",
           "loop() NAO chama otaManager.handle(): o OTA para de responder")
    checar(re.search(r"otaManager\.begin\(", setup), "setup() inicia o OTA (otaManager.begin)")
    checar("ArduinoOTA.handle()" in ota, "OTAManager::handle() chama ArduinoOTA.handle()")
    checar("ArduinoOTA.begin()" in ota, "OTAManager chama ArduinoOTA.begin()")
    checar("ArduinoOTA.setPassword(" in ota, "OTA sempre configura senha (ArduinoOTA.setPassword)")
    checar('"/ota_config.json"' in ota, "senha do OTA continua em /ota_config.json (nao se perde ao atualizar firmware)")
    checar('DEFAULT_PASSWORD = "senha"' in ota, "senha padrao do OTA continua \"senha\"")
    checar(re.search(r"ArduinoOTA\.setHostname\(", ota), "OTA usa hostname (esp32.local)")

    print("\n== platformio.ini (OTA pelo PlatformIO) ==")
    checar(re.search(r"\[env:esp32ota\]", ini), "env esp32ota existe")
    checar(re.search(r"upload_protocol\s*=\s*espota", ini), "esp32ota usa upload_protocol = espota")
    checar(re.search(r"upload_port\s*=\s*esp32\.local", ini), "esp32ota grava em esp32.local")
    checar(re.search(r"upload_flags\s*=\s*--auth=\$\{ota\.password\}", ini), "esp32ota envia a senha (--auth)")
    checar(re.search(r"extra_configs\s*=\s*ota_senha\.ini", ini), "senha local vem de ota_senha.ini")
    checar(re.search(r"board_build\.filesystem\s*=\s*littlefs", ini), "sistema de arquivos continua LittleFS")
    # Trocar a tabela de particoes NAO funciona por OTA (exige cabo). Avisa se aparecer.
    checar(not re.search(r"^\s*board_build\.partitions", ini, flags=re.M),
           "tabela de particoes nao foi alterada (mudanca de particao exige cabo USB)",
           "board_build.partitions apareceu: mudar particoes NAO funciona via OTA")

    print("\n== mDNS (esp32.local) ==")
    checar(re.search(r"MDNS\.begin\(", setup), "setup() inicia o mDNS")
    checar(re.search(r'MDNS\.addService\(\s*"http"\s*,\s*"tcp"\s*,\s*80\s*\)', setup),
           "mDNS anuncia _http._tcp porta 80 (o app Android procura por ele)")
    checar(re.search(r'otaHost\s*=\s*"esp32"', setup), "hostname continua \"esp32\"")

    print("\n== Portal Wi-Fi (ESP32-Config) ==")
    checar(re.search(r'autoConnect\(\s*"ESP32-Config"\s*\)', setup), "setup() usa autoConnect(\"ESP32-Config\")")
    checar(re.search(r"setConfigPortalTimeout\(\s*180\s*\)", setup), "portal fecha sozinho em 180 s")
    wificfg = corpo_funcao(web, r"void\s+WebServerManager::handleWiFiConfig\s*\(\s*\)") or ""
    checar("shouldStartPortal = true" in wificfg, "/wificonfig agenda o portal (shouldStartPortal = true)")
    checar("server.send(200" in wificfg, "/wificonfig responde ANTES de abrir o portal")
    hc = corpo_funcao(web, r"void\s+WebServerManager::handleClient\s*\(\s*\)") or ""
    i_stop = hc.find("relayManager->stop()")
    i_srv_stop = hc.find("server.stop()")
    i_portal = hc.find('startConfigPortal("ESP32-Config")')
    i_srv_begin = hc.find("server.begin()", max(i_portal, 0))
    checar(i_portal >= 0, "handleClient() abre startConfigPortal(\"ESP32-Config\")")
    checar(0 <= i_stop < i_portal, "ventilador e desligado ANTES de abrir o portal")
    checar(0 <= i_srv_stop < i_portal, "servidor web e parado antes do portal (libera a porta 80)")
    checar(i_srv_begin > i_portal, "servidor web volta (server.begin) depois do portal")
    checar("server.handleClient()" in hc, "handleClient() atende as requisicoes")

    print("\n== Rotas do servidor web ==")
    for rota, metodo in ROTAS.items():
        padrao = r'server\.on\(\s*"' + re.escape(rota) + r'"\s*,\s*' + metodo
        checar(re.search(padrao, web), f"rota {rota} ({metodo})")
    checar("initialHTML" in corpo_funcao(web, r"void\s+WebServerManager::handleRecovery\s*\(\s*\)") or "",
           "/recovery usa a pagina embutida no firmware (independe do LittleFS)")

    print("\n== Envio do index.html (nao pode deixar o painel em branco) ==")
    up = corpo_funcao(web, r"void\s+WebServerManager::handleUpload\s*\(\s*\)") or ""
    upraw = corpo_funcao(web, r"void\s+WebServerManager::handleUploadRaw\s*\(\s*\)") or ""
    checar(re.search(r'server\.on\(\s*"/upload-html"\s*,\s*HTTP_POST\s*,[^;]*handleUploadRaw', web, flags=re.S),
           "/upload-html recebe o corpo em pedacos (sem carregar tudo na RAM)")
    checar('server.arg("plain")' not in up,
           "envio nao usa server.arg(\"plain\") (falhava calado com paginas grandes)")
    checar(not re.search(r'LittleFS\.open\(\s*"/index\.html"\s*,\s*"w"', up + upraw),
           "envio nunca abre /index.html para escrita direto (grava num temporario)")
    checar("clientContentLength()" in up and re.search(r"LittleFS\.rename\(\s*UPLOAD_TMP", up),
           "so troca a pagina se chegou completa (tamanho confere) e via rename")

    print("\n== Nada travando o loop ==")
    # O servidor so atende quando o loop gira. delay() nesses arquivos atrasa
    # todas as requisicoes (e o OTA).
    for nome in ["WebServerManager.cpp", "RelayManager.cpp", "DHTManager.cpp", "NTPManager.cpp"]:
        codigo = sem_comentarios(ler(nome))
        checar(not re.search(r"\bdelay\s*\(", codigo), f"{nome} sem delay()",
               f"{nome} tem delay(): trava o loop, o painel e o OTA")
    checar(not re.search(r"\bdelay\s*\(", loop), "loop() sem delay()")
    ntp = sem_comentarios(ler("NTPManager.cpp")) + sem_comentarios(ler("NTPManager.h"))
    checar("NTPClient" not in ntp and "forceUpdate" not in ntp,
           "NTP nao usa a NTPClient (que travava o loop ~1 s por tentativa)")
    checar(re.search(r"configTime\(", ntp), "NTP usa o SNTP nativo (configTime), sem bloquear")

    print("\n== Modo automatico (funciona com a pagina fechada) ==")
    relay = sem_comentarios(ler("RelayManager.cpp"))
    checar(re.search(r"relayManager\.update\(\s*dhtManager\.readTemperature\(\)\s*\)", loop),
           "loop() roda o rele com a temperatura a cada volta (nao depende da pagina)")
    checar("WebServer" not in relay and "server." not in relay,
           "RelayManager nao depende do servidor web")
    janela = corpo_funcao(relay, r"bool\s+RelayManager::isWithinActiveHours\s*\(\s*\)\s*const") or ""
    checar(re.search(r"!\s*_ntpManager->isSynced\(\)", janela) and "result = false" in janela,
           "automatico espera a hora da internet (nao usa o relogio falso de ~21:00)")
    cache = re.search(r"millis\(\)\s*-\s*_lastTimeCheck\s*<\s*(\d+)", janela)
    checar(cache and int(cache.group(1)) <= 1000,
           f"horario conferido a cada {cache.group(1) if cache else '?'} ms (liga/desliga no minuto certo)",
           "horario conferido com intervalo maior que 1 s (pode atrasar o automatico)")
    checar(re.search(r"startTime\s*<=\s*_autoSettings\.endTime", janela) and
           re.search(r"currentTime\s*>=\s*_autoSettings\.startTime\s*\|\|", janela),
           "janela que cruza a meia-noite (ex.: 21:00-05:00) continua tratada")

    print("\n== Economia de energia do Wi-Fi ==")
    bg = corpo_funcao(web, r"void\s+WebServerManager::begin\s*\([^)]*\)") or ""
    i_obs = bg.find("addHandler(new ActivityHandler")
    i_rota = bg.find("server.on(")
    checar(0 <= i_obs < i_rota, "observador de atividade registrado ANTES das rotas",
           "observador de atividade ausente ou depois das rotas (nao veria as requisicoes)")
    obs = corpo_funcao(web, r"bool\s+canHandle\s*\([^)]*\)\s*override") or ""
    checar("return false" in obs and "return true" not in obs,
           "observador nunca \"rouba\" a requisicao (canHandle retorna false)")
    checar(re.search(r"WiFi\.setSleep\(\s*true\s*\)", bg), "economia de energia LIGADA por padrao")
    checar("updatePowerSave()" in hc, "handleClient() religa a economia depois de um tempo parado")
    checar(re.search(r"WiFi\.setSleep\(\s*true\s*\)", hc[i_portal:] if i_portal >= 0 else ""),
           "depois do portal volta para o modo economico")


# =============================================================================
#  MODO PLACA (ao vivo, so leituras)
# =============================================================================
def get(host, rota, timeout=8):
    inicio = time.perf_counter()
    with urllib.request.urlopen(f"http://{host}{rota}", timeout=timeout) as r:
        corpo = r.read()
        ttfb = time.perf_counter() - inicio
        return r.status, corpo.decode("utf-8", errors="replace"), ttfb


def resolver(host):
    try:
        return socket.gethostbyname(host)
    except OSError:
        return None


def mdns_tem_servico(servico, timeout=3.0):
    """Pergunta na rede (mDNS) quem oferece 'servico' (ex.: _arduino._tcp.local).
    Devolve os bytes de todas as respostas recebidas."""
    def nome_dns(n):
        return b"".join(bytes([len(p)]) + p.encode() for p in n.split(".")) + b"\x00"

    # Cabecalho: id 0, flags 0, 1 pergunta. Tipo PTR (12), classe IN com bit
    # "unicast response" (0x8001) para a resposta voltar direto para nos.
    pacote = struct.pack(">HHHHHH", 0, 0, 1, 0, 0, 0) + nome_dns(servico) + struct.pack(">HH", 12, 0x8001)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    s.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 255)
    s.settimeout(0.5)
    respostas = b""
    try:
        s.bind(("", 0))
        s.sendto(pacote, ("224.0.0.251", 5353))
        fim = time.time() + timeout
        while time.time() < fim:
            try:
                dados, _ = s.recvfrom(9000)
                respostas += dados
            except socket.timeout:
                pass
    finally:
        s.close()
    return respostas


def redes_wifi_visiveis():
    try:
        saida = subprocess.run(["netsh", "wlan", "show", "networks"], capture_output=True,
                               text=True, errors="replace", timeout=20).stdout
        return saida
    except Exception:
        return None


def teste_placa(host, firmware_esperado, limite_ms, testar_portal, testar_upload=False):
    print(f"\n== Conexao com {host} ==")
    ip = resolver(host)
    if not checar(ip is not None, f"{host} resolve para {ip}", f"nao foi possivel resolver {host} (placa desligada ou fora da rede?)"):
        return
    alvo = ip  # usa o IP direto para medir a placa, nao o mDNS do Windows

    print("\n== Rotas (somente leitura) ==")
    tempos = []
    esperados = {
        "/remaining": ["remaining"],
        "/get-auto-settings": ["active", "temp", "ventTime", "standby", "startTime", "endTime"],
        "/system": ["memory", "system"],
        "/flash-info": ["flash", "fs"],
        "/device-info": ["ssid", "ip", "rssi", "hostname", "otaDefaultPassword"],
        "/state": ["time", "synced", "on", "status", "remaining", "auto", "sensor"],
    }
    dados = {}
    for rota in ["/", "/recovery", "/time", "/status", "/remaining", "/get-auto-settings",
                 "/sensor-data", "/system", "/flash-info", "/device-info", "/state"]:
        try:
            status, corpo, ttfb = get(alvo, rota)
        except Exception as e:
            falha(f"{rota}: {e}")
            continue
        if rota not in ("/", "/recovery"):
            tempos.append(ttfb)
        if not checar(status == 200, f"{rota} -> 200 ({ttfb * 1000:.0f} ms)", f"{rota} -> {status}"):
            continue
        if rota in esperados:
            try:
                j = json.loads(corpo)
                dados[rota] = j
                faltando = [c for c in esperados[rota] if c not in j]
                checar(not faltando, f"{rota} tem os campos esperados", f"{rota} sem os campos {faltando}")
            except ValueError:
                falha(f"{rota} nao devolveu JSON valido")
        if rota == "/recovery":
            checar("Modo de recuperacao" in corpo, "/recovery mostra a pagina de recuperacao embutida")
        if rota == "/status":
            checar(corpo in ("Ventilador Ligado", "Ventilador Desligado"), f"/status = \"{corpo}\"")
        if rota == "/sensor-data":
            try:
                j = json.loads(corpo)
                if "error" in j:
                    aviso(f"sensor DHT com erro: {j['error']}")
                else:
                    ok(f"sensor: {j.get('temp')} C, {j.get('humidity')} %")
            except ValueError:
                falha("/sensor-data nao devolveu JSON valido")
        if rota == "/time":
            if checar(re.fullmatch(r"\d{2}:\d{2}:\d{2}", corpo), f"/time no formato HH:MM:SS ({corpo})"):
                agora = datetime.now(timezone(timedelta(hours=-3)))
                h, m, s = map(int, corpo.split(":"))
                placa = agora.replace(hour=h, minute=m, second=s)
                dif = abs((placa - agora).total_seconds())
                dif = min(dif, 86400 - dif)  # perto da meia-noite
                # Compara com o relogio do PC (que pode estar alguns segundos fora)
                checar(dif <= 5, f"hora da placa bate com o PC (diferenca {dif:.0f} s) -> NTP funcionando",
                       f"hora da placa {dif:.0f} s diferente do PC -> NTP nao esta atualizando")

    if tempos:
        mediana = statistics.median(tempos) * 1000
        checar(mediana <= limite_ms, f"tempo de resposta mediano {mediana:.0f} ms (limite {limite_ms} ms)",
               f"tempo de resposta mediano {mediana:.0f} ms, acima do limite de {limite_ms} ms")

    info = dados.get("/device-info", {})
    if firmware_esperado:
        checar(info.get("firmware") == firmware_esperado, f"firmware = {firmware_esperado}",
               f"firmware na placa = {info.get('firmware', '(campo ausente: firmware antigo)')}, esperado {firmware_esperado}")
    else:
        print(f"  [INFO]  firmware na placa: {info.get('firmware', '(campo ausente: firmware antigo)')}")
    if info:
        checar(info.get("otaDefaultPassword") is False, "OTA com senha personalizada (nao voltou para \"senha\")",
               "OTA voltou para a senha padrao \"senha\": ota_senha.ini nao vai bater")

    print("\n== OTA anunciado na rede (mDNS) ==")
    resp = mdns_tem_servico("_arduino._tcp.local")
    checar(b"_arduino" in resp and b"esp32" in resp,
           "placa anuncia _arduino._tcp (o OTA esta escutando)",
           "nenhuma resposta _arduino._tcp: o OTA pode nao estar ativo")
    resp = mdns_tem_servico("_http._tcp.local")
    checar(b"_http" in resp and b"esp32" in resp, "placa anuncia _http._tcp (app Android encontra)",
           "nenhuma resposta _http._tcp: o app Android nao vai achar a placa")

    if testar_upload:
        teste_upload(alvo)

    if testar_portal:
        teste_portal(alvo, host)


def teste_upload(ip):
    """Envia o data/index.html do repositorio e confere que a placa devolve igual."""
    print("\n== Envio do index.html (data/index.html do repositorio) ==")
    pagina = (SRC / "data" / "index.html").read_bytes()
    req = urllib.request.Request(f"http://{ip}/upload-html", data=pagina, method="POST",
                                 headers={"Content-Type": "text/plain"})
    try:
        with urllib.request.urlopen(req, timeout=60) as r:
            checar(r.status == 200, f"/upload-html aceitou {len(pagina)} bytes")
    except Exception as e:
        falha(f"/upload-html: {e}")
    try:
        with urllib.request.urlopen(f"http://{ip}/", timeout=20) as r:
            servida = r.read()
        checar(servida == pagina, f"placa devolve a pagina identica ({len(servida)} bytes)",
               f"pagina servida difere da enviada ({len(servida)} de {len(pagina)} bytes)")
    except Exception as e:
        falha(f"GET /: {e}")


def teste_portal(ip, host):
    print("\n== Portal Wi-Fi (ESP32-Config) ==")
    print("  Abrindo o portal: o ventilador sera desligado e a placa sai da rede por ate 3 min.")
    try:
        status, corpo, _ = get(ip, "/wificonfig")
        checar(status == 200 and "ESP32-Config" in corpo, "/wificonfig respondeu e citou ESP32-Config")
    except Exception as e:
        falha(f"/wificonfig: {e}")
        return

    viu_rede = False
    fim = time.time() + 60
    while time.time() < fim:
        redes = redes_wifi_visiveis()
        if redes is None:
            aviso("netsh indisponivel: nao da para conferir a rede ESP32-Config")
            break
        if "ESP32-Config" in redes:
            viu_rede = True
            break
        time.sleep(5)
    if redes is not None:
        checar(viu_rede, "rede ESP32-Config apareceu", "rede ESP32-Config NAO apareceu em 60 s")

    print("  Esperando a placa voltar para a rede (ate 4 min)...")
    fim = time.time() + 240
    voltou = False
    while time.time() < fim:
        try:
            status, _, _ = get(ip, "/status", timeout=3)
            if status == 200:
                voltou = True
                break
        except Exception:
            pass
        time.sleep(5)
    checar(voltou, "placa voltou para a rede depois do portal", "placa NAO voltou em 4 min")
    if voltou:
        resp = mdns_tem_servico("_arduino._tcp.local")
        checar(b"esp32" in resp, "OTA continua anunciado depois do portal",
               "OTA nao respondeu no mDNS depois do portal")


def main():
    p = argparse.ArgumentParser(description="Testes de regressao do ProjetoQuartoEsp32")
    sub = p.add_subparsers(dest="modo", required=True)
    sub.add_parser("estatico", help="confere o codigo-fonte (sem placa)")
    pl = sub.add_parser("placa", help="testa a placa na rede (so leituras)")
    pl.add_argument("--host", default="esp32.local")
    pl.add_argument("--firmware", help="versao esperada em /device-info (ex.: v2.6.0)")
    pl.add_argument("--limite-ms", type=int, default=400, help="tempo de resposta mediano maximo")
    pl.add_argument("--testar-portal", action="store_true",
                    help="ABRE o portal de verdade (desliga o ventilador, placa fora da rede ate 3 min)")
    pl.add_argument("--testar-upload", action="store_true",
                    help="ENVIA o data/index.html do repositorio para a placa e confere que voltou igual")
    a = p.parse_args()

    if a.modo == "estatico":
        teste_estatico()
    else:
        teste_placa(a.host, a.firmware, a.limite_ms, a.testar_portal, a.testar_upload)

    print()
    if falhas:
        print(f"RESULTADO: {len(falhas)} falha(s), {len(avisos)} aviso(s)")
        sys.exit(1)
    print(f"RESULTADO: tudo certo ({len(avisos)} aviso(s))")


if __name__ == "__main__":
    main()
