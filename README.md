# Raspberry Pi AVoIP Visual Processing System

**By Fabio Salgado**

A Raspberry Pi-based prototype demonstrating the architecture of an **Audio/Video over Internet Protocol (AVoIP)** system, including video capture, encoding, network transport, decoding, digital signal processing (DSP), and web-based control.

---

## Overview

This project implements a simplified AVoIP pipeline using multiple Raspberry Pis connected through a **Gigabit PoE network switch**.

The system uses **Power over Ethernet (PoE)** to provide both power and network connectivity over Cat 6 Ethernet cables. This allows the Raspberry Pis to communicate and receive power through the same physical connection.

### System Pipeline

The system consists of three primary types of nodes:

### Encoder

Encoder nodes receive video input and compress it for transmission across the network.

Two input sources were implemented:

* **Camera input** using a Raspberry Pi Camera Module
* **HDMI input** for screen/device mirroring

Video compression is necessary because the network switch provides a maximum **1 Gbps Ethernet connection**.

### Core

The **Raspberry Pi 5** acts as the central processing node.

The core is responsible for:

* Receiving and decoding video
* Routing inputs to outputs
* Applying digital signal processing
* Re-encoding processed video
* Transmitting video to output nodes
* Hosting a web-based control interface

Because the core must decode, process, and re-encode the video stream, it carries the majority of the computational workload.

### Decoder

Decoder nodes receive the encoded video stream over Ethernet, decode it, and output the resulting video to a display.

---

## Hardware

The system was built using:

* Raspberry Pi 5 — Core/DSP processor
* Raspberry Pi 4 — Encoder/decoder nodes
* Raspberry Pi Camera Module 3
* HDMI capture device
* Gigabit PoE network switch
* Raspberry Pi PoE HATs
* Cat 6 Ethernet cables
* Display/monitor

The system is configured to start automatically on power-up using Linux services.

---

# Digital Signal Processing

The primary focus of this project was implementing real-time **image processing on the Raspberry Pi 5**.

The DSP implementation is contained primarily within the `av_dsp` directory.

Four convolution-based image filters were implemented:

1. Sharpen
2. Blur
3. Emboss
4. Edge Detection

Additionally, the system provides adjustable:

* Brightness
* Contrast

---

## 2D Convolution

Image filtering differs from traditional one-dimensional signal processing because each pixel is influenced by neighboring pixels in both the horizontal and vertical directions.

For this reason, the system uses **2D convolution with 3×3 kernels**.

For example, the sharpening kernel is:

```text
 0  -1   0
-1   5  -1
 0  -1   0
```

For each pixel, the kernel weights are multiplied by the corresponding neighboring pixel values and summed to produce a new pixel value.

This operation is performed independently on the **red, green, and blue channels**.

### Example

Consider a bright pixel surrounded by darker pixels:

```text
50   50   50
50  200   50
50   50   50
```

Applying the sharpening kernel:

```text
(50×0) + (50×-1) + (50×0)
+ (50×-1) + (200×5) + (50×-1)
+ (50×0) + (50×-1) + (50×0)
```

produces:

```text
800
```

Since standard 8-bit pixel values are limited to **0–255**, the result is clamped to:

```text
255
```

The center pixel therefore becomes significantly brighter, producing the sharpening effect.

---

## Blur

The blur kernel is:

```text
1  2  1
2  4  2
1  2  1
```

The kernel uses a divisor of **16** to normalize the result:

```text
output = convolution result / 16
```

Unlike sharpening, neighboring pixels have significant influence on the center pixel, reducing differences in intensity and producing a smoothing effect.

---

## Emboss

The emboss kernel is:

```text
-2  -1   0
-1   1   1
 0   1   2
```

This kernel emphasizes directional differences between pixels.

The upper-left region is darkened while the lower-right region is brightened, creating the appearance that portions of the image have been raised.

---

## Edge Detection

The edge-detection kernel is:

```text
-1  -1  -1
-1   8  -1
-1  -1  -1
```

This strongly emphasizes differences between the center pixel and its surrounding pixels.

Areas with relatively uniform intensity produce smaller responses, while areas containing sharp transitions produce strong responses, highlighting edges.

---

# Multithreaded Image Processing

Real-time image convolution is computationally expensive.

The system processes video at approximately **30 FPS**. At 1920×1080 resolution, each frame contains:

```text
1920 × 1080 = 2,073,600 pixels
```

Since each pixel contains three color channels:

```text
2,073,600 × 3 ≈ 6.22 million channel evaluations/frame
```

Each channel evaluation requires multiple arithmetic operations due to the 3×3 convolution kernel.

Processing every pixel sequentially created significant performance limitations.

### Four-Core Processing

To improve performance, the image is divided into **four horizontal regions**.

Each region is assigned to a separate thread, allowing the four CPU cores of the Raspberry Pi 5 to process different portions of the image concurrently.

```text
┌─────────────────────────────┐
│          Thread 1           │
├─────────────────────────────┤
│          Thread 2           │
├─────────────────────────────┤
│          Thread 3           │
├─────────────────────────────┤
│          Thread 4           │
└─────────────────────────────┘
```

This parallelization significantly reduced processing time and allowed the system to maintain approximately **30 FPS under the selected operating conditions**.

---

# Brightness and Contrast

Brightness and contrast processing are implemented separately from the convolution-based filters.

## Contrast

The output pixel is calculated using:

```text
output = contrast × (input - 128) + 128 + brightness
```

The midpoint of the 8-bit pixel range is approximately **128**.

Subtracting 128 before applying the contrast factor and adding it back afterward causes pixels to move away from or toward the midpoint rather than simply scaling their values.

For example, with a contrast factor of `1.1`:

### Dark pixel

```text
1.1 × (50 - 128) + 128
= 42.2
```

The pixel becomes darker.

### Light pixel

```text
1.1 × (180 - 128) + 128
= 185.2
```

The pixel becomes lighter.

Increasing the contrast therefore pushes darker pixels toward black and brighter pixels toward white.

Brightness is then applied uniformly to the resulting pixel values.

All output values are clamped to the valid **0–255** range.

---

# Filter Implementation

The main DSP execution occurs in:

```text
AV_CORE/
└── av_dsp/
    ├── main.cpp
    └── filter.cpp
```

`filter.cpp` contains the primary image-processing implementation, while `main.cpp` coordinates the processing pipeline.

### Repeated Filtering

Because the convolution kernels operate on a relatively small **3×3 neighborhood**, a single application of blur or sharpening can produce a relatively subtle effect.

The implementation therefore allows these filters to be applied multiple times.

For example:

```text
Input
  ↓
Sharpen
  ↓
Sharpen
  ↓
Sharpen
  ↓
Output
```

Increasing the number of applications makes the visual effect more pronounced.

---

# Web Interface

The Raspberry Pi 5 core also hosts a web-based user interface for controlling the DSP system.

The interface allows the user to adjust processing parameters without directly interacting with the C++ program.

Controls include:

* Filter selection
* Filter intensity
* Brightness
* Contrast

The web interface communicates with the DSP core, allowing processing parameters to be changed while the system is running.

---

# Software Architecture

The project combines several software components:

| Component          | Purpose                                         |
| ------------------ | ----------------------------------------------- |
| **C++**            | Core processing and DSP                         |
| **Python / Flask** | Web-based control interface                     |
| **OpenCV**         | Image processing                                |
| **GStreamer**      | Video encoding, decoding, and network streaming |
| **Linux systemd**  | Automatic startup services                      |

The overall processing pipeline is:

```text
Video Source
     ↓
Capture
     ↓
Encode
     ↓
Ethernet / PoE Network
     ↓
Raspberry Pi 5 Core
     ↓
Decode
     ↓
2D DSP Processing
     ↓
Encode
     ↓
Ethernet / PoE Network
     ↓
Decoder
     ↓
Display
```

---

# Project Structure

```text
AV_CORE/
│
├── av_dsp/
│   ├── main.cpp
│   ├── filter.cpp
│   └── ...
│
├── web_ui/
│   └── ...
│
└── startup services/
    └── ...
```

The repository also contains supporting documentation, wiring diagrams, and demonstration videos.

---

# Results

The completed system successfully demonstrates the fundamental architecture of an AVoIP processing system:

* Video capture from multiple sources
* Network-based video transport
* Encoding and decoding
* Centralized video routing
* Real-time image processing
* Multithreaded DSP
* Brightness and contrast adjustment
* Web-based control
* Automatic system startup

The Raspberry Pi 5's four-core architecture was utilized to parallelize image processing and maintain approximately **30 FPS under the selected operating conditions**.

---

# Reflection and Future Improvements

This project provided practical experience with both **AV systems and real-time image processing**.

The scope of the project was intentionally broad, covering networking, video transport, embedded Linux, DSP, multithreading, and web control. While this allowed the system to demonstrate an entire AVoIP pipeline, a more focused implementation could allow deeper optimization of individual components.

One potential area for further investigation is **kernel size**. The current implementation uses 3×3 kernels to limit the number of operations per pixel. Larger kernels could potentially produce more pronounced filtering effects, but would also increase computational requirements.

Future work could therefore investigate the relationship between:

* Kernel size
* Image quality
* Processing latency
* CPU utilization
* Frame rate

---

# References

### Image Filtering

[OpenCV — Smoothing and Filtering](https://docs.opencv.org/4.5.4/d4/d13/tutorial_py_filtering.html)

### Brightness and Contrast

[OpenCV — Basic Linear Transforms](https://docs.opencv.org/4.13.0/d3/dc1/tutorial_basic_linear_transform.html)
