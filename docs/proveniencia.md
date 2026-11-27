# Proveniência do acervo

As datas das capturas registram ensaios, não datas de lançamento ou tags de firmware. No começo de 2026, a interface mais simples já tinha console e CSV. Depois apareceram a busca por IDs e o gerenciador de Wi-Fi. As capturas de março registram os testes com o ramo específico do MMW03.

| Origem no acervo | Nome nesta publicação |
| --- | --- |
| `13.scanner_modbus` | [scanner_modbus_v1](../sketches/scanner_modbus_v1/) |
| `17_scanner_modbus_v2.0` | [scanner_modbus_v2](../sketches/scanner_modbus_v2/) |
| `19.scanner_modbus_v3.0` | [scanner_modbus_v3](../sketches/scanner_modbus_v3/) |
| `26.cacador_de_reg` | [Scanner de Registradores Modbus](../sketches/scanner_registradores_modbus/) |
| `27.scanner_modbus` | [scanner_modbus_configuravel](../sketches/scanner_modbus_configuravel/) |

O `index.html` separado das duas primeiras pastas era idêntico. Aqui há [uma só cópia](interface-inicial.html). A interface da v2.0 para busca por ID é o HTML embarcado no próprio sketch, não esse arquivo separado.

## Ambiente

Os sketches usam o core Arduino do **ESP8266** e a biblioteca **modbus-esp8266**. As primeiras etapas também usam **WebSockets**, **ArduinoJson** e **RTClib**; o gerenciador de Wi-Fi usa a EEPROM emulada do core. A versão de uma dependência pode alterar os diagnósticos ou a compilação. Os arquivos não constituem um único firmware com um ambiente de build travado.

Na cópia pública do scanner configurável, sete linhas em `04_Web_UI.ino` usam limites do tipo `long` em `min` e `max`, compatíveis com o retorno de `String::toInt()` no core ESP8266. Os valores dos limites e os parâmetros de comunicação não mudaram. O arquivo histórico permanece como estava.

Em outra etapa do projeto, passei da Arduino IDE para o VS Code com PlatformIO e passei a usar IA com mais frequência. O acervo preservado não contém o `platformio.ini` histórico dessa etapa. Não atribuo essa mudança a uma revisão específica nem uso um manifesto novo como se fosse o original.
