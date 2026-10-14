# Araçlar

## frame_tool.py

LoRa protokol çerçevelerini çözer ve üretir. Seri monitörde gördüğünüz baytları anlamak için kullanın.

```bash
python3 tools/frame_tool.py decode "01 01 10 01 2A 07 01 02 04 E2 02 01 01 45 CB"
python3 tools/frame_tool.py telemetry --src 0x1001 --seq 1 --level 1250 --valve 1
python3 tools/frame_tool.py command --seq 7 --cmd-id 15 --op VANA_AC
python3 -m unittest discover tools     # testler
```
