// ============================================================
//  MODO FUTBOL — ESP32 + micro-ROS
//  Publica:  /futbol/odom  (nav_msgs/Odometry)
//  Suscribe: /futbol/cmd_vel    (geometry_msgs/Twist)
//            /futbol/servo_der  (std_msgs/Bool)
//            /futbol/servo_izq  (std_msgs/Bool)
// ============================================================

#include <Arduino.h>
#include <micro_ros_platformio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <nav_msgs/msg/odometry.h>
#include <geometry_msgs/msg/twist.h>
#include <std_msgs/msg/bool.h>
#include <ESP32Encoder.h>
#include <math.h>

// ============================================================
// PINES MOTORES
// ============================================================

#define AIN1 27
#define AIN2 26
#define PWMA 25
#define BIN1 15
#define BIN2 13
#define PWMB 14
#define STBY 5

// ============================================================
// PINES SERVOS
// ============================================================

#define PIN_SERVO_1 19
#define PIN_SERVO_2 18

#define SERVO1_REPOSO 100
#define SERVO1_GOLPE  10
#define SERVO2_REPOSO 10
#define SERVO2_GOLPE  100
#define TIEMPO_GOLPE  120

// ============================================================
// PWM
// ============================================================

#define CH_A       0
#define CH_B       1
#define CH_SERVO1  2
#define CH_SERVO2  3

#define FREQ_MOTOR  5000
#define RES_MOTOR   8
#define FREQ_SERVO  50
#define RES_SERVO   16

#define VEL_MAX 200

// ============================================================
// ODOMETRÍA
// ============================================================

#define TICKS_POR_VUELTA   210.0f   // 7 PPR * 30 reduccion
#define DIAMETRO_RUEDA     0.042f   // metros
#define DISTANCIA_RUEDAS   0.093f   // metros

#define RADIO_RUEDA (DIAMETRO_RUEDA / 2.0f)

// ============================================================
// MICRO-ROS
// ============================================================

#define WIFI_SSID "RENGIFO.VARELA_5_Ghz"
#define WIFI_PASS "BMWX3CH3"
#define AGENT_IP  "192.168.1.11"
#define AGENT_PORT 8888

// ============================================================
// VARIABLES GLOBALES
// ============================================================

// Encoders
ESP32Encoder encoderIzq;
ESP32Encoder encoderDer;

int64_t ticksIzqPrev = 0;
int64_t ticksDerPrev = 0;

// Odometría
float x   = 0.0f;
float y   = 0.0f;
float yaw = 0.0f;

// Servos
bool servo1_activo = false;
bool servo2_activo = false;
unsigned long t_servo1 = 0;
unsigned long t_servo2 = 0;

// micro-ROS
rcl_node_t node;
rcl_allocator_t allocator;
rclc_support_t support;
rclc_executor_t executor;

rcl_publisher_t odom_pub;
rcl_subscription_t cmd_vel_sub;
rcl_subscription_t servo_der_sub;
rcl_subscription_t servo_izq_sub;

nav_msgs__msg__Odometry odom_msg;
geometry_msgs__msg__Twist cmd_vel_msg;
std_msgs__msg__Bool servo_der_msg;
std_msgs__msg__Bool servo_izq_msg;

rcl_timer_t odom_timer;

// ============================================================
// PWM SERVO MANUAL
// ============================================================

void writeServo(uint8_t channel, int angle) {
    angle = constrain(angle, 0, 180);
    int pulse_us = map(angle, 0, 180, 500, 2500);
    uint32_t duty = (pulse_us * 65535UL) / 20000UL;
    ledcWrite(channel, duty);
}

// ============================================================
// MOTORES
// ============================================================

void moverMotores(float linear, float angular) {
    float velIzq = linear - angular * DISTANCIA_RUEDAS / 2.0f;
    float velDer = linear + angular * DISTANCIA_RUEDAS / 2.0f;

    float maxVel = max(abs(velIzq), abs(velDer));
    if (maxVel > 1.0f) {
        velIzq /= maxVel;
        velDer /= maxVel;
    }

    int pwmIzq = (int)(abs(velIzq) * VEL_MAX);
    int pwmDer = (int)(abs(velDer) * VEL_MAX);

    // Motor izquierdo (Motor A)
    if (velIzq > 0.05f) {
        digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    } else if (velIzq < -0.05f) {
        digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
    } else {
        digitalWrite(AIN1, LOW);  digitalWrite(AIN2, LOW);
    }

    // Motor derecho (Motor B)
    if (velDer > 0.05f) {
        digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
    } else if (velDer < -0.05f) {
        digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
    } else {
        digitalWrite(BIN1, LOW);  digitalWrite(BIN2, LOW);
    }

    ledcWrite(CH_A, pwmIzq);
    ledcWrite(CH_B, pwmDer);
}

void detenerMotores() {
    ledcWrite(CH_A, 0);
    ledcWrite(CH_B, 0);
    digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
    digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

// ============================================================
// CALLBACKS micro-ROS
// ============================================================

void cmd_vel_callback(const void* msg_in) {
    const geometry_msgs__msg__Twist* msg =
        (const geometry_msgs__msg__Twist*)msg_in;
    moverMotores(msg->linear.x, msg->angular.z);
}

void servo_der_callback(const void* msg_in) {
    const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*)msg_in;
    if (msg->data && !servo2_activo) {
        writeServo(CH_SERVO2, SERVO2_GOLPE);
        servo2_activo = true;
        t_servo2 = millis();
    }
}

void servo_izq_callback(const void* msg_in) {
    const std_msgs__msg__Bool* msg = (const std_msgs__msg__Bool*)msg_in;
    if (msg->data && !servo1_activo) {
        writeServo(CH_SERVO1, SERVO1_GOLPE);
        servo1_activo = true;
        t_servo1 = millis();
    }
}

// ============================================================
// TIMER ODOMETRÍA — cada 50ms
// ============================================================

void odom_callback(rcl_timer_t* timer, int64_t last_call_time) {
    (void)last_call_time;
    if (timer == NULL) return;

    int64_t ticksIzq = encoderIzq.getCount();
    int64_t ticksDer = encoderDer.getCount();

    int64_t dIzq = ticksIzq - ticksIzqPrev;
    int64_t dDer = ticksDer - ticksDerPrev;

    ticksIzqPrev = ticksIzq;
    ticksDerPrev = ticksDer;

    float distIzq = (dIzq / TICKS_POR_VUELTA) * 2.0f * M_PI * RADIO_RUEDA;
    float distDer = (dDer / TICKS_POR_VUELTA) * 2.0f * M_PI * RADIO_RUEDA;

    float distCenter = (distIzq + distDer) / 2.0f;
    float deltaYaw   = (distDer - distIzq) / DISTANCIA_RUEDAS;

    x   += distCenter * cos(yaw + deltaYaw / 2.0f);
    y   += distCenter * sin(yaw + deltaYaw / 2.0f);
    yaw += deltaYaw;

    // Publicar odometría
    odom_msg.pose.pose.position.x = x;
    odom_msg.pose.pose.position.y = y;

    // Quaternion desde yaw
    odom_msg.pose.pose.orientation.z = sin(yaw / 2.0f);
    odom_msg.pose.pose.orientation.w = cos(yaw / 2.0f);

    rcl_publish(&odom_pub, &odom_msg, NULL);
}

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);

    // LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // TB6612
    pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
    pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
    pinMode(STBY, OUTPUT);
    digitalWrite(STBY, HIGH);

    // PWM motores
    ledcSetup(CH_A, FREQ_MOTOR, RES_MOTOR);
    ledcAttachPin(PWMA, CH_A);
    ledcSetup(CH_B, FREQ_MOTOR, RES_MOTOR);
    ledcAttachPin(PWMB, CH_B);

    // PWM servos
    ledcSetup(CH_SERVO1, FREQ_SERVO, RES_SERVO);
    ledcAttachPin(PIN_SERVO_1, CH_SERVO1);
    ledcSetup(CH_SERVO2, FREQ_SERVO, RES_SERVO);
    ledcAttachPin(PIN_SERVO_2, CH_SERVO2);

    writeServo(CH_SERVO1, SERVO1_REPOSO);
    writeServo(CH_SERVO2, SERVO2_REPOSO);

    // Encoders
    ESP32Encoder::useInternalWeakPullResistors = puseUp;
    encoderIzq.attachHalfQuad(36, 39);  // VP, VN
    encoderDer.attachHalfQuad(34, 35);
    encoderIzq.clearCount();
    encoderDer.clearCount();

    // micro-ROS WiFi
    set_microros_wifi_transports(
        WIFI_SSID, WIFI_PASS,
        AGENT_IP, AGENT_PORT
    );

    delay(2000);

    allocator = rcl_get_default_allocator();
    rclc_support_init(&support, 0, NULL, &allocator);
    rclc_node_init_default(&node, "futbol_esp32", "futbol", &support);

    // Publisher odometría
    rclc_publisher_init_default(
        &odom_pub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry),
        "odom"
    );

    // Subscribers
    rclc_subscription_init_default(
        &cmd_vel_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "cmd_vel"
    );
    rclc_subscription_init_default(
        &servo_der_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
        "servo_der"
    );
    rclc_subscription_init_default(
        &servo_izq_sub, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
        "servo_izq"
    );

    // Timer odometría cada 50ms
    rclc_timer_init_default(
        &odom_timer, &support,
        RCL_MS_TO_NS(50),
        odom_callback
    );

    // Executor — 4 handles: 3 subs + 1 timer
    rclc_executor_init(&executor, &support.context, 4, &allocator);
    rclc_executor_add_subscription(&executor, &cmd_vel_sub,
        &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);
    rclc_executor_add_subscription(&executor, &servo_der_sub,
        &servo_der_msg, &servo_der_callback, ON_NEW_DATA);
    rclc_executor_add_subscription(&executor, &servo_izq_sub,
        &servo_izq_msg, &servo_izq_callback, ON_NEW_DATA);
    rclc_executor_add_timer(&executor, &odom_timer);

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("=== FUTBOL ESP32 micro-ROS listo ===");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    // Retorno automático servos
    unsigned long ahora = millis();

    if (servo1_activo && (ahora - t_servo1 >= TIEMPO_GOLPE)) {
        writeServo(CH_SERVO1, SERVO1_REPOSO);
        servo1_activo = false;
    }
    if (servo2_activo && (ahora - t_servo2 >= TIEMPO_GOLPE)) {
        writeServo(CH_SERVO2, SERVO2_REPOSO);
        servo2_activo = false;
    }

    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
    delay(10);
}
