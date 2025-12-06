# -*- coding: mbcs -*-
#
# Abaqus/CAE Release 2020 replay file
# Internal Version: 2019_09_14-01.49.31 163176
# Run by Dragon on Sat Oct 22 06:31:51 2022
#

# from driverUtils import executeOnCaeGraphicsStartup
# executeOnCaeGraphicsStartup()
#: Executing "onCaeGraphicsStartup()" in the site directory ...
from abaqus import *
from abaqusConstants import *
from caeModules import *
from driverUtils import executeOnCaeStartup
import numpy as np
import os
import inspect
import subprocess

# For Abaqus/CAE run script case where __file__ is undefined
SCRIPT_DIR = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

exe_path = os.path.join(SCRIPT_DIR, "InsertCohesive.exe")
print(exe_path)

inp_name  = "Job-2D-4.inp"
set_name  = "New_mortar"


# 使用 Popen + communicate（不会卡死 GUI）
p = subprocess.Popen(
    [exe_path, inp_name, set_name],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    cwd=SCRIPT_DIR  # 工作目录
)

out, err = p.communicate()

print("Output file =", out)
if err:
    print("Error information:", err)
