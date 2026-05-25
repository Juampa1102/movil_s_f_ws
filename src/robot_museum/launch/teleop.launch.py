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

    teleop_node = Node(
        package='teleop_twist_joy',
        executable='teleop_node',
        name='teleop_twist_joy_node',
        parameters=[config],
        remappings=[('/cmd_vel', '/sumo/cmd_vel')],
        output='screen'
    )

    return LaunchDescription([
        joy_node,
        teleop_node
    ])
