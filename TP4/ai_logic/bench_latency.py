import time, json, statistics, sys
import paho.mqtt.client as m
dev="esp-01"
lats=[]
def on(c,u,msg):
    try:
        j=json.loads(msg.payload.decode())
        if j.get("device_id")!=dev: return
        t_ms=int(j.get("t_ms",0))
        if t_ms<=0: return
        now=int(time.time()*1000)
        lats.append(now - t_ms)
        print(f"latency_ms={lats[-1]}", flush=True)
        if len(lats)>=10:
            c.disconnect()
    except Exception as e:
        print("err",e, file=sys.stderr)
c=m.Client()
c.on_message=on
c.connect("broker.mqtt.cool",1883,60)
c.subscribe("esp32/data",1)
c.loop_forever()
if lats:
    avg=statistics.mean(lats)
    p95=sorted(lats)[int(0.95*(len(lats)-1))]
    open("report.md","a",encoding="utf-8").write(f"\\n\\n## Latency Results\\n- Samples: {len(lats)}\\n- Avg latency: {avg:.1f} ms\\n- P95 latency: {p95:.1f} ms\\n")
    print("Appended latency stats to report.md")
else:
    print("No samples captured")
