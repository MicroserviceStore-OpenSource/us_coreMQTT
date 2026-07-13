# coreMQTT Microservice

An embedded Microservice that wraps the **FreeRTOS coreMQTT** client library and
exposes it as an independent, run-time isolated, access-controlled executable
for Microcontrollers (Arm Cortex-M / RISC-V) running under the embedded
Microservice Runtime (eMR).

## Overview

This Microservice provides a lightweight, session-based MQTT client. It receives
requests over IPC, parses them, calls the coreMQTT API, and responds back to the
caller. The TLS transport used underneath is provided by the eMR as a
**Universal Resource**.

### Credits & License (IMPORTANT)

> This Microservice is built on top of the open-source **coreMQTT** library.
>
> - **Owner / Account**: FreeRTOS (Amazon Web Services)
> - **Repository**: https://github.com/FreeRTOS/coreMQTT
> - **Pinned Commit**: `04845c6`
> - **License**: MIT License
> - **Website**: https://www.freertos.org/mqtt/index.html
>
> **All credit for the coreMQTT library goes to FreeRTOS / Amazon Web Services
> and its contributors.** This project only wraps their work into an embedded
> Microservice; it does not claim ownership of the coreMQTT source.

## How to Build and Run
 - Clone the open-source library and initialise the repository using `make init_repo`
    - It will clone coreMQTT (see the version in the `Libs/libraries.txt`). It also clones mbedtls for the simulator projects to simulate the `TLS Universal Resource`
 - Copy the required `CPU Core SDK` from Microservice Store to `Environment/CPU/`.
 - Generate a config file under `Configurations\`
 - Create a Microservice Store Package using `make package CONFIG=<CONFIG_NAME>`

## Running Test Simulator
You dont need and embedded target to run the tests. There is a simulator config where you can run the Microservice and a Test Application in the development environment (Linux). Test Application tries to connect to `https://test.mosquitto.org` using the Microservice.

   ```
        host        = "test.mosquitto.org";
        clientId    = "usCoreMqttSmokeTest";
        topic       = "us/coreMQTT/test";
        payload     = "hello-from-microservice";
        port        = 8883;
   ```

### Creating Test TLS Certificates
 - Download the Root CA certificate from: `https://test.mosquitto.org/ssl/mosquitto.org.crt`, save as `8020FFFF` to the repository's ROOT directory.
 - Follow `https://test.mosquitto.org/ssl/index.php` and create the private key(PKEY) and Device Certificate.
   - Save the Device Certificate as `8021FFFF` to the repository's ROOT directory.
   - Save the Private as `8022FFFF` to the repository's ROOT directory.

### Run The Simulator
 > make simulator CONFIG=sim

You should see the test results

```
$ make simulator CONFIG=sim

Running simulator (timeout 1s)...
========================================

   0 > coreMQTT Microservice. Version : 1.0
   1 > Container : Microservice Test User App
   1 > Test Connect    : expected 0, actual 0 -> SUCCESS
   1 > Test Subscribe  : expected 0, actual 0 -> SUCCESS
   1 > Test Publish    : expected 0, actual 0 -> SUCCESS
   1 > Test ProcessLoop: expected 0, actual 0 -> SUCCESS
   1 > Test Disconnect : expected 0, actual 0 -> SUCCESS
   1 > Test NoSession  : expected 4, actual 4 -> SUCCESS
   1 > usTest : SUCCESS

========================================
Simulator run complete.

```

### Configurations
Please see `Configurations/sim.config` for the configurations.
