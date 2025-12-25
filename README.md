# ATS ESP32 Firmware Demo

> **ESP32 firmware designed for automated hardware testing**

This firmware exists solely to demonstrate automated hardware testing and is not intended to be a production product.

---

## 📁 Repository Structure

```
ats-fw-esp32-demo/
├── README.md
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── app_main.c
│   ├── gpio_demo.c
│   ├── oled_demo.c
│   └── ota.c
├── platforms/
│   ├── ESP32/
│   │   ├── Jenkinsfile          # Build pipeline
│   │   └── Jenkinsfile.test     # Test pipeline
│   ├── RaspberryPi/
│   └── README.md
└── sdkconfig
```

---

## 🎯 Purpose

This firmware is built on the Xeon server as part of the CI/CD pipeline.

**Key points:**

- ✅ Firmware is built on the Xeon server
- ✅ ATS nodes never build firmware
- ✅ ATS nodes only consume signed/versioned artifacts for hardware validation
- ✅ **Firmware artifacts produced by this repository are validated using `ats-test-esp32-demo`**

---

## 🔄 Build Process

1. **Source checkout** from Git repository
2. **ESP-IDF build** on Jenkins build agent (`fw-build` label)
3. **Artifact generation:**
   - `firmware-esp32.bin` (firmware binary)
   - `ats-manifest.yaml` (build metadata)
4. **Artifact archiving** in Jenkins
5. **Tag creation** (local Git tag for versioning)

---

## 🧪 Test Integration

Firmware artifacts are automatically tested using the `ats-test-esp32-demo` framework:

- Test pipeline copies artifacts from build job
- Test execution runs on ATS nodes (Raspberry Pi)
- Hardware validation includes:
  - UART boot validation
  - GPIO behavior
  - OLED display
  - Firmware stability

**The firmware repository does not contain test execution logic** — that responsibility belongs to `ats-test-esp32-demo`.

---

## 🏗️ Multi-Platform Support

The repository is organized by platform:

```
platforms/
├── ESP32/          # ESP32 firmware build and test
├── RaspberryPi/    # Raspberry Pi image build (future)
└── nRF52/          # nRF52 firmware build (future)
```

Each platform has its own:
- Build pipeline (`Jenkinsfile`)
- Test pipeline (`Jenkinsfile.test`)

---

## 📦 Artifacts

### Firmware Binary

- **Name:** `firmware-{PLATFORM}.bin`
- **Format:** ESP32 binary image
- **Location:** Jenkins artifact archive

### ATS Manifest

- **Name:** `ats-manifest.yaml`
- **Contains:**
  - Build metadata (CI system, job name, build number)
  - Git information (repo, commit, branch)
  - Artifact checksum (SHA256)
  - Device target information
  - Test plan references

---

## 🔗 Relationship to Other Repositories

- **`ats-ci-infra`**: Build infrastructure and pipeline orchestration
- **`ats-test-esp32-demo`**: Hardware test execution framework
- **`ats-platform-docs`**: System documentation and architecture

---

## 👤 Author

**Hai Dang Son**  
Senior Embedded / Embedded Linux / IoT Engineer
