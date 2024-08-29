cmd_/home/zyz/projects/driver_learning/modules.order := {   echo /home/zyz/projects/driver_learning/simple_driver.ko; :; } | awk '!x[$$0]++' - > /home/zyz/projects/driver_learning/modules.order
