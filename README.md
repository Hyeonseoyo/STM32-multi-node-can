# STM32 Multi-Node CAN Communication System

4개의 STM32 MCU를 활용한 레지스터 기반 CAN 통신 및 분산 제어 시스템


## 1. 프로젝트 개요 (Overview)

본 프로젝트는 4개의 STM32 MCU를 CAN 버스로 연결하여 센서 데이터 수집, 제어 판단, 액추에이터 구동 및 상태 모니터링을 수행하는 다중 노드 통신 시스템입니다.

각 노드는 독립적인 역할을 수행하며, CAN 메시지를 통해 데이터를 공유하고 제어 명령을 전달하도록 설계했습니다.

STM32 HAL 라이브러리에 의존하지 않고 Reference Manual을 기반으로 주변장치 레지스터를 직접 설정하여 CAN 통신 및 하드웨어 제어 기능을 구현했습니다.

### 주요 구현 내용

- **4-Node CAN Network:** 4개의 STM32 MCU를 하나의 CAN 버스로 연결하여 노드 간 통신 구현
- **Register-Level Programming:** Reference Manual을 기반으로 주변장치 레지스터 직접 설정
- **Distributed Control:** 센서 데이터 수집, 제어 판단, 액추에이터 구동, 모니터링 기능을 노드별로 분리
- **CAN Message Filtering:** CAN ID 기반 하드웨어 필터를 설정하여 필요한 메시지만 선택적으로 수신
- **Real-Time Monitoring:** OLED 디스플레이를 통해 센서 데이터 및 액추에이터 동작 상태 표시


## 2. 시스템 구성 (System Architecture)

본 시스템은 CAN 버스를 공유하는 4개의 노드로 구성됩니다. 각 노드는 서로 다른 기능을 담당하며 CAN 메시지를 통해 데이터를 송수신합니다.

### 2.1 노드별 역할

| Node | MCU | 역할 | 주요 기능 |
|---|---|---|---|
| Node 1 | STM32F446RE | Actuator | 브레이크 LED 및 냉각 팬 제어 |
| Node 2 | STM32F446RE | Sensor | 온도 측정 및 가변저항 기반 차량 속도 모사 |
| Node 3 | STM32F103C8T6 | Controller | 센서 데이터 분석 및 제어 명령 생성 |
| Node 4 | STM32F103C8T6 | Dashboard | OLED를 통한 센서 데이터 및 액추에이터 상태 표시 |

### 2.2 시스템 동작 흐름

**1. Sensor Node (Node 2)**

- SHT20 센서를 통해 온도 데이터를 측정합니다.
- ADC로 가변저항 값을 읽어 차량 속도(0~200 km/h)를 모사합니다.
- 측정한 온도와 속도를 CAN 메시지로 송신합니다.

**2. Controller Node (Node 3)**

- Node 2에서 송신한 온도 및 속도 데이터를 수신합니다.
- 각 데이터를 사전에 설정한 임계값과 비교합니다.
- 임계값에 따라 브레이크 및 냉각 팬 제어 명령을 생성하고 CAN 메시지로 송신합니다.

**3. Actuator Node (Node 1)**

- Node 3에서 송신한 제어 명령을 수신합니다.
- 브레이크 명령에 따라 LED를 ON/OFF 합니다.
- 냉각 팬 명령에 따라 팬을 구동하거나 정지합니다.
- 액추에이터의 동작 상태를 CAN 메시지로 송신합니다.

**4. Dashboard Node (Node 4)**

- CAN 버스에서 센서 데이터와 액추에이터 상태 메시지를 수신합니다.
- 수신한 데이터를 OLED 디스플레이에 표시합니다.
- 차량 속도, 온도, 브레이크 상태 및 냉각 팬 상태를 확인할 수 있습니다.
