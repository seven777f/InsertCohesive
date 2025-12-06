



#include "Tools.h"
#include "ToolFor2D.h"
//#include "Tools2.h"
//int main() {
//    std::string InpName = "Irregular_50-4.inp";
//    Timer main_timer("Main Program"); // 主程序计时
//    main_timer.start();
//    // 设置线程数
//    std::cout << "Hardware concurrency: " << std::thread::hardware_concurrency() << std::endl;
//    omp_set_dynamic(0);
//    omp_set_num_threads(std::thread::hardware_concurrency());
//    //std::cout << "OpenMP version: " << _OPENMP << std::endl;
//
//    std::vector<Node3D> originNodes;
//    std::vector<COH3D6> originElements;
//    OutputSet outputSet;
//    std::string INPFilePath = inpFolder + "/" + InpName;
//    std::string ExtractSdegIdsFolder = "extract_sdeg_ids";
//    //if (read3DInp(inpFolder + "/" + INPName, originNodes, originElements)) {
//    read3DInpOutput(INPFilePath, originNodes, originElements, outputSet);
//    main_timer.stop(); main_timer.getLastTime("读取inp成功");
//
//    main_timer.start();
//    std::unordered_map<int, COH3D6*> ElementMap = creatElementMap(originElements);
//    std::unordered_map<int, Node3D*> node_map = creatNodeMap(originNodes);
//    main_timer.stop(); main_timer.getLastTime("创建map成功！");
//
//
//
//
//    //计算每个内聚力单元的面积
//    //根据std::vector<Node3D> originNodes;和std::vector<COH3D6> originElements;  计算每个内聚力单元的面积 添加到每个COH3D6的成员变量area中
//    main_timer.start();
//    ComputeCohesiveAreas(node_map, originElements);
//    main_timer.stop(); main_timer.getLastTime("计算面积成功：");
//    //读取std::string ExtractSdegIdsFolder文件夹内的所有文件:文件名称如 ：sdeg_ids__CONCRETE-1.COHESIVEELEMENTS__frame0.txt   sdeg_ids__CONCRETE-1.COHESIVEELEMENTS__frame81.txt  0  81 表示每一帧的帧序号
//    //CONCRETE-1.5963373
//    //CONCRETE-1.5963802
//    //CONCRETE-1.5963826
//    //CONCRETE-1.5963865
//    //解析出"CONCRETE-1."后面的内聚力单元的id  利用函数bool IsITZCohesiveElement(int id, const OutputSet& outputSet);bool IsMortarCohesiveElement(int id, const OutputSet & outputSet);
//    //判断是ITZ的内聚力单元还是砂浆的内聚力单元 分别保存
//    //最终分别返回每一帧的 断裂的砂浆中内聚力单元的id 和 ITZ中内聚力单元的id
//    main_timer.start();
//    std::map<int, std::pair<std::unordered_set<int>, std::unordered_set<int>>> frameBrokenElements =ProcessBrokenCohesiveElements(ExtractSdegIdsFolder, outputSet);
//    std::string ExtractSdegIdsOutputFolder = "extract_sdeg_idsOutput"; prepareOutputFolder(ExtractSdegIdsOutputFolder);
//   
//    WriteFrameBrokenElements(ExtractSdegIdsOutputFolder, frameBrokenElements);
//    main_timer.stop(); main_timer.getLastTime("分离成功：");
//    //根据frameBrokenElements和std::unordered_map<int, COH3D6*> ElementMap计算每一帧 的破坏面积（ITZ和Mortar分别统计）
//    //输出一个txt文件  frameNum  Mortar_crackArea   ITZ_CrackArea  如果是0的话填0  要从第0帧开始统计
//    main_timer.start();
//    ComputeAndWriteCrackAreas(frameBrokenElements, ElementMap, "crack_areas.txt");
//
//    main_timer.stop(); main_timer.getLastTime("统计断裂面积成功：");
//
//}




//int main(int argc, char* argv[]) {//给定一个参数即可即 inp文件的名称
//    // 检查参数数量（至少需要程序名 + 1个参数）
//    if (argc < 2) {
//        std::cerr << "用法: " << argv[0] << " <InpName>\n";
//        std::cerr << "示例: " << argv[0] << " model.inp\n";
//        system("pause");
//        return 1;
//    }
//    std::string jsonFilePath = "mergeInfo.json";//写死就行
//    std::string InpName = argv[1];   // 第一个参数 inp文件的名称
//
//  
//    std::string convertInpName = ConvertInpFromHyperMesh(InpName, jsonFilePath);
//    // 可选：输出结果
//    std::cout << "转换完成: " << convertInpName << std::endl;
//
//    system("pause");
//    return 0;
//}

//int main(int argc, char* argv[]) {//需要三个参数
//     //检查参数数量（至少需要程序名 + 1个参数）
//    if (argc < 3) {
//        std::cerr << "用法: " << argv[0] << " <InpName> <SetName>\n";
//        std::cerr << "示例: " << argv[0] << " model.inp ALL\n";
//        system("pause");
//        return 0;
//    }
//
//    std::string InpName = argv[1];   // 第一个参数
//    std::string keyword = argv[2];   // 第二个参数
// 
//    std::string outputname=Insert(InpName, keyword);
//    std::cout << "插入完成: " << outputname << std::endl;
//
//
//
//
//
//    system("pause");
//    return 0;
//
//}
int main(int argc, char* argv[]) {//需要三个参数
     //检查参数数量（至少需要程序名 + 1个参数）
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <InpName> <SetName> <2D/3D>\n";
        std::cerr << "Example: " << argv[0] << " model.inp ALL 2D\n";
        system("pause");
        return 0;
    }
    //initFolders();
    std::string InpName = argv[1];   // 第一个参数
    std::string keyword = argv[2];   // 第二个参数
    std::string Dimension = argv[3];   // 第二个参数
    std::string outputname;
    if (Dimension=="2D") {
        outputname = Insert2D(InpName, keyword);
    }
    else {
        outputname = Insert3D(InpName, keyword);
    }
   
    std::cout << "[Success] Cohesive insertion completed: " << outputname << std::endl;





    system("pause");
    return 0;

}
//int main() {
//    
//
//    std::string InpName = "Job-2D-4.inp";
//  
//    Insert2D(InpName, "All");
//
//
//
//
//
//
//    return 0;
//
//}
//int main() {
//   std::string InpName = "Convert_1110.inp";
//   Insert3D(InpName, "ALL");
//    
//  
//    
//   
//   
//    
//   
//
//    return 0;
//  
//}


//
//int main(int argc, char* argv[]) {
//    if (argc < 3) {
//        std::cerr << "用法: " << argv[0] << " <InpName> <Keyword>\n";
//        return 1;
//    }
//
//    std::string InpName = argv[1];   // 第一个参数
//    std::string keyword = argv[2];   // 第二个参数
//
//    std::string convertInpName = ConvertInpFromHyperMesh(InpName);
//    if (!convertInpName.empty()) {
//        Insert(convertInpName, keyword);
//    }
//
//
//    
//   
// 
//
//
//
//
//
//
//
//
//
//    //Timer main_timer("Main Program"); // 主程序计时
//    //main_timer.start();
//    //std::vector<Node3D> originNodes;
//    //std::vector<Element3D> originElements;
//    //std::string inpFolder = "F:/2025NewC/InsertCohesive/InsertCohesive/InpFolder";
//    ////std::string OutputFolder = "F:/2025NewC/InsertCohesive/InsertCohesive/OutputFolder";
//    //std::string OutputFolder = "F:/temp/LJZ0907";
//    //std::string INPName = "5555.inp";
//    //if (read3DInp(inpFolder +"/" + INPName, originNodes, originElements)) {
//    //    std::vector<Element3D> NewElements; std::vector<Node3D> NewNodes;  std::vector<COH3D6> cohesive_elements;
//    //
//    //    if (insertCohesiveElements2(originNodes, originElements, NewElements, NewNodes, cohesive_elements)) {
//    //        printMeshSummary(NewNodes, NewElements);
//
//
//    //        std::cout << "Inserted " << cohesive_elements.size() << " COH3D6 elements." << std::endl;
//
//    //       /* for (int i = 0; i < 10; i++) {
//    //            printCOH3D6(cohesive_elements[i]);
//    //        }*/
//    //        write3DInp(OutputFolder + "/Output_" + INPName, NewNodes, NewElements, cohesive_elements);
//    //    }
//    //    else {
//    //        std::cout << "插入内聚力单元失败！程序中止！" << std::endl;
//    //    }
//    //}
//    //else {
//    //    std::cout << "读取INP文件失败 ！程序终止" << std::endl;
//    //}
//    //main_timer.stop();
//    //main_timer.print();
//
//
//
//   
//
//
//    return 0;
//}
//
//

