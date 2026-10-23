#!/usr/bin/env python3
"""
TP10: WebSocket to MQTT Audio Dispatcher
Receives real-time audio from WebSocket clients and streams to ESP32 via MQTT
"""

import asyncio
import argparse
import sys
import struct
import numpy as np
import paho.mqtt.client as mqtt
from websockets.server import serve
import json

DEFAULT_MQTT_BROKER = "broker.mqtt.cool"
DEFAULT_MQTT_PORT = 1883
DEFAULT_MQTT_TOPIC = "ei/audio/raw"
DEFAULT_WS_PORT = 8765
DEFAULT_SLICE_SIZE = 1024

class WebSocketAudioBridge:
    def __init__(self, mqtt_broker, mqtt_port, mqtt_topic, ws_port, slice_size, verbose=False):
        self.mqtt_broker = mqtt_broker
        self.mqtt_port = mqtt_port
        self.mqtt_topic = mqtt_topic
        self.ws_port = ws_port
        self.slice_size = slice_size
        self.verbose = verbose
        self.mqtt_client = None
        self.mqtt_connected = False
        self.audio_buffer = []
        self.total_slices_sent = 0

    def on_mqtt_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self.mqtt_connected = True
            print(f"✓ Connected to MQTT broker: {self.mqtt_broker}:{self.mqtt_port}")
        else:
            print(f"✗ MQTT connection failed with code {rc}")
            self.mqtt_connected = False

    def on_mqtt_disconnect(self, client, userdata, rc):
        self.mqtt_connected = False
        if rc != 0:
            print(f"✗ Unexpected MQTT disconnection (code {rc})")

    def connect_mqtt(self):
        print(f"\n📡 Connecting to MQTT broker: {self.mqtt_broker}:{self.mqtt_port}")
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_disconnect = self.on_mqtt_disconnect

        try:
            self.mqtt_client.connect(self.mqtt_broker, self.mqtt_port, 60)
            self.mqtt_client.loop_start()

            import time
            timeout = 10
            start_time = time.time()
            while not self.mqtt_connected and (time.time() - start_time) < timeout:
                time.sleep(0.1)

            if not self.mqtt_connected:
                print("✗ MQTT connection timeout")
                return False

            return True
        except Exception as e:
            print(f"✗ MQTT connection error: {e}")
            return False

    def disconnect_mqtt(self):
        if self.mqtt_client:
            self.mqtt_client.loop_stop()
            self.mqtt_client.disconnect()
            print("\n📡 Disconnected from MQTT broker")

    async def handle_websocket(self, websocket, path):
        client_addr = websocket.remote_address
        print(f"\n🌐 WebSocket client connected: {client_addr[0]}:{client_addr[1]}")

        try:
            async for message in websocket:
                if isinstance(message, str):
                    try:
                        data = json.loads(message)
                        if data.get('type') == 'config':
                            print(f"📋 Client config: {data}")
                            await websocket.send(json.dumps({
                                'type': 'config_ack',
                                'slice_size': self.slice_size,
                                'mqtt_topic': self.mqtt_topic
                            }))
                    except json.JSONDecodeError:
                        print(f"⚠ Invalid JSON message: {message}")

                elif isinstance(message, bytes):
                    await self.process_audio_data(message, websocket)

        except Exception as e:
            print(f"✗ WebSocket error: {e}")
        finally:
            print(f"🌐 WebSocket client disconnected: {client_addr[0]}:{client_addr[1]}")

    async def process_audio_data(self, audio_bytes, websocket):
        audio_samples = np.frombuffer(audio_bytes, dtype=np.int16)

        if self.verbose:
            print(f"📥 Received {len(audio_samples)} samples ({len(audio_bytes)} bytes)")

        self.audio_buffer.extend(audio_samples)

        while len(self.audio_buffer) >= self.slice_size:
            slice_samples = self.audio_buffer[:self.slice_size]
            self.audio_buffer = self.audio_buffer[self.slice_size:]

            slice_array = np.array(slice_samples, dtype=np.int16)
            payload = slice_array.tobytes()

            if self.mqtt_connected:
                result = self.mqtt_client.publish(self.mqtt_topic, payload, qos=0)

                if result.rc == mqtt.MQTT_ERR_SUCCESS:
                    self.total_slices_sent += 1

                    if self.verbose:
                        print(f"📤 Sent slice #{self.total_slices_sent} to MQTT ({len(payload)} bytes)")
                    elif self.total_slices_sent % 10 == 0:
                        print(f"📊 Progress: {self.total_slices_sent} slices sent")

                    ack_msg = json.dumps({
                        'type': 'ack',
                        'slice_number': self.total_slices_sent,
                        'samples_received': len(slice_samples)
                    })
                    await websocket.send(ack_msg)
                else:
                    print(f"✗ MQTT publish failed: {mqtt.error_string(result.rc)}")
            else:
                print("⚠ MQTT not connected, dropping audio slice")

    async def start_websocket_server(self):
        print(f"\n🌐 Starting WebSocket server on port {self.ws_port}")
        print(f"   Waiting for audio clients to connect...")

        async with serve(self.handle_websocket, "0.0.0.0", self.ws_port):
            print(f"✓ WebSocket server ready at ws://localhost:{self.ws_port}")
            await asyncio.Future()

async def main_async(args):
    bridge = WebSocketAudioBridge(
        mqtt_broker=args.mqtt_broker,
        mqtt_port=args.mqtt_port,
        mqtt_topic=args.topic,
        ws_port=args.ws_port,
        slice_size=args.slice_size,
        verbose=args.verbose
    )

    if not bridge.connect_mqtt():
        sys.exit(1)

    try:
        await bridge.start_websocket_server()
    except KeyboardInterrupt:
        print("\n\n⏹ Stopped by user")
    finally:
        bridge.disconnect_mqtt()
        print(f"\n📊 Total slices sent: {bridge.total_slices_sent}")

def main():
    parser = argparse.ArgumentParser(description="WebSocket to MQTT Audio Bridge for Edge Impulse")
    parser.add_argument('--mqtt-broker', '-b', default=DEFAULT_MQTT_BROKER, help='MQTT broker address')
    parser.add_argument('--mqtt-port', '-p', type=int, default=DEFAULT_MQTT_PORT, help='MQTT broker port')
    parser.add_argument('--topic', '-t', default=DEFAULT_MQTT_TOPIC, help='MQTT topic')
    parser.add_argument('--ws-port', '-w', type=int, default=DEFAULT_WS_PORT, help='WebSocket server port')
    parser.add_argument('--slice-size', '-s', type=int, default=DEFAULT_SLICE_SIZE, help='Samples per slice')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')

    args = parser.parse_args()

    try:
        asyncio.run(main_async(args))
    except KeyboardInterrupt:
        print("\n⏹ Shutting down...")

if __name__ == "__main__":
    main()
