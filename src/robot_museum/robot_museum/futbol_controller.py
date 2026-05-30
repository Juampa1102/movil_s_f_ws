# src/robot_museum/robot_museum/futbol_controller.py

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy
from geometry_msgs.msg import Twist
from std_msgs.msg import Bool

class FutbolController(Node):
    def __init__(self):
        super().__init__("futbol_controller")
        self.enabled = False
        self.prev_button_a = 0

        self.joy_sub = self.create_subscription(Joy, "/joy", self.joy_callback, 10)
        self.cmd_pub = self.create_publisher(Twist, "/futbol/cmd_vel", 10)
        self.servo_der_pub = self.create_publisher(Bool, "/futbol/servo_der", 10)
        self.servo_izq_pub = self.create_publisher(Bool, "/futbol/servo_izq", 10)

        self.get_logger().info("Futbol controller listo. Presiona A para habilitar.")

    def joy_callback(self, msg):
        # Toggle con botón A (índice 0)
        button_a = msg.buttons[0]
        if button_a == 1 and self.prev_button_a == 0:
            self.enabled = not self.enabled
            estado = "HABILITADO" if self.enabled else "DESHABILITADO"
            self.get_logger().info(f"Movimiento {estado}")
        self.prev_button_a = button_a

        # Movimiento — solo si está habilitado
        twist = Twist()
        if self.enabled:
            twist.linear.x  = msg.axes[1] * 2.5    # stick izq Y → adelante/atrás (escala aumentada, sin inversión)
            twist.angular.z = msg.axes[0] * -0.8    # stick izq X → giro (escala reducida para suavidad y control)
        self.cmd_pub.publish(twist)

        # Servo izquierdo — botón L1 (índice 6)
        msg_izq = Bool()
        msg_izq.data = bool(msg.buttons[6])
        self.servo_izq_pub.publish(msg_izq)

        # Servo derecho — botón R1 (índice 7)
        msg_der = Bool()
        msg_der.data = bool(msg.buttons[7])
        self.servo_der_pub.publish(msg_der)

def main(args=None):
    rclpy.init(args=args)
    node = FutbolController()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
