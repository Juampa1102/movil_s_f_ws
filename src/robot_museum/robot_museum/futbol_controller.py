import rclpy
from std_msgs.msg import Bool
from rclpy.node import Node
from sensor_msgs.msg import Joy
from geometry_msgs.msg import Twist
from std_msgs.msg import Float64

class FutbolController(Node):
    def __init__(self):
        super().__init__("futbol_controller")
        self.enabled = False
        self.prev_button_a = 0

        self.joy_sub = self.create_subscription(Joy, "/joy", self.joy_callback, 10)
        self.cmd_pub = self.create_publisher(Twist, "/futbol/cmd_vel", 10)
        self.servoder_pub = self.create_publisher(Float64, "/futbol/servoder_position", 10)
        self.servoizq_pub = self.create_publisher(Float64, "/futbol/servoizq_position", 10)

        self.get_logger().info("Futbol controller iniciado. Presiona A para habilitar movimiento.")

    def joy_callback(self, msg):
        # Toggle con boton A (0) — deteccion de flanco
        button_a = msg.buttons[0]
        if button_a == 1 and self.prev_button_a == 0:
            self.enabled = not self.enabled
            estado = "HABILITADO" if self.enabled else "DESHABILITADO"
            self.get_logger().info(f"Movimiento {estado}")
        self.prev_button_a = button_a

        # Movimiento solo si esta habilitado
        twist = Twist()
        if self.enabled:
            twist.linear.x = msg.axes[1] * 1.0   # stick izq Y
            twist.angular.z = msg.axes[0] * -0.9  # stick izq X
        self.cmd_pub.publish(twist)

        # Servo izquierdo — boton L1 (6)
        if msg.buttons[6] == 1:
            pos = Float64()
            pos.data = 1.5707
            self.servoizq_pub.publish(pos)
        else:
            pos = Float64()
            pos.data = 0.0
            self.servoizq_pub.publish(pos)

        # Servo derecho — boton R1 (7)
        if msg.buttons[7] == 1:
            pos = Float64()
            pos.data = 1.5707
            self.servoder_pub.publish(pos)
        else:
            pos = Float64()
            pos.data = 0.0
            self.servoder_pub.publish(pos)

def main(args=None):
    rclpy.init(args=args)
    node = FutbolController()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
