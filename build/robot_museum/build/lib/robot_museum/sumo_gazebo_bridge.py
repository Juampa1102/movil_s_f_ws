import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist

class SumoGazeboBridge(Node):
    def __init__(self):
        super().__init__("sumo_gazebo_bridge")
        self.sub = self.create_subscription(
            Twist, "/sumo/cmd_vel_real",
            self.callback, 10)
        self.pub = self.create_publisher(
            Twist, "/sumo/cmd_vel", 10)
        self.get_logger().info("Sumo Gazebo Bridge activo")

    def callback(self, msg):
        self.pub.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = SumoGazeboBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
