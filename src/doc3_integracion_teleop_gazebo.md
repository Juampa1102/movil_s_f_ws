# Integración del Control Joystick con el Robot en Gazebo

**Proyecto:** Robot Móvil Museo — movil_s_f_ws  
**Fecha:** Mayo 2026  
**Sistema:** Ubuntu 22.04 / ROS2 Humble / Gazebo Classic 11.10.2

---

## 1. Descripción general

Este documento describe cómo se conectaron los dos sistemas desarrollados previamente — el control por joystick y la simulación en Gazebo — para que el mando Zikway controle el robot sumo en tiempo real dentro de la simulación.

---

## 2. Arquitectura de nodos

```
[Mando Zikway USB]
        │
        ▼
[joy_node]  →  /joy  →  [teleop_twist_joy_node]
                                  │
                                  ▼
                          /sumo/cmd_vel
                                  │
                                  ▼
                    [diff_drive_controller]  ←  Gazebo
                                  │
                                  ▼
                    Rueda_Izquierda_joint
                    Rueda_Derecha_joint
                                  │
                                  ▼
                             /sumo/odom
                             /tf
```

### Nodos activos durante la teleoperación

| Nodo | Paquete | Función |
|------|---------|---------|
| joy_node | joy | Lee el mando USB y publica /joy |
| teleop_twist_joy_node | teleop_twist_joy | Convierte /joy a /sumo/cmd_vel |
| robot_state_publisher | robot_state_publisher | Publica TF desde URDF |
| joint_state_publisher | joint_state_publisher | Publica estado de joints |
| gazebo | gazebo_ros | Simulación física |
| diff_drive_controller | (plugin Gazebo) | Controla ruedas con cmd_vel |

---

## 3. Punto clave de integración — Namespace

El plugin `diff_drive` en el URDF está configurado con namespace `/sumo`:

```xml
<ros>
  <namespace>/sumo</namespace>
  <remapping>cmd_vel:=cmd_vel</remapping>
</ros>
```

Esto significa que el robot escucha en `/sumo/cmd_vel`. El teleop debe publicar en ese mismo topic. El remapping en el launch file del teleop es:

```python
remappings=[('/cmd_vel', '/sumo/cmd_vel')],
```

Si el namespace no coincide, el robot no responde aunque todo lo demás esté funcionando.

---

## 4. Procedimiento de lanzamiento

Se requieren dos terminales separadas:

**Terminal 1 — Simulación en Gazebo:**
```bash
cd ~/movil_s_f_ws
source install/setup.bash
ros2 launch sumo_robot sumo_gazebo_only.launch.py
```

Esperar a que Gazebo cargue completamente y el robot aparezca (aproximadamente 5-8 segundos).

**Terminal 2 — Teleoperación:**
```bash
cd ~/movil_s_f_ws
source install/setup.bash
ros2 launch robot_museum teleop.launch.py
```

**Para operar:** Mantener presionado L1 y mover el stick izquierdo.

---

## 5. Mapeo de controles final

| Acción en el mando | Resultado en simulación |
|-------------------|------------------------|
| L1 presionado + stick arriba | Robot avanza |
| L1 presionado + stick abajo | Robot retrocede |
| L1 presionado + stick izquierda | Robot gira antihorario |
| L1 presionado + stick derecha | Robot gira horario |
| L1 presionado + R1 + stick | Modo turbo (velocidad doble) |
| Sin L1 | Robot no responde (seguridad) |

---

## 6. Parámetros de velocidad finales

```yaml
scale_linear:
  x: 2.0        # Velocidad lineal normal (m/s)
scale_linear_turbo:
  x: 4.0        # Velocidad lineal turbo (m/s)
scale_angular:
  yaw: -0.8     # Velocidad angular (rad/s) — negativo corrige inversión del mando
```

Los parámetros físicos en el plugin diff_drive que habilitan estas velocidades:
```xml
<max_wheel_torque>50</max_wheel_torque>
<max_wheel_acceleration>20.0</max_wheel_acceleration>
```

---

## 7. Verificación del flujo completo

Con ambas terminales corriendo, verificar los topics activos:

```bash
ros2 topic list
```

Deben aparecer entre otros:
- `/joy` — datos crudos del mando
- `/sumo/cmd_vel` — comandos de velocidad
- `/sumo/odom` — odometría del robot simulado
- `/tf` — transformaciones
- `/robot_description` — URDF publicado

Verificar que llegan comandos cuando se mueve el mando con L1:
```bash
ros2 topic echo /sumo/cmd_vel
```

Verificar que la odometría se actualiza cuando el robot se mueve:
```bash
ros2 topic echo /sumo/odom
```

---

## 8. Problemas encontrados durante la integración

| Problema | Causa | Solución |
|---------|-------|----------|
| Robot no respondía al mando | Teleop publicaba en /robot/cmd_vel, plugin escucha en /sumo/cmd_vel | Cambiar remapping en teleop.launch.py |
| Movimientos hacia adelante muy lentos | max_wheel_torque y max_wheel_acceleration bajos en el plugin | Aumentar a torque=50, acceleration=20 |
| Giros y avance intercambiados | axis_linear y axis_angular con ejes cruzados | Intercambiar axis_linear.x: 0 y axis_angular.yaw: 1 |
| Robot se volcaba al girar | scale_angular demasiado alto | Reducir a 0.8 rad/s |

---

## 9. Estado del sistema al finalizar esta etapa

| Componente | Estado |
|-----------|--------|
| Mando detectado por Linux | ✅ |
| joy_node publicando /joy | ✅ |
| teleop publicando /sumo/cmd_vel | ✅ |
| Botón de seguridad L1 funcional | ✅ |
| URDF visible en RViz con orientación correcta | ✅ |
| Robot spawneado en Gazebo con colores | ✅ |
| Robot controlable con mando en Gazebo | ✅ |
| Odometría publicándose | ✅ |

---

## 10. Próximos pasos

- Crear mundo de Gazebo con dojo de sumo (círculo negro con borde blanco)
- Implementar nodo de estrategia autónoma de minisumo en el PC
- Simular sensores VL53L0X como publicadores de prueba
- Agregar URDF del robot fútbol con servos
- Desarrollar interfaz web con rosbridge_suite
