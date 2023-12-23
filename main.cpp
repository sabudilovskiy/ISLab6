#include <iostream>
#include <unordered_map>
#include <random>
#include <queue>
#include <map>
#include "uuid.hpp"
#include <array>
#include "fs.hpp"

int ReadInt(std::string_view text, std::string_view error, int max){
    try{
        int a;
        std::cout << text;
        std::cin >> a;
        if (a < 0 || a > max){
            throw std::exception{};
        }
        return a;
    }
    catch (...){
        while (true) {
            try {
                std::cout << error;
                int a;
                std::cin >> a;
                if (a < 0 || a > max){
                    throw std::exception{};
                }
                return a;
            }
            catch (...){}
        }
    }
}

std::string ReadString(std::string_view text){
    std::string result;
    std::cout << text;
    std::cin >> result;
    return result;
}

struct ConsoleHandler{
    using Method = void (ConsoleHandler::*)();
    using FSMethod = void (FS::*)();
    void AddToMenu(std::string result, std::function<void()> callback){
        menu.emplace_back(std::move(result), std::move(callback));
    }
    void AddToMenu(std::string result, Method method){
        menu.emplace_back(std::move(result), [this, method](){
            (this->*method)();
        });
    }
    void AddToMenu(std::string result, FSMethod method){
        menu.emplace_back(std::move(result), [this, method](){
            (this->fs.*method)();
        });
    }
    void End(){
        end = true;
    }
    void AddObjectType(){
        std::string name = ReadString("Введите название нового типа: ");
        auto& created_id = fs.AddObjectType(name);
        std::cout << "Создан тип с id: " << created_id  << '\n';
    }
    void AddObject(){
        std::string name = ReadString("Введите название нового объекта: ");
        std::string object_type_id = ReadString("Введите id вида объекта: ");
        auto& created_id = fs.AddObject(name, object_type_id);
        std::cout << "Создан тип с id: " << created_id << '\n';
    }
    void AddRootSubjectType(){
        std::string name = ReadString("Введите название нового типа: ");
        auto& created_id = fs.AddSubjectType(name);
        std::cout << "Создан тип с id: " << created_id  << '\n';
    }
    void AddParentSubjectType(){
        std::string name = ReadString("Введите название нового типа: ");
        std::optional<std::string> id = ReadString("Введите id родительского типа: ");
        auto& created_id = fs.AddSubjectType(name, id);
        std::cout << "Создан тип с id: " << created_id  << '\n';
    }
    void AddSubject(){
        std::string name = ReadString("Введите название нового субъекта: ");
        std::string id = ReadString("Введите id родительского типа: ");
        auto& created_id = fs.AddSubject(name, id);
        std::cout << "Создан тип с id: " << created_id  << '\n';
    }

    void GrantRightsObjectTypeSubjectType(){
        std::string id_object_type = ReadString("Введите id типа объектов: ");
        std::string id_subject_type = ReadString("Введите id типа субъектов: ");
        OperationRights rights;
        rights.read = ReadInt("Могут ли субъекты этого типа читать? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;
        rights.write = ReadInt("Могут ли субъекты этого типа записывать? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;
        rights.grant = ReadInt("Могут ли субъекты этого типа давать права? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;
        fs.GrantRightsObjectTypeSubjectType(id_object_type, id_subject_type, rights);
    }

    void TryToReadSubject() {
        std::string subject_id = ReadString("Введите id субъекта: ");
        std::string target_object_id = ReadString("Введите id целевого объекта: ");
        fs.TryToRead(subject_id, target_object_id);
    }

    void TryToWriteSubject() {
        std::string subject_id = ReadString("Введите id субъекта: ");
        std::string target_subject_id = ReadString("Введите id целевого объекта: ");
        fs.TryToWrite(subject_id, target_subject_id);
    }

    void TryToGrantSubject() {
        std::string grantor_id = ReadString("Введите id субъекта, который дает права: ");
        std::string grantee_id = ReadString("Введите id субъекта, которому даем права: ");

        OperationRights rights;
        rights.read = ReadInt("Может ли субъект читать? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;
        rights.write = ReadInt("Может ли субъект записывать? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;
        rights.grant = ReadInt("Может ли субъект давать права? (1 - Да, 0 - Нет): ", "Введите 0 или 1: ", 1) == 1;

        fs.TryToGrant(grantor_id, grantee_id, rights);
    }
    ConsoleHandler(){
        AddToMenu("Закончить работу.", &ConsoleHandler::End);
        AddToMenu("Напечатать все объекты FS", &FS::PrintObjects);
        AddToMenu("Напечатать все виды объектов FS", &FS::PrintObjectsType);
        AddToMenu("Напечатать все субъекты FS", &FS::PrintSubjects);
        AddToMenu("Напечатать все виды субъектов FS", &FS::PrintSubjectsType);
        AddToMenu("Создать новый вид объектов", &ConsoleHandler::AddObjectType);
        AddToMenu("Создать новый объект", &ConsoleHandler::AddObject);
        AddToMenu("Создать новый вид субъектов(без родительского типа)", &ConsoleHandler::AddRootSubjectType);
        AddToMenu("Создать новый вид субъектов(с родительским типом)", &ConsoleHandler::AddParentSubjectType);
        AddToMenu("Создать новый субъект", &ConsoleHandler::AddSubject);
        AddToMenu("Дать права типу субъеков у типа объектов", &ConsoleHandler::GrantRightsObjectTypeSubjectType);
        AddToMenu("Попытка чтения субъекта", &ConsoleHandler::TryToReadSubject);
        AddToMenu("Попытка записи субъекта", &ConsoleHandler::TryToWriteSubject);
        AddToMenu("Попытка выдачи прав на субъект", &ConsoleHandler::TryToGrantSubject);
    }
    void PrintPoints(){
        for (int i = 0; i < menu.size(); i++){
            std::cout << i << ") " << menu[i].first << '\n';
        }
    }
    void Run(){
        while (!end){
            PrintPoints();
            auto point = ReadInt("Введите пункт меню: ", "Допущена ошибка, повторите ввод: ", menu.size() - 1);
            try {
                menu[point].second();
            }
            catch (std::exception& exc){
                std::cout << "Произошла ошибка: \"" << exc.what() << "\"\n";
            }
        }
    }
private:
    bool end = false;
    FS fs;
    std::vector<std::pair<std::string, std::function<void()>>> menu;
};


int main() {
    ConsoleHandler consoleHandler;
    consoleHandler.Run();
}
