"""
soccer_odom.launch.py
=====================
Levanta el stack completo de odometría para el robot futbol con micro-ROS:

  1. micro_ros_agent  — escucha en UDP puerto 8888, acepta conexiones del ESP32
  2. robot_state_publisher — publica TF base_footprint → base_link (y joints fijos)
                             usando el URDF mínimo soccer_robot_odom.urdf
  3. rviz2            — visualización configurada con el display de odometría

Uso:
  ros2 launch soccer_robot_bringup soccer_odom.launch.py

Parámetros opcionales:
  agent_port:=8888          Puerto UDP del agente (debe coincidir con AGENT_PORT en el ESP32)
  rviz_config:=<path>       Path alternativo al archivo .rviz
"""

import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, TimerAction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    pkg = get_package_share_directory('soccer_robot_bringup')

    urdf_file = os.path.join(pkg, 'urdf', 'soccer_robot_odom.urdf')
    rviz_config = os.path.join(pkg, 'config', 'soccer_odom.rviz')

    with open(urdf_file, 'r') as f:
        robot_description = f.read()

    # -------------------------------------------------------
    # Argumento para el puerto del agente
    # -------------------------------------------------------
    arg_port = DeclareLaunchArgument(
        'agent_port',
        default_value='8888',
        description='Puerto UDP del micro-ROS agent'
    )

    # -------------------------------------------------------
    # micro_ros_agent — UDP
    # Instalación: sudo apt install ros-humble-micro-ros-agent
    #   o vía snap: snap install micro-ros-agent
    # -------------------------------------------------------
    micro_ros_agent = ExecuteProcess(
        cmd=[
            'ros2', 'run', 'micro_ros_agent', 'micro_ros_agent',
            'udp4',
            '--port', LaunchConfiguration('agent_port'),
            '-v6'
        ],
        output='screen',
        name='micro_ros_agent'
    )

    # -------------------------------------------------------
    # robot_state_publisher
    # Publica la TF estática: base_footprint → base_link
    # La TF dinámica: odom → base_footprint la publica el ESP32
    # -------------------------------------------------------
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': robot_description,
            'use_sim_time': False,
            'publish_frequency': 50.0,
        }]
    )

    # -------------------------------------------------------
    # RViz2 — con configuración prearmada
    # Se lanza con delay de 3s para que el agente esté listo
    # -------------------------------------------------------
    rviz = TimerAction(
        period=3.0,
        actions=[
            Node(
                package='rviz2',
                executable='rviz2',
                name='rviz2',
                output='screen',
                arguments=['-d', rviz_config] if os.path.exists(rviz_config) else [],
            )
        ]
    )

    return LaunchDescription([
        arg_port,
        micro_ros_agent,
        robot_state_publisher,
        rviz,
    ])
