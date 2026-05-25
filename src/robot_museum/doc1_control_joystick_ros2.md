# Configuración del Control Joystick en ROS2 Humble

**Proyecto:** Robot Móvil Museo — movil_s_f_ws  
**Fecha:** Mayo 2026  
**Sistema:** Ubuntu 22.04 / ROS2 Humble

---

## 1. Hardware utilizado

- **Mando:** Zikway HID gamepad
- **Conexión:** USB directo al PC
- **Dispositivo en Linux:** `/dev/input/js0`

---

## 2. Verificación del mando en Linux

Antes de configurar ROS2, se verificó que el sistema operativo reconocía el mando:

```bash
ls /dev/input/js*
# Resultado: /dev/input/js0

cat /proc/bus/input/devices | grep -B 2 "js0" | grep "Name"
# Resultado: N: Name="Zikway HID gamepad"
```

Para ver los ejes y botones en tiempo real:
```bash
jstest /dev/input/js0
```

### Mapeo de ejes confirmado

| Eje | Movimiento | Valor mínimo | Valor máximo |
|-----|-----------|--------------|--------------|
| 0 | Stick izquierdo X (izq/der) | -32767 | 32767 |
| 1 | Stick izquierdo Y (arr/abajo) | -32767 | 32767 |
| 2 | Stick derecho X | -32767 | 32767 |
| 3 | Stick derecho Y | -32767 | 32767 |
| 4 | Gatillo izquierdo | -32767 | 32767 |
| 5 | Gatillo derecho | -32767 | 32767 |

### Botones relevantes

| Botón | Función asignada |
|-------|-----------------|
| 6 (L1) | Enable button — botón de seguridad |
| 7 (R1) | Enable turbo button |

---

## 3. Paquetes ROS2 instalados

```bash
sudo apt install ros-humble-joy ros-humble-teleop-twist-joy -y
```

Verificación:
```bash
ros2 pkg list | grep joy
# Debe aparecer: joy y teleop_twist_joy
```

---

## 4. Estructura del paquete robot_museum

```
movil_s_f_ws/src/robot_museum/
├── config/
│   └── joystick.yaml
├── launch/
│   └── teleop.launch.py
├── resource/
│   └── robot_museum
├── package.xml
└── setup.py
```

Creación del paquete:
```bash
cd ~/movil_s_f_ws/src
ros2 pkg create --build-type ament_python robot_museum \
  --dependencies rclpy geometry_msgs sensor_msgs std_msgs
mkdir -p ~/movil_s_f_ws/src/robot_museum/config
mkdir -p ~/movil_s_f_ws/src/robot_museum/launch
```

---

## 5. Archivo de configuración joystick.yaml

Ubicación: `~/movil_s_f_ws/src/robot_museum/config/joystick.yaml`

```yaml
joy_node:
  ros__parameters:
    device_id: 0
    deadzone: 0.05
    autorepeat_rate: 20.0

teleop_twist_joy_node:
  ros__parameters:
    axis_linear:
      x: 0
    axis_angular:
      yaw: 1
    scale_linear:
      x: 2.0
    scale_angular:
      yaw: -0.8
    scale_linear_turbo:
      x: 4.0
    enable_button: 6
    enable_turbo_button: 7
```

**Notas importantes:**
- `enable_button: 6` corresponde a L1 — el robot solo se mueve si se mantiene presionado L1. Esto es un requisito de seguridad para la exhibición.
- Los valores negativos en `scale_angular` corrigen la inversión de eje del mando Zikway.
- `axis_linear.x: 0` usa el eje X del stick izquierdo para movimiento lineal (adelante/atrás).
- `axis_angular.yaw: 1` usa el eje Y del stick izquierdo para rotación.

---

## 6. Launch file teleop.launch.py

Ubicación: `~/movil_s_f_ws/src/robot_museum/launch/teleop.launch.py`

```python
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
```

**Nota:** El remapping `/cmd_vel` → `/sumo/cmd_vel` conecta el teleop con el namespace del plugin diff_drive del robot sumo.

---

## 7. setup.py

```python
from setuptools import setup
import os
from glob import glob

package_name = 'robot_museum'

setup(
    name=package_name,
    version='0.0.1',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        (os.path.join('share', package_name, 'launch'),
            glob('launch/*.launch.py')),
        (os.path.join('share', package_name, 'config'),
            glob('config/*.yaml')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='tu_nombre',
    maintainer_email='tu@email.com',
    description='Robot museum exhibition package',
    license='MIT',
    entry_points={
        'console_scripts': [],
    },
)
```

---

## 8. Compilación y verificación

```bash
cd ~/movil_s_f_ws
colcon build --packages-select robot_museum
source install/setup.bash
ros2 launch robot_museum teleop.launch.py
```

Verificación en otra terminal:
```bash
source ~/movil_s_f_ws/install/setup.bash
ros2 topic echo /sumo/cmd_vel
```

### Comportamiento esperado al mover el stick izquierdo con L1 presionado

| Movimiento stick | Resultado en /sumo/cmd_vel |
|-----------------|---------------------------|
| Arriba | Linear x positivo (avanza) |
| Abajo | Linear x negativo (retrocede) |
| Izquierda | Angular z positivo (giro antihorario) |
| Derecha | Angular z negativo (giro horario) |
| Sin L1 presionado | Sin publicación (seguridad) |

---

## 9. Problemas encontrados y soluciones

| Problema | Causa | Solución |
|---------|-------|----------|
| No aparecía output en /cmd_vel | enable_button mapeado incorrectamente | Verificar con jstest y corregir a botón 6 (L1) |
| Movimientos invertidos | Ejes del mando Zikway con valores negativos | Usar scale negativo en yaml |
| Ejes cruzados (X controlaba rotación) | axis_linear y axis_angular invertidos | Intercambiar axis_linear.x: 0 y axis_angular.yaw: 1 |
