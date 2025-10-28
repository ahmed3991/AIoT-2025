# ملخص تنفيذ TP4

يوضح هذا المستند صورة سريعة عن كيفية تنفيذ حل TP4 وخطوات تشغيله بالاعتماد على كل من تدفقَي MQTT وHTTP/REST.

## نظرة عامة على الحل
- البرنامج الثابت للوحة ESP32 يبني حمولة JSON موحدة تتضمن `device_id` والمتجهات الحسية، ثم يرسلها عبر MQTT أو REST بحسب العلم `USE_HTTP_TRANSPORT`.
- سكربت Python `mqtt_ai_subscriber.py` يستقبل القياسات من الموضوع `esp32/data`، يحمل خطوط TP2 (Logistic Regression أو XGBoost)، يجري الاستدلال، ثم يعيد نشر قرار التحكم إلى `esp32/control` مع احتمال التوقع عند توفره.
- خادم Flask الموجود في `http_ai_server.py` يعرض المسار `POST /infer` لمعالجة نفس الحمولة، ويعيد مخطط الاستجابة ذاته كي تتمكن اللوحة من تحديث LED وLCD.
- مجلد `TP4/models/` يحتوي ملفات النماذج الملتقطة من TP2 والمطلوبة لتشغيل الطرفين.

## تحضير البيئة البرمجية
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## تشغيل مسار MQTT
```bash
python TP4/ai_logic/mqtt_ai_subscriber.py --models-dir TP4/models --broker <عنوان_الوسيط> --port 1883
```
يمكن تعديل علم `--model` لاختيار النموذج (`lr` أو `xgb`) وتحديث أسماء المواضيع عبر `--topic-in` و`--topic-out` إذا لزم الأمر.

## تشغيل مسار HTTP/REST
```bash
python TP4/ai_logic/http_ai_server.py --models-dir TP4/models --host 0.0.0.0 --port 8000
```
بعد ذلك ترسل اللوحة (أو أداة اختبار) طلبًا إلى `http://<الخادم>:8000/infer` بالحمولة ذاتها المستخدمة في MQTT.

## بناء البرنامج الثابت للوحة ESP32
```bash
platformio run -d TP4
```
لبرمجة اللوحة عبر USB:
```bash
platformio run -d TP4 -t upload
```
ولمتابعة السجلات التسلسلية:
```bash
platformio device monitor -d TP4
```

بهذه الخطوات يكون تنفيذ TP4 مكتملًا ويمكن التبديل بين النقل عبر MQTT أو HTTP حسب متطلبات النشر.
