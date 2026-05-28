// ============================================================
//  MODO SUMO — ESP32 + micro-ROS
//  Publica:  /sumo/sensor/vl53  (sensor_msgs/Range) x3
//            /sumo/cmd_vel_real (geometry_msgs/Twist)
//  Suscribe: nada — lógica autónoma en ESP32
//
//  Máquina de estados:
//    BUSCAR   → gira horario hasta detectar oponente
//    CORREGIR → centra sensor del medio sobre oponente
//    ATACAR   → avanza a máxima velocidad
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <sensor_msgs/msg/range.h>
#include <Adafruit_VL53L0X.h>

// ============================================================
// PINES MOTORES — mismo que fútbol
// ============================================================
#define AIN1 27
#define AIN2 26
#define PWMA 25
#define BIN1 15
#define BIN2 13
#define PWMB 14
#define STBY 5

// ============================================================
// PWM MOTORES
// ============================================================
#define CH_A 0
#define CH_B 1
#define FREQ_MOTOR 5000
#define RES_MOTOR  8
#define VEL_MAX    200
#define VEL_BUSCAR 120
#define VEL_ATACAR 200

// ============================================================
// SENSORES VL53L0X
// ============================================================
#define DIST_DETECCION 400   // mm
#define TCA_ADDR       0x70

Adafruit_VL53L0X lox0;
Adafruit_VL53L0X lox1;
Adafruit_VL53L0X lox2;

uint16_t dist[3] = {9999, 9999, 9999};

// ============================================================
// MICRO-ROS
// ============================================================
#define WIFI_SSID   "juanpa"
#define WIFI_PASS   "juampa312"
#define AGENT_PORT  8888

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;

rcl_publisher_t pub_vl53_0;
rcl_publisher_t pub_vl53_1;
rcl_publisher_t pub_vl53_2;
rcl_publisher_t pub_cmd;

sensor_msgs__msg__Range msg_vl53_0;
sensor_msgs__msg__Range msg_vl53_1;
sensor_msgs__msg__Range msg_vl53_2;
geometry_msgs__msg__Twist msg_cmd;

rcl_timer_t sensor_timer;

// ============================================================
// MÁQUINA DE ESTADOS
// ============================================================
typedef enum { BUSCAR, CORREGIR, ATACAR } Estado;
Estado estado = BUSCAR;

// ============================================================
// TCA9548A
// ============================================================
void TCA9548A(uint8_t bus) {
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(1 << bus);
    Wire.endTransmission();
    delay(5);
}

// ============================================================
// MOTORES
// ============================================================
void moverMotores(int pwmA, int pwmB, bool dirA, bool dirB) {
    digitalWrite(AIN1, dirA ? HIGH : LOW);
    digitalWrite(AIN2, dirA ? LOW  : HIGH);
    digitalWrite(BIN1, dirB ? HIGH : LOW);
    digitalWrite(BIN2, dirB ? LOW  : HIGH);
    ledcWrite(CH_A, pwmA);
    ledcWrite(CH_B, pwmB);
}

void detenerMotores() {
    ledcWrite(CH_A, 0);
    ledcWrite(CH_B, 0);
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

void girarHorario(int vel) {
    // Motor A adelante, Motor B atrás
    moverMotores(vel, vel, true, false);
}

void girarAntihorario(int vel) {
    moverMotores(vel, vel, false, true);
}

void avanzar(int vel) {
    moverMotores(vel, vel, true, true);
}

// ============================================================
// LEER SENSORES
// ============================================================
void leerSensores() {
    VL53L0X_RangingMeasurementData_t measure;

    TCA9548A(0);
    lox0.rangingTest(&measure, false);
    dist[0] = (measure.RangeStatus != 4) ? measure.RangeMilliMeter : 9999;

    TCA9548A(1);
    lox1.rangingTest(&measure, false);
    dist[1] = (measure.RangeStatus != 4) ? measure.RangeMilliMeter : 9999;

    TCA9548A(2);
    lox2.rangingTest(&measure, false);
    dist[2] = (measure.RangeStatus != 4) ? measure.RangeMilliMeter : 9999;
}

// ============================================================
// LÓGICA DE SUMO
// ============================================================
void ejecutarLogica() {
    bool det0 = dist[0] < DIST_DETECCION;  // izquierdo
    bool det1 = dist[1] < DIST_DETECCION;  // centro
    bool det2 = dist[2] < DIST_DETECCION;  // derecho

    switch (estado) {

        case BUSCAR:
            girarHorario(VEL_BUSCAR);
            if (det1) {
                estado = ATACAR;
            } else if (det0 || det2) {
                estado = CORREGIR;
            }
            break;

        case CORREGIR:
            if (det1) {
                estado = ATACAR;
            } else if (!det0 && !det1 && !det2) {
                estado = BUSCAR;
            } else if (det0 && !det2) {
                // Oponente a la izquierda — girar antihorario
                girarAntihorario(VEL_BUSCAR);
            } else if (det2 && !det0) {
                // Oponente a la derecha — girar horario
                girarHorario(VEL_BUSCAR);
            }
            break;

        case ATACAR:
            avanzar(VEL_ATACAR);
            if (!det0 && !det1 && !det2) {
                // Perdió al oponente
                estado = BUSCAR;
            }
            break;
    }
}

// ============================================================
// TIMER — sensores + publicación + lógica cada 100ms
// ============================================================
void sensor_callback(rcl_timer_t* timer, int64_t last_call_time) {
    (void)last_call_time;
    if (timer == NULL) return;

    leerSensores();
    ejecutarLogica();

    // Publicar sensores
    msg_vl53_0.range = dist[0] / 1000.0f;
    msg_vl53_1.range = dist[1] / 1000.0f;
    msg_vl53_2.range = dist[2] / 1000.0f;

    rcl_publish(&pub_vl53_0, &msg_vl53_0, NULL);
    rcl_publish(&pub_vl53_1, &msg_vl53_1, NULL);
    rcl_publish(&pub_vl53_2, &msg_vl53_2, NULL);

    // Publicar cmd_vel para que Gazebo replique el movimiento
    switch (estado) {
        case BUSCAR:
            msg_cmd.linear.x  = 0.0f;
            msg_cmd.angular.z = -1.5f;  // giro horario
            break;
        case CORREGIR:
            msg_cmd.linear.x  = 0.0f;
            msg_cmd.angular.z = (dist[0] < dist[2]) ? 1.5f : -1.5f;
            break;
        case ATACAR:
            msg_cmd.linear.x  = 0.3f;
            msg_cmd.angular.z = 0.0f;
            break;
    }
    rcl_publish(&pub_cmd, &msg_cmd, NULL);
}

// ============================================================
// INIT SENSOR RANGE MSG
// ============================================================
void initRangeMsg(sensor_msgs__msg__Range* msg, const char* frame_id) {
    msg->radiation_type = sensor_msgs__msg__Range__INFRARED;
    msg->field_of_view  = 0.44f;   // VL53L0X ~25 grados
    msg->min_range      = 0.03f;
    msg->max_range      = 2.0f;
    msg->range          = 0.0f;
    msg->header.frame_id.data     = (char*)frame_id;
    msg->header.frame_id.size     = strlen(frame_id);
    msg->header.frame_id.capacity = strlen(frame_id) + 1;
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // TB6612
    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);

    ledcSetup(CH_A, FREQ_MOTOR, RES_MOTOR);
    ledcAttachPin(PWMA, CH_A);
    ledcSetup(CH_B, FREQ_MOTOR, RES_MOTOR);
    ledcAttachPin(PWMB, CH_B);

    // I2C y sensores
    Wire.begin();

    TCA9548A(0);
    if (!lox0.begin()) Serial.println("Error sensor 0");
    TCA9548A(1);
    if (!lox1.begin()) Serial.println("Error sensor 1");
    TCA9548A(2);
    if (!lox2.begin()) Serial.println("Error sensor 2");

    Serial.println("Sensores VL53L0X listos");

    // micro-ROS WiFi
    set_microros_wifi_transports(
        (char*)WIFI_SSID, (char*)WIFI_PASS,
        IPAddress(10,51,219,200), AGENT_PORT
    );

    delay(2000);

    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "sumo_esp32", "sumo", &support);

    // Publishers
    rclc_publisher_init_default(&pub_vl53_0, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range), "sensor/vl53_0");
    rclc_publisher_init_default(&pub_vl53_1, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range), "sensor/vl53_1");
    rclc_publisher_init_default(&pub_vl53_2, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range), "sensor/vl53_2");
    rclc_publisher_init_default(&pub_cmd, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist), "cmd_vel_real");

    // Inicializar mensajes Range
    initRangeMsg(&msg_vl53_0, "vl53_izq");
    initRangeMsg(&msg_vl53_1, "vl53_centro");
    initRangeMsg(&msg_vl53_2, "vl53_der");

    // Timer cada 100ms
    rclc_timer_init_default(&sensor_timer, &support,
        RCL_MS_TO_NS(100), sensor_callback);

    // Executor — 1 timer
    rclc_executor_init(&executor, &support.context, 1, &allocator);
    rclc_executor_add_timer(&executor, &sensor_timer);

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("=== SUMO ESP32 micro-ROS listo ===");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    delay(10);
}
