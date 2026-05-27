# src/robot_museum/launch/futbol_teleop.launch.py

from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    config = os.path.join(
        get_package_share_directory('robot_museum'),
        'config',
        'joystick.yaml'
    )

    joy_node = Node(
        package='joy',
        executable='joy_node',
        name='joy_node',
        parameters=[config],
        output='screen'
    )

    futbol_controller = Node(
        package='robot_museum',
        executable='futbol_controller',
        name='futbol_controller',
        output='screen'
    )

    return LaunchDescription([
        joy_node,
        futbol_controller,
    ])
