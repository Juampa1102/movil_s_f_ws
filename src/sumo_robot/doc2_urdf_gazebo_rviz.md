# Configuración del URDF del Robot Sumo en Gazebo y RViz

**Proyecto:** Robot Móvil Museo — movil_s_f_ws  
**Fecha:** Mayo 2026  
**Sistema:** Ubuntu 22.04 / ROS2 Humble / Gazebo Classic 11.10.2

---

## 1. Origen del URDF

El URDF fue exportado desde SolidWorks usando el plugin sw_urdf_exporter. El paquete original tenía los siguientes problemas que requirieron corrección:

| Problema | Descripción |
|---------|-------------|
| Launch files ROS1 | Los archivos .launch usaban sintaxis de ROS1 (rostopic, tf, gazebo_ros ROS1) |
| Nombre del paquete con puntos | `UDRF_sumo.final.SLDASM` — ROS2 no maneja puntos en nombres de paquetes |
| Sin plugin diff_drive | Solo tenía geometría, sin capacidad de movimiento |
| RPY incorrecto en rueda derecha | SolidWorks exportó `rpy="0 0.992 1.5707"` en lugar de `rpy="1.5707 -1.5707 0"` |
| CMakeLists.txt y package.xml de ROS1 | Incompatibles con ament_cmake de ROS2 |
| Sin declaración de encoding correcta | La línea `encoding="utf-8"` causaba error en Gazebo al parsear XML |

---

## 2. Estructura del paquete corregido

```
movil_s_f_ws/src/sumo_robot/
├── CMakeLists.txt
├── package.xml
├── config/
│   └── joint_names_sumo.yaml
├── launch/
│   ├── sumo_gazebo_only.launch.py
│   └── sumo_rviz.launch.py
├── meshes/
│   ├── base_link.STL
│   ├── Rueda_Derecha_Link.STL
│   └── Rueda_Izquierda_Link.STL
└── urdf/
    └── sumo_robot.urdf
```

---

## 3. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.8)
project(sumo_robot)

find_package(ament_cmake REQUIRED)

install(
  DIRECTORY urdf meshes launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
```

---

## 4. package.xml

```xml
<?xml version="1.0"?>
<package format="3">
  <name>sumo_robot</name>
  <version>0.0.1</version>
  <description>URDF del robot sumo para simulacion en ROS2 Humble</description>
  <maintainer email="tu@email.com">tu_nombre</maintainer>
  <license>MIT</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <exec_depend>robot_state_publisher</exec_depend>
  <exec_depend>joint_state_publisher</exec_depend>
  <exec_depend>gazebo_ros</exec_depend>
  <exec_depend>rviz2</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
```

---

## 5. URDF sumo_robot.urdf

Correcciones aplicadas sobre el original de SolidWorks:

- Eliminada la declaración `encoding="utf-8"` (causa error en Gazebo)
- Agregado link `base_footprint` como raíz para corrección de orientación
- Corregido RPY de la rueda derecha
- Agregado plugin `libgazebo_ros_diff_drive.so`
- Agregados materiales para Gazebo (`Gazebo/White`, `Gazebo/Black`)
- Ajustados parámetros de fricción y contacto de ruedas

### Corrección de orientación — base_footprint

SolidWorks exporta con el eje Y hacia arriba. ROS2 usa Z hacia arriba. La solución fue agregar un link raíz `base_footprint` con la rotación de corrección sin tocar las inercias ni las posiciones de los joints:

```xml
<link name="base_footprint"/>

<joint name="base_footprint_joint" type="fixed">
  <parent link="base_footprint"/>
  <child link="base_link"/>
  <origin xyz="0 0 0.081" rpy="1.5707963 0 0"/>
</joint>
```

El valor `z="0.081"` eleva el robot para que las ruedas toquen el plano. Fue determinado iterativamente.

### Plugin diff_drive

```xml
<gazebo>
  <plugin name="diff_drive_controller" filename="libgazebo_ros_diff_drive.so">
    <ros>
      <namespace>/sumo</namespace>
      <remapping>cmd_vel:=cmd_vel</remapping>
      <remapping>odom:=odom</remapping>
    </ros>
    <left_joint>Rueda_Izquierda_joint</left_joint>
    <right_joint>Rueda_Derecha_joint</right_joint>
    <wheel_separation>0.0714</wheel_separation>
    <wheel_diameter>0.042</wheel_diameter>
    <max_wheel_torque>50</max_wheel_torque>
    <max_wheel_acceleration>20.0</max_wheel_acceleration>
    <publish_odom>true</publish_odom>
    <publish_odom_tf>true</publish_odom_tf>
    <publish_wheel_tf>true</publish_wheel_tf>
    <odometry_frame>odom</odometry_frame>
    <robot_base_frame>base_footprint</robot_base_frame>
  </plugin>
</gazebo>
```

**Parámetros físicos del robot:**
- `wheel_separation`: 0.0714 m (tomado del URDF, distancia entre centros de ruedas)
- `wheel_diameter`: 0.042 m (medido del robot físico)

### Fricción de ruedas

```xml
<gazebo reference="Rueda_Izquierda_Link">
  <mu1>2.0</mu1>
  <mu2>2.0</mu2>
  <kp>1000000.0</kp>
  <kd>1.0</kd>
  <minDepth>0.001</minDepth>
</gazebo>
```

Los valores `mu1=2.0` y `mu2=2.0` fueron necesarios para evitar deslizamiento. El `kp=1000000` garantiza contacto rígido con el suelo.

### Materiales

```xml
<gazebo reference="base_link">
  <material>Gazebo/White</material>
</gazebo>
<gazebo reference="Rueda_Izquierda_Link">
  <material>Gazebo/Black</material>
</gazebo>
<gazebo reference="Rueda_Derecha_Link">
  <material>Gazebo/Black</material>
</gazebo>
```

---

## 6. Launch files

### sumo_rviz.launch.py — Solo RViz

```python
import os
from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory

def generate_launch_description():
    pkg_sumo = get_package_share_directory('sumo_robot')
    urdf_file = os.path.join(pkg_sumo, 'urdf', 'sumo_robot.urdf')

    with open(urdf_file, 'r') as f:
        robot_description = f.read()

    return LaunchDescription([
        Node(package='robot_state_publisher', executable='robot_state_publisher',
             parameters=[{'robot_description': robot_description}], output='screen'),
        Node(package='joint_state_publisher', executable='joint_state_publisher', output='screen'),
        Node(package='rviz2', executable='rviz2', output='screen'),
    ])
```

### sumo_gazebo_only.launch.py — Solo Gazebo

Incluye un `TimerAction` de 5 segundos antes del spawn para que Gazebo esté completamente cargado antes de intentar spawnear el robot.

```python
spawn_robot = TimerAction(
    period=5.0,
    actions=[
        Node(
            package='gazebo_ros',
            executable='spawn_entity.py',
            arguments=['-file', urdf_file, '-entity', 'sumo_robot',
                       '-x', '0.0', '-y', '0.0', '-z', '0.05'],
            output='screen'
        )
    ]
)
```

---

## 7. Configuración de variables de entorno

Necesarias para que Gazebo encuentre los meshes STL del paquete:

```bash
export GAZEBO_MODEL_PATH=$GAZEBO_MODEL_PATH:~/movil_s_f_ws/install/sumo_robot/share
export GAZEBO_RESOURCE_PATH=$GAZEBO_RESOURCE_PATH:/home/juampa/movil_s_f_ws/install/sumo_robot/share/sumo_robot
```

Agregadas al `~/.bashrc` para persistir entre sesiones.

---

## 8. Copia de meshes a ~/.gazebo/models

Gazebo Classic requiere que los modelos estén registrados en `~/.gazebo/models` para encontrar los meshes STL con rutas `package://`:

```bash
mkdir -p ~/.gazebo/models/sumo_robot/meshes
cp ~/movil_s_f_ws/install/sumo_robot/share/sumo_robot/meshes/*.STL \
   ~/.gazebo/models/sumo_robot/meshes/
cp ~/movil_s_f_ws/install/sumo_robot/share/sumo_robot/urdf/sumo_robot.urdf \
   ~/.gazebo/models/sumo_robot/
```

Archivo `model.config` requerido por Gazebo:

```xml
<?xml version="1.0"?>
<model>
  <name>sumo_robot</name>
  <version>1.0</version>
  <sdf version="1.6">sumo_robot.urdf</sdf>
  <description>Robot sumo</description>
</model>
```

**Importante:** Cada vez que se modifica el URDF, se debe copiar nuevamente a `~/.gazebo/models/sumo_robot/`.

---

## 9. Dependencias a instalar

```bash
sudo apt install ros-humble-gazebo-ros-pkgs \
                 ros-humble-robot-state-publisher \
                 ros-humble-joint-state-publisher \
                 ros-humble-joint-state-publisher-gui -y
pip install lxml --break-system-packages
```

---

## 10. Problemas encontrados y soluciones

| Problema | Causa | Solución |
|---------|-------|----------|
| Robot volcado en RViz | SolidWorks exportó con Y arriba, ROS usa Z arriba | Agregar base_footprint con rpy="1.5707963 0 0" |
| Ruedas no visibles en RViz | joint_state_publisher no estaba en el launch | Agregar joint_state_publisher al launch file |
| Robot invisible en Gazebo | Meshes STL no encontrados por Gazebo | Copiar a ~/.gazebo/models y configurar GAZEBO_MODEL_PATH |
| gzserver crash con código -11 | Dos instancias del plugin diff_drive con mismo nombre | No spawnear el robot dos veces |
| Robot no se mueve en Gazebo | Plugin diff_drive no presente en URDF original | Agregar plugin libgazebo_ros_diff_drive.so |
| Ruedas se deslizan | mu1/mu2 demasiado bajos | Aumentar a mu1=mu2=2.0 con kp=1000000 |
| Error encoding="utf-8" al spawnear | lxml no acepta declaration Unicode | Eliminar encoding="utf-8" de la primera línea del URDF |
