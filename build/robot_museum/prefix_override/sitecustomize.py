import sys
if sys.prefix == '/home/juampa/.platformio/penv':
    sys.real_prefix = sys.prefix
    sys.prefix = sys.exec_prefix = '/home/juampa/movil_s_f_ws/install/robot_museum'
