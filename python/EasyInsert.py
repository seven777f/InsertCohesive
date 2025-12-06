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
import os
import inspect
import ctypes
import uuid
##我的代码只支持 CPS3 CPE3 C3D4
def EasyInsert(Model_name="Model-1",Part_name="MORTAR_RECTANGLE",Set_name="All",isGUIRender=True):
    #判断模型部件 集合是否存在 不存在直接返回-2
    # 判断模型是否存在
    if Model_name not in mdb.models.keys():
        print("Error: Model '%s' does not exist." % Model_name)
        return -2

    model = mdb.models[Model_name]

    # 判断部件是否存在
    if Part_name not in model.parts.keys():
        print("Error: Part '%s' does not exist in model '%s'." % (Part_name, Model_name))
        return -2

    p = model.parts[Part_name]
    # 判断部件是否已经 mesh
    if len(p.elements) == 0:
        print("Error: Part '%s' has not been meshed." % Part_name)
        session.viewports['Viewport: 1'].setValues(displayedObject=p)
        print("Tip: Use Seed + Mesh tool to mesh the part before calling EasyInsert.")
        return -3
    # 判断集合是否存在
    if Set_name not in p.sets.keys():
        print("Error: Set '%s' does not exist in part '%s'." % (Set_name, Part_name))
       
        return -2

    #插入内聚力单元的脚本
    current_path = os.getcwd()
    # 获取脚本目录
    SCRIPT_DIR = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
    #inp_name ="Job-2D-4"#临时过渡变量
    inp_name = "Insert_" + str(uuid.uuid4())[:8]

    inp_name2=inp_name+".inp"
    TargetPartName=(Part_name+"("+Set_name+")").upper()#这是dll文件处理后自动生成的部件名称 不能修改
    InpFileFolder=os.path.join(SCRIPT_DIR, "InpFolder")#abaqus生成的 输入inp文件的文件夹 
    OutPutInpFileFolder=os.path.join(SCRIPT_DIR, "OutputFolder")#dll处理后输出文件的文件夹
    InputinpFilePath=os.path.join(InpFileFolder,inp_name2)#这是abaqus生成的被dll使用的实际文件 最后要删除掉 绝对路径
    OutputInpPath= os.path.join(OutPutInpFileFolder,"NewOutput_"+inp_name2)#这是dll文件生成后默认的名称 不能变  导入后要清理掉  绝对路径



  

    TargetSet=p.sets[Set_name]
    
    eles=TargetSet.elements
    eleType=eles[0].type
    ##我的代码只支持 CPS3 CPE3 C3D4
    if eleType in (CPS3, CPE3):
        Dimention = "2D"
    elif eleType == C3D4:
        Dimention = "3D"
    else:
        Dimention = "INVALID"
    print(Dimention)
    if Dimention == "INVALID":
        return -1
    a = mdb.models[Model_name].rootAssembly
    a.Instance(name=Part_name+'-1', part=p, dependent=ON)
    mdb.Job(name=inp_name, model=Model_name, description='', type=ANALYSIS, 
        atTime=None, waitMinutes=0, waitHours=0, queue=None, memory=90, 
        memoryUnits=PERCENTAGE, getMemoryFromAnalysis=True, 
        explicitPrecision=SINGLE, nodalOutputPrecision=SINGLE, echoPrint=OFF, 
        modelPrint=OFF, contactPrint=OFF, historyPrint=OFF, userSubroutine='', 
        scratch='', resultsFormat=ODB, parallelizationMethodExplicit=DOMAIN, 
        numDomains=2, activateLoadBalancing=False, multiprocessingMode=DEFAULT, 
        numCpus=2, numGPUs=0)

    os.chdir(InpFileFolder)
    mdb.jobs[inp_name].writeInput(consistencyChecking=OFF)
    #os.chdir(current_path)

    # 切换工作目录到脚本目录（关键）
    os.chdir(SCRIPT_DIR)
    # DLL 路径
    dll_path = os.path.join(SCRIPT_DIR, "InsertCohesiveDll.dll")

    # 加载 DLL
    dll = ctypes.CDLL(dll_path)
    # 定义 Python 回调函数类型
    CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    def py_print(msg):
        print(msg.decode("utf-8"))

    # 转成 C 回调
    cb = CALLBACK(py_print)
    dll.SetCallback.argtypes = [CALLBACK]
    dll.SetCallback.restype  = None
    # 注册回调到 DLL
    dll.SetCallback(cb)
    # 指定函数签名
    dll.Insert2D_C.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    dll.Insert2D_C.restype  = ctypes.c_char_p

    # 构造绝对路径

    # 调用 C++ 函数
    try:
        result = dll.Insert2D_C(inp_name2.encode("utf-8"), Set_name.encode("utf-8"), Dimention.encode("utf-8"))
    except Exception as e:
        print("DLL error:", e)
        return -3
  
    os.chdir(current_path)
    # 卸载 DLL
    handle = dll._handle
    FreeLibrary = ctypes.windll.kernel32.FreeLibrary
    FreeLibrary.argtypes = [ctypes.c_void_p]
    FreeLibrary.restype  = ctypes.c_bool
    FreeLibrary(ctypes.c_void_p(handle))
    del dll

    #导入inp文件 
    if (isGUIRender):
        NewModelName='NewOutput_Job-2D-4'#用于过渡的模型名称 没有实际意义

        mdb.ModelFromInputFile(name=NewModelName,  inputFileName=OutputInpPath)#导入到abaqus中

        #转移Part的位置到原来的模型上
        p2= mdb.models[Model_name].Part( name=TargetPartName, 
        objectToCopy=mdb.models[NewModelName].parts[TargetPartName])
        del mdb.models[NewModelName]#删除该模型 因为部件已经复制到指定地方了
        session.viewports['Viewport: 1'].setValues(displayedObject=p2)#视角显示该part
         # 删除输出文件
        if os.path.exists(OutputInpPath):
            os.remove(OutputInpPath)
        
    #清理资源 删除两个生成的inp文件避免给用户生成太多垃圾文件
    # 删除输入文件
    if os.path.exists(InputinpFilePath):
        os.remove(InputinpFilePath)
    del a.features[Part_name+'-1']
    del mdb.jobs[inp_name]
   
    return 0


if __name__=='__main__':##主函数   
    Model_name="Model-1"#模型名称
    Part_name="MORTAR_RECTANGLE"#部件的名称
    Set_name="Aggregate"#集合的名称
    isGUIRender=True#是否导入到abaqus的GUI界面（大型模型可能卡死GUI界面用户可选择）
    result=EasyInsert(Model_name,Part_name,Set_name,isGUIRender)