

---

# 📘 **InsertCohesive – Cohesive Element Automatic Insertion Tool**

*Automatically insert zero-thickness cohesive elements into Abaqus INP models*

---

## 🚀 **1. Project Overview**

**InsertCohesive** is a lightweight and efficient C++ tool designed for **automatically inserting zero-thickness cohesive elements** into Abaqus `.inp` files.

It supports both **2D** (CPS3 / CPE3) and **3D** (C3D4) finite element models, and is suitable for users working on:

* Concrete meso-scale simulation
* Interface fracture
* Composite delamination
* Random aggregate modeling
* Micro-structure modeling requiring ITZ (Interfacial Transition Zone)

This tool can run independently from the command line **or integrate seamlessly into an Abaqus Python plugin**.

---

## ✨ **2. Features**

### ✔ Automatic cohesive element insertion

* Based on shared faces between solid elements
* Zero-thickness cohesive elements are generated automatically
* Clean and consistent element numbering

### ✔ Supports 2D & 3D Abaqus models

| Dimension | Supported Element | Cohesive Element Inserted |
| --------- | ----------------- | ------------------------- |
| **2D**    | CPS3 / CPE3       | COH2D3                    |
| **3D**    | C3D4              | COH3D6                    |

### ✔ Command-line executable & DLL interface

* Direct `.exe` execution
* DLL callable from Abaqus Python
* Supports callback for GUI log printing

### ✔ Clean and readable output

* Generates new INP file with cohesive elements
* Names follow pattern:

  ```
  NewOutput_<original>.inp
  ```

---

## 📦 **3. Installation**

### **Build with Visual Studio (Windows)**

1. Clone the repository:

```bash
git clone https://github.com/seven777f/InsertCohesive.git
cd InsertCohesive
```

2. Open the `.sln` file in **Visual Studio 2022**

3. Select:

```
Release x64
```

4. Build the solution.
   Executable will appear in:

```
/Release/InsertCohesive.exe
```

DLL will appear in:

```
/Release/InsertCohesiveDll.dll
```

---

## 🖥 **4. Command-Line Usage**

### **Syntax**

```
InsertCohesive.exe <InpName> <SetName> <2D/3D>
```

### **Example**

```
InsertCohesive.exe model.inp ALL 3D
```

### **Argument Description**

| Argument    | Meaning                                                  |
| ----------- | -------------------------------------------------------- |
| `<InpName>` | Input INP filename (must be inside InpFolder/)           |
| `<SetName>` | The node/element set where cohesive insertion is applied |
| `<2D/3D>`   | Model dimension mode                                     |

---

## 📂 **5. Directory Structure**

```
InsertCohesive/
│
├── InpFolder/           # Input Abaqus INP files
├── OutputFolder/        # Generated cohesive INP files
│
├── Tools.cpp / Tools.h  # Core insertion logic
├── ToolFor2D.cpp        # 2D cohesive insertion
├── ToolFor2D.h
├── Tools2.cpp / Tools2.h
│
├── InsertCohesiveDll.dll  # DLL for Python plugin
├── InsertCohesive.exe     # Executable
│
├── kernel/               # Abaqus plugin kernel
└── python/               # Abaqus GUI plugin
```

---

## 🔌 **6. Using the DLL in Abaqus Plugins**

Example (Python 2.7):

```python
dll = ctypes.CDLL("InsertCohesiveDll.dll")

dll.Insert2D_C.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
dll.Insert2D_C.restype  = ctypes.c_char_p

result = dll.Insert2D_C("Job.inp", "ITZ_Set", "3D")
print(result)
```

---

## 📘 **7. Example Output**

After running:

```
InsertCohesive.exe Job.inp ITZ 3D
```

Output:

```
[Success] Cohesive insertion completed: NewOutput_Job.inp
```

Generated file:

```
OutputFolder/NewOutput_Job.inp
```

---

## ⚠️ **8. Limitations**

* Only supports **CPS3 / CPE3 / C3D4** (linear triangular and tetrahedral elements)
* Does **not** currently support quadratic elements (e.g., C3D10)
* Requires all faces to match cleanly and mesh to be fully connected
* Large models may require significant memory

---

## ❤️ **9. License & Use Terms**

This software is **free for personal and academic use**, but **commercial use is prohibited** without written permission.

---

## 📞 **10. Contact Author**

If you want to report bugs, request features, or collaborate:

**Contact Author**

* QQ: **343947592**
* WeChat: **hu25199084**

---

## ⭐ **11. Acknowledgements**

Special thanks to the Abaqus community and open-source contributors for their inspiration.

---

