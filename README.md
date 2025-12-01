# 🚀 AI for IoT (AIoT) - Complete Course Journey

**Master 2 Program | University of El Oued, Algeria (2025/2026)**

---

## 📖 Course Overview

This repository documents a comprehensive learning journey through **Artificial Intelligence for Internet of Things (AIoT)**, progressively building expertise from cloud-based machine learning to real-time edge AI deployment on resource-constrained microcontrollers.

### 🎯 Learning Path

The course follows a structured progression through four main phases:

```mermaid
graph LR
    A[Phase 1: ML Fundamentals] --> B[Phase 2: Embedded ML]
    B --> C[Phase 3: Deep Learning]
    C --> D[Phase 4: MLOps & Production]
    
    A --> A1[Cloud Computing]
    A --> A2[Model Development]
    A --> A3[Resource Analysis]
    
    B --> B1[Microcontroller Deployment]
    B --> B2[Communication Protocols]
    B --> B3[Hybrid Architectures]
    
    C --> C1[CNNs for Images]
    C --> C2[TensorFlow Lite]
    C --> C3[Distributed Inference]
    
    D --> D1[MLOps Platforms]
    D --> D2[Audio Processing]
    D --> D3[Real-time Systems]
```

---

## 🌟 The Complete AIoT Journey

### **Phase 1: Machine Learning Foundations (TP1-TP2)**
*From Cloud to Constraints*

The journey begins in the **cloud environment**, where computational resources are abundant and we can focus on understanding ML fundamentals without hardware limitations.

#### [TP1: Fire Alarm Detection with Classical ML](TP1/TP1.md)
**Objective**: Build binary classification models for IoT fire detection systems

We start with the most fundamental question in AIoT: *Can we predict dangerous conditions from sensor data?* Using environmental sensor readings (temperature, humidity, gas concentrations), you'll implement:

- **Logistic Regression**: A lightweight, interpretable baseline model
- **XGBoost**: A powerful ensemble method for comparison

**Key Learning**: Understanding the trade-off between model complexity and performance - a theme that will echo throughout the entire course.

#### [TP2: Optimization & Deployment Analysis](TP2/TP2.md)
**Objective**: Measure computational footprint and evaluate hardware feasibility

Now comes the critical transition from **theory to reality**. You'll discover that a model working perfectly on Kaggle might be completely unsuitable for an ESP32. This TP introduces:

- **ML Pipelines**: Encapsulating preprocessing and inference
- **Resource Measurement**: Model size (KB), inference time (ms), memory footprint
- **The ESP32 Question**: *Can this actually run on a microcontroller?*

**Critical Insight**: The best model isn't the most accurate - it's the one that can actually deploy within your constraints (512 KB SRAM, 240 MHz processor).

---

### **Phase 2: Embedded Machine Learning (TP3-TP4)**
*Bridging Cloud and Edge*

Having measured our models, we now face the challenge: **How do we move ML from powerful servers to tiny microcontrollers?**

#### [TP3: Manual Deployment on Arduino](TP3/TP3_Logistic_Arduino.md)
**Objective**: Manually implement Logistic Regression on Arduino Uno

This is where theory meets silicon. You'll extract model parameters (weights, biases, scaling factors) from Python and manually implement the inference pipeline in C++:

```
Raw sensor data → Standardization → Linear combination → Sigmoid → Prediction
```

**Breakthrough Moment**: Running your first ML inference on a $5 microcontroller and realizing that AI doesn't require powerful GPUs - it requires smart engineering.

#### [TP4: Communication Architectures for AIoT](TP4/TP4.md)
**Objective**: Design distributed ML systems using MQTT and HTTP/REST

Real IoT systems rarely perform inference in isolation. This TP introduces **edge-cloud hybrid architectures**:

- **MQTT Pub/Sub**: ESP32 publishes sensor data → Python subscriber runs ML inference → Decision sent back
- **HTTP/REST Alternative**: Request/response pattern for comparison

**System Design Insight**: Sometimes the best deployment strategy is **not** running ML on the device - it's about choosing the right architecture for your use case (latency, power, connectivity).

---

### **Phase 3: Deep Learning for Edge AI (TP5-TP8)**
*From Simple Models to Neural Networks*

Classical ML has limits. For complex tasks like image recognition, we need **deep learning** - but how do we fit neural networks on microcontrollers?

#### [TP5: Neural Networks for Image Classification](TP5/TP5.md)
**Objective**: Compare MLP and CNN architectures on Fashion-MNIST

You'll discover why **Convolutional Neural Networks (CNNs)** are superior for images:

- **MLP**: Treats pixels as independent features (784 inputs)
- **CNN**: Learns spatial hierarchies (edges → patterns → objects)

**Performance vs. Efficiency**: CNNs achieve higher accuracy with fewer parameters than MLPs - a crucial lesson for embedded deployment.

#### [TP6: TensorFlow Lite & Quantization](TP6/TP6.md)
**Objective**: Convert Keras models to TinyML using quantization

This is where **TensorFlow Lite** bridges the gap between training frameworks and microcontrollers:

- **Quantization Magic**: Convert Float32 (4 bytes) → Int8 (1 byte) = **75% size reduction**
- **Trade-off**: ~2% accuracy loss for 4x smaller model and 2-4x faster inference

**Reality Check**: Even with quantization, you must carefully design models to fit in 512 KB SRAM.

#### [TP7: On-Device CNN Inference](TP7/TP7.md)
**Objective**: Deploy TensorFlow Lite model on ESP32-S3

The full pipeline comes together:

1. Train CNN in Keras
2. Convert to `.tflite` with Int8 quantization
3. Convert model to C header file using `xxd`
4. Integrate with TensorFlow Lite Micro interpreter
5. Run real-time inference on Fashion-MNIST images

**Technical Depth**: Understanding tensor arenas, memory allocation, and the TFLite Micro runtime.

#### [TP8: Distributed Inference Architecture](TP8/TP8.md)
**Objective**: Offload ML inference from ESP32 to Python via MQTT

A practical design pattern for production systems:

- **ESP32**: Data acquisition and publishing (low power)
- **Python Backend**: Heavy ML computation (high performance)
- **MQTT Bridge**: Asynchronous, scalable communication

**Architectural Decision**: When to run ML on-device vs. offload to the cloud? Consider: latency, power, privacy, connectivity.

---

### **Phase 4: MLOps & Production Edge AI (TP9-TP10)**
*From Prototype to Production*

The final phase introduces **MLOps (Machine Learning Operations)** - the practices and tools needed to build production-ready Edge AI systems.

#### [TP9: Edge Impulse MLOps Platform](TP9/TP9.md)
**Objective**: Build Arabic Keyword Spotting model using a professional MLOps platform

**Edge Impulse** revolutionizes TinyML development by providing:

- **End-to-End Pipeline**: Data collection → Feature engineering → Training → Deployment
- **Automated Optimization**: Automatic quantization and hardware-specific tuning
- **Audio DSP Blocks**: MFCC/MFE feature extraction (no manual implementation needed)
- **One-Click Export**: Generate ready-to-use Arduino libraries

**The Arabic KWS Project**: Train a model to recognize Arabic commands ("تمكين" - Enable, "تعطيل" - Disable) that will deploy on ESP32-S3. This mirrors how Google Assistant and Alexa work - a tiny wake-word detector runs locally on the device before sending data to the cloud.

**MLOps Concepts Learned**:
- Data versioning and management
- Experiment tracking (comparing MFCC vs MFE, CNN vs Transfer Learning)
- Automated quantization and deployment
- Model performance monitoring

#### [TP10: Real-Time Audio Inference System](TP10/TP10.md)
**Objective**: Deploy Arabic KWS model with MQTT audio streaming

The final project brings everything together in a sophisticated real-time system:

**System Architecture**:
```
Microphone (Python) → MQTT Broker → ESP32 (Wokwi) → Edge Impulse Model → Real-time Predictions
```

**Technical Challenges Solved**:
- **Circular Buffering**: Handle irregular network packet arrival with FIFO buffer
- **Audio Slicing**: Stream audio in 1024-sample chunks to prevent memory overflow
- **Continuous Inference**: Run classifier in real-time on streaming audio
- **Simulation Workaround**: Using MQTT to simulate I2S microphone in Wokwi

**The Complete Pipeline**:
1. Audio captured (browser/file) → Python dispatcher
2. Audio sliced and published via MQTT (1024 samples/packet)
3. ESP32 receives packets, writes to circular buffer
4. Edge Impulse model reads from buffer, extracts MFCC features
5. CNN classifier predicts: "enable", "disable", "unknown", or "noise"
6. Predictions displayed on LCD in real-time

---

## 📚 Practical Work Structure

| TP | Title | Technology Stack | Key Concepts |
|---|---|---|---|
| [TP1](TP1/TP1.md) | Fire Alarm Detection | Python, Scikit-learn, XGBoost, Kaggle | Binary classification, Model comparison |
| [TP2](TP2/TP2.md) | Resource Analysis | Python, Pickle, Time profiling | Model size, Inference time, ESP32 constraints |
| [TP3](TP3/TP3_Logistic_Arduino.md) | Arduino Deployment | C++, Arduino, DHT sensor, Wokwi | Manual inference, Embedded C++ |
| [TP4](TP4/TP4.md) | Communication Protocols | MQTT, HTTP/REST, ESP32, Python | Pub/Sub, Edge-Cloud architecture |
| [TP5](TP5/TP5.md) | Neural Networks | TensorFlow/Keras, Fashion-MNIST | MLP vs CNN, Deep learning |
| [TP6](TP6/TP6.md) | TensorFlow Lite | TFLite, Quantization, ESP32-S3 | Model optimization, Int8 quantization |
| [TP7](TP7/TP7.md) | CNN on ESP32 | TFLite Micro, ESP32-S3, C++ | On-device inference, Tensor arena |
| [TP8](TP8/TP8.md) | Distributed Inference | MQTT, Python, NumPy, TFLite | Offloading computation, Hybrid architecture |
| [TP9](TP9/TP9.md) | Edge Impulse MLOps | Edge Impulse, Audio DSP, CNN | MLOps, MFCC/MFE, Transfer learning |
| [TP10](TP10/TP10.md) | Real-time KWS System | ESP32-S3, MQTT, Edge Impulse, Wokwi | Streaming audio, Circular buffer, Production deployment |

---

## 🎓 Learning Outcomes

By completing this course, students will:

### 🧠 Machine Learning Expertise
- ✅ Implement classical ML (Logistic Regression, XGBoost) and deep learning (MLP, CNN)
- ✅ Understand the full ML pipeline: data → features → training → evaluation → deployment
- ✅ Master model optimization techniques (quantization, compression, pruning)
- ✅ Apply transfer learning for faster training and better performance

### 🔧 Embedded Systems Skills
- ✅ Deploy ML models on resource-constrained microcontrollers (Arduino, ESP32)
- ✅ Measure and optimize for memory (Flash, SRAM), CPU, and power constraints
- ✅ Implement real-time inference with buffering and streaming data
- ✅ Work with TensorFlow Lite Micro runtime

### 🌐 IoT System Design
- ✅ Design distributed edge-cloud ML architectures
- ✅ Implement MQTT and HTTP/REST communication protocols
- ✅ Build hybrid systems (on-device vs. cloud inference)
- ✅ Handle audio streaming and real-time signal processing

### 🚀 MLOps & Production
- ✅ Use professional MLOps platforms (Edge Impulse)
- ✅ Manage datasets, version models, track experiments
- ✅ Automate deployment pipeline from training to embedded export
- ✅ Build production-ready Keyword Spotting systems

---

## 🛠️ Technologies & Tools

### Development Platforms
- **[Kaggle Notebooks](https://www.kaggle.com/)** - Cloud ML environment (TP1-TP2)
- **[Wokwi Simulator](https://wokwi.com/)** - Online Arduino/ESP32 simulator
- **[Edge Impulse](https://www.edgeimpulse.com/)** - MLOps platform for TinyML

### Frameworks & Libraries
- **Machine Learning**: Scikit-learn, XGBoost, TensorFlow/Keras, TensorFlow Lite
- **Embedded**: Arduino, PlatformIO, ESP-IDF
- **Communication**: MQTT (PubSubClient), HTTP/REST (Flask/FastAPI)
- **Signal Processing**: Edge Impulse DSP blocks (MFCC, MFE)

### Hardware
- **Microcontrollers**: Arduino Uno, XIAO ESP32-S3
- **Sensors**: DHT22 (temperature/humidity), I2S microphone (INMP441)
- **Displays**: LCD I2C (16x2)

---

## 🔑 Key Concepts Mastered

### 💡 The AIoT Design Triangle

Every AIoT system must balance three constraints:

```
        Accuracy
           △
          ╱ ╲
         ╱   ╲
        ╱     ╲
       ╱       ╲
      ╱_________╲
  Memory    Latency
```

**Trade-off Examples**:
- **TP2**: XGBoost is more accurate but 4x larger than Logistic Regression
- **TP6**: Int8 quantization reduces size 75% with only 2% accuracy loss
- **TP8**: Offloading inference increases latency but enables complex models

### 🔄 The ML-to-Edge Pipeline

1. **Train** on powerful hardware (cloud/laptop) with full-precision (Float32)
2. **Optimize** using quantization, pruning, knowledge distillation
3. **Convert** to edge format (TensorFlow Lite, Edge Impulse Library)
4. **Deploy** on microcontroller with custom inference code
5. **Monitor** performance and iterate

### 🚀 Deployment Strategies

| Strategy | Use Case | TP Example |
|----------|----------|------------|
| **On-Device Inference** | Low latency, privacy, offline | TP7: CNN on ESP32 |
| **Cloud Offloading** | Complex models, unlimited resources | TP8: Python MQTT subscriber |
| **Hybrid Edge-Cloud** | Wake word → full NLP | TP10: KWS triggers cloud |
| **Federated Learning** | Privacy-preserving training | (Advanced topic) |

---

## 🎯 Real-World Applications

The skills learned in this course directly apply to:

- 🔥 **Smart Fire Detection Systems** (TP1-TP4)
- 🏠 **Smart Home Voice Control** (TP9-TP10)
- 📷 **Visual Quality Inspection** (TP5-TP7)
- 🏭 **Predictive Maintenance** (ML + MQTT architecture)
- 🚗 **Automotive Keyword Spotting** (Arabic voice commands)
- 🏥 **Health Monitoring Wearables** (On-device ML for privacy)

---

## 📖 How to Navigate This Repository

### For Students Following the Course
Work through the TPs **in sequential order** - each builds on concepts from previous sessions.

### For Self-Learners
1. **Start with TP1-TP2** to understand ML fundamentals and constraints
2. **Choose your path**:
   - *Hardware enthusiast*: TP3 → TP7 (embedded deployment)
   - *System designer*: TP4 → TP8 (distributed architectures)
   - *Audio/MLOps focus*: TP9 → TP10 (production systems)

### For Instructors
Each TP includes:
- Clear learning objectives
- Step-by-step instructions
- Deliverables and grading rubrics
- Extensions and bonus challenges

---

## 🌍 Why Arabic Keyword Spotting?

TP9-TP10 focus on **Arabic voice commands** to:
- Address the **underrepresentation of Arabic** in ML datasets
- Make AIoT relevant to **local applications and markets**
- Demonstrate **cultural adaptation** of ML systems
- Provide students with **practical, deployable skills** for regional industry

---

## 🔬 Advanced Topics & Extensions

The course provides natural pathways to advanced research:

- **Federated Learning**: Train models across distributed IoT devices without sharing raw data
- **Neural Architecture Search**: Automated design of efficient TinyML models
- **Streaming Audio Classification**: Extend TP10 to multiple languages
- **Computer Vision on Edge**: Deploy YOLOv8-Nano on ESP32-S3
- **Energy-Aware ML**: Profile power consumption and optimize for battery life

---

## 📊 Course Statistics

- **Total Practical Works**: 10
- **Technologies Covered**: 15+
- **Programming Languages**: Python, C++, (JavaScript for web clients)
- **Hardware Platforms**: Arduino Uno, ESP32-S3, Wokwi Simulator
- **ML Paradigms**: Classical ML, Deep Learning, Transfer Learning, TinyML
- **Communication Protocols**: MQTT, HTTP/REST, WebSockets

---

## 👥 Contributors

**Master 2 Students - AI for IoT Course**  
University of El Oued, Algeria  
Academic Year: 2025/2026

---

## 📜 License

Educational materials for university coursework.

---

## 🙏 Acknowledgments

- **Edge Impulse** for providing a free MLOps platform for education
- **Kaggle** for free cloud GPU resources and datasets
- **Wokwi** for enabling embedded system simulation in browsers
- **TensorFlow Lite** team for making TinyML accessible