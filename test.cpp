#include <iostream>
#include <map>  // multimapを使うために必要

int main() {
    // 先生の名前をキーにして、担当科目を管理する
    std::multimap<std::string, std::string> subjects;

    // データを追加
    subjects.insert(std::make_pair("田中", "数学"));
    subjects.insert(std::make_pair("田中", "物理"));  // 田中先生は数学と物理を担当
    subjects.insert(std::make_pair("鈴木", "英語"));
    subjects.insert(std::make_pair("鈴木", "国語"));

    // すべてのデータを表示
    std::cout << "【担当科目リスト】\n";
    for (std::multimap<std::string, std::string>::iterator it = subjects.begin(); it != subjects.end(); ++it) {
        std::cout << it->first << "先生: " << it->second << std::endl;
    }

    // 田中先生の担当科目を表示
    std::cout << "\n【田中先生の担当科目】\n";
    std::pair<std::multimap<std::string, std::string>::iterator, 
              std::multimap<std::string, std::string>::iterator> range = subjects.equal_range("田中");

    for (std::multimap<std::string, std::string>::iterator it = range.first; it != range.second; ++it) {
        std::cout << it->second << std::endl;
    }

    return 0;
}
