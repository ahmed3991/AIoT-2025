import json,time
import paho.mqtt.client as m
def on(c,u,msg):
    j=json.loads(msg.payload.decode())
    now=int(time.time()*1000)
    t_ms=int(j.get("t_ms",0))
    print(f"seq={j.get('seq')} latency_ms={now - t_ms}", flush=True)
c=m.Client()
c.on_message=on
c.connect("broker.mqtt.cool",1883,60)
c.subscribe("esp32/data",1)
c.loop_forever()
