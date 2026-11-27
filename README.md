# Scanner Modbus

Eu tinha uma aplicação Modbus com parâmetros fixos que não funcionava. Em vez de continuar mudando o programa a cada hipótese, reaproveitei um **SolarView uBox**: o ESP8266 e a interface RS485 do datalogger viraram uma ferramenta para entender o que o **WEG MMW03** realmente devolvia.

Queria separar três perguntas: a velocidade de comunicação estava certa? O endereço no manual já era o endereço enviado no quadro ou ainda precisava de conversão? E a consulta retornava dados? A ferramenta foi mudando conforme essas perguntas mudavam. Aqui estão os sketches dessas etapas, com as limitações que tinham na época.

Este registro faz parte do acervo da [Galeria -2048](https://www.galeria-2048.com.br/).

## Os sketches

| Sketch | O que eu estava tentando fazer |
| --- | --- |
| [Primeiro scanner](sketches/scanner_modbus_v1/) | Consultar uma faixa de registradores de um ID já conhecido. A página embarcada mandava os resultados ao console por WebSocket e permitia salvar o texto em CSV. |
| [Scanner v2.0](sketches/scanner_modbus_v2/) | Procurar IDs com uma leitura de teste e operar pela rede local. O gerenciador de Wi-Fi guardava a configuração em EEPROM emulada e deixava o ponto de acesso disponível para configurar. |
| [Scanner v3.0](sketches/scanner_modbus_v3/) | Separar a busca por IDs, a leitura avulsa, o roteamento WebSocket e a interface em arquivos. A aba de escrita era um espaço reservado; a leitura avulsa não substituía a varredura de uma faixa do primeiro sketch. |
| [Scanner de Registradores Modbus](sketches/scanner_registradores_modbus/) | Voltar a uma pergunta específica do MMW03. O ensaio usava 38.400 bit/s; a tabela podia mostrar duas palavras de 16 bits e duas interpretações em ponto flutuante. A interpretação depende do mapa do equipamento. |
| [Scanner configurável](sketches/scanner_modbus_configuravel/) | Reunir ajustes da serial, busca por ID, busca por faixa, leitura avulsa e log na interface. O Wi-Fi voltou a operar pelo ponto de acesso do próprio scanner, sem o gerenciador anterior. A busca por ID tratava o callback da biblioteca como presença; um timeout também pode disparar esse callback, então ele não é prova de resposta. |

As pastas não representam uma fila de versões que precisam ser instaladas uma sobre a outra. O ramo do MMW03 e o configurável são experimentos distintos. Cada diretório contém seu próprio sketch principal, com o mesmo nome da pasta para abrir na Arduino IDE.

## O medidor que ficou em silêncio

Usei a v2.0 para investigar um DOW1310L que registrava energia injetada na rede, não o consumo da instalação. O manual mencionava RS485, a interface elétrica. A busca não trouxe respostas e eu não encontrei um mapa de registradores. O teste não deu base para dizer se o medidor falava Modbus ou outro protocolo.

No MMW03, as capturas mostram algo bem diferente: uma consulta podia devolver palavras com valor zero. Em outro ensaio, nenhuma leitura entrou na tabela. Ler zero e não conseguir ler continuavam sendo resultados diferentes.

## Acervo e limites

- [Proveniência, dependências e limites](docs/proveniencia.md)
- [HTML da primeira interface](docs/interface-inicial.html) — cópia histórica; o console dependia do firmware no ESP8266.
- [Fotos e capturas da bancada](media/) — versões tratadas, sem os arquivos originais ou seus metadados.

O código é para diagnóstico em bancada ou em equipamento que você tenha autorização para testar. Os pontos de acesso têm configurações históricas para uso local e não são uma configuração segura de produção. As credenciais de uma rede externa não vêm com os sketches: nas versões com gerenciador de Wi-Fi, são informadas na interface e armazenadas no próprio dispositivo.
