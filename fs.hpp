#pragma once
#include <iostream>
#include <unordered_map>
#include <random>
#include <queue>
#include <map>
#include "uuid.hpp"
#include <array>
#include <nlohmann/json.hpp>


template<class...Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};

template<class...Fs>
overloaded(Fs...) -> overloaded<Fs...>;

const inline std::string kEmpty = "";
struct OperationRights{
    bool read = false;
    bool write = false;
    bool grant = false;
    OperationRights& operator|=(const OperationRights& rhs){
        read = read | rhs.read;
        write = write | rhs.write;
        grant = grant | rhs.grant;
    }
    bool IsAll(){
        return read && write && grant;
    }
};

namespace ns {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(OperationRights, read, write, grant);
}

using TypeSubjectsId = std::string;
using SubjectId = std::string;
using ObjectId = std::string;
using ObjectsTypeId = std::string;

#define match_type(X) if constexpr (std::is_same_v<T, X>)

struct Object{
    std::string name;
    std::string type_id;
    std::unordered_map<TypeSubjectsId, OperationRights> type_rights;
    std::unordered_map<SubjectId, OperationRights> subject_rights;
};

namespace ns {
    inline void to_json(nlohmann::json & json, const  std::unordered_map<TypeSubjectsId, OperationRights>& t){
        for (auto& [key, value]: t){
            to_json(json[key], value);
        }
    }
    inline void from_json(const nlohmann::json & json, std::unordered_map<TypeSubjectsId, OperationRights>& t){
        for (auto& [key, value]: t){
            from_json(json[key], value);
        }
    }
    inline void to_json(nlohmann::json &nlohmann_json_j, const Object &t) {
        nlohmann_json_j["name"] = t.name;
        nlohmann_json_j["type_id"] = t.type_id;
        to_json(nlohmann_json_j["type_rights"], t.type_rights);
        to_json(nlohmann_json_j["subject_rights"], t.subject_rights);
    }
    inline void from_json(const nlohmann::json &nlohmann_json_j, Object &t) {
        nlohmann_json_j.at("name").get_to(t.name);
        nlohmann_json_j.at("type_id").get_to(t.type_id);
        from_json(nlohmann_json_j["type_rights"], t.type_rights);
        from_json(nlohmann_json_j["subject_rights"], t.subject_rights);
    }
}

struct ObjectType{
    std::string name;
    std::unordered_map<TypeSubjectsId, OperationRights> type_rights;
    std::unordered_map<SubjectId, OperationRights> subject_rights;
};

struct Subject{
    std::string name;
    std::string password;
    std::string type_id;
};

struct SubjectType{
    std::string name;
    std::optional<std::string> type_id_parent;
};

struct FS{
    Object& FindObject(const std::string& id){
        auto it = objects_.find(id);
        if (it == objects_.end()){
            std::string msg = "не найден объект с id: ";
            msg.append(id);
            throw std::runtime_error(msg);
        }
        return it->second;
    }
    Subject& FindSubject(const std::string& id){
        auto it = subjects.find(id);
        if (it == subjects.end()){
            std::string msg = "не найден тип субъект с id: ";
            msg.append(id);
            throw std::runtime_error(msg);
        }
        return it->second;
    }
    ObjectType& FindTypeObject(const std::string& id){
        auto it = types_objects.find(id);
        if (it == types_objects.end()){
            std::string msg = "не найден тип объекта с id: ";
            msg.append(id);
            throw std::runtime_error(msg);
        }
        return it->second;
    }
    SubjectType& FindTypeSubject(const std::string& id){
        auto it = type_subjects.find(id);
        if (it == type_subjects.end()){
            std::string msg = "не найден тип субъекта с id: ";
            msg.append(id);
            throw std::runtime_error(msg);
        }
        return it->second;
    }
    OperationRights CollectRights(const std::string& subject_id, const std::string& object_id){
        OperationRights operationRights;
        auto& subject = FindSubject(subject_id);
        auto& object = FindObject(object_id);
        if (object.subject_rights.contains(subject_id)){
            operationRights |= object.subject_rights.find(subject_id)->second;
        }
        std::optional<std::string> cur_subject_type_id;
        cur_subject_type_id.emplace(subject.type_id);
        while (!operationRights.IsAll() && cur_subject_type_id){
            if (object.type_rights.contains(cur_subject_type_id.value())){
                operationRights |= object.type_rights.find(cur_subject_type_id.value())->second;
            }
            auto& cur_subject = FindSubject(*cur_subject_type_id);
            cur_subject_type_id = cur_subject.type_id;
        }
        return operationRights;
    }
    template <typename T>
    std::string GetFreeUuid(const T& t){
        std::string id = generatorUuid.Next();
        while (t.contains(id)){
            id = generatorUuid.Next();
        }
        return id;
    }

    template <typename T>
    std::unordered_map<std::string, T>& GetMap(){
        if constexpr (std::is_same_v<T, Object>){
            return objects_;
        }
        else if constexpr (std::is_same_v<T, Subject>){
            return subjects;
        }
        else if constexpr (std::is_same_v<T, ObjectType>){
            return types_objects;
        }
        else if constexpr (std::is_same_v<T, SubjectType>){
            return type_subjects;
        }
    }
    template <typename T>
    const std::string& Emplace(const T& t){
        std::unordered_map<std::string, T>& map = GetMap<T>();
        auto [it, emplaced] = map.emplace(GetFreeUuid(map), t);
        return it->first;
    }

    const ObjectId& AddObject(const std::string& object_name, const std::string& type_id){
        FindTypeObject(type_id);
        Object object{
                .name = object_name,
                .type_id = type_id
        };
        return Emplace(object);
    }
    const ObjectsTypeId& AddObjectType(const std::string& object_type_name){
        ObjectType type_object{
                .name = object_type_name,
        };
        return Emplace(type_object);
    }
    const SubjectId& AddSubject(const std::string& subject_name, const std::string& subject_type_id){
        FindTypeSubject(subject_type_id);
        Subject subject{
                .name = subject_name,
                .type_id = subject_type_id
        };
        return Emplace(subject);
    }
    const TypeSubjectsId& AddSubjectType(const std::string& subject_type_name, const std::optional<std::string>& parent_type_id = std::nullopt){
        SubjectType subject_type{
                .name = subject_type_name,
                .type_id_parent = parent_type_id
        };
        return Emplace(subject_type);
    }
    bool TryToRead(const std::string& subject_id, const std::string& object_id){
        return CollectRights(subject_id, object_id).read;
    }
    bool TryToWrite(const std::string& subject_id, const std::string& object_id){
        return CollectRights(subject_id, object_id).write;
    }
    bool TryToGrant(const std::string& subject_id, const std::string& object_id, OperationRights operationRights){
        FindSubject(subject_id);
        auto& object = FindObject(object_id);
        auto rights = CollectRights(subject_id, object_id);
        auto can_grant =
                rights.write == operationRights.write &&
                rights.read == operationRights.read &&
                rights.grant;
        if (!can_grant){
            return false;
        }
        if (object.subject_rights.contains(subject_id)){
            auto& op = object.subject_rights.find(subject_id)->second;
            op |= operationRights;
        }
        else {
            object.subject_rights.emplace(subject_id, operationRights);
        }
        return true;
    }

    bool TryToGrantType(const std::string& subject_id, const std::string& object_type_id, OperationRights operationRights){
        FindSubject(subject_id);
        auto& object_type = FindObject(object_type_id);
        auto rights = CollectRights(subject_id, object_type_id);
        auto can_grant =
                rights.write == operationRights.write &&
                rights.read == operationRights.read &&
                rights.grant;
        if (!can_grant){
            return false;
        }
        if (object_type.subject_rights.contains(subject_id)){
            auto& op = object_type.subject_rights.find(subject_id)->second;
            op |= operationRights;
        }
        else {
            object_type.subject_rights.emplace(subject_id, operationRights);
        }
        return true;
    }

    std::string GenerateBorder(std::size_t n){
        std::string result (n, '-');
        return result;
    }

    const std::string& GetObjectTypeName(const std::string& object_type_id){
        return FindTypeObject(object_type_id).name;
    }

    const std::string& GetSubjectTypeName(const std::string& subject_type_id){
        return FindTypeSubject(subject_type_id).name;
    }

    static constexpr std::string_view kId = "id";
    static constexpr std::string_view kNameObject = "name of object";
    static constexpr std::string_view kNameObjectType = "name of object type";
    static constexpr std::string_view kNameSubject = "name of subject";
    static constexpr std::string_view kNameSubjectType = "name of subject type";
    static constexpr std::string_view kNameParentSubjectType = "name of parent subject type";
    static constexpr std::string_view kNameType = "name of type";
    static constexpr std::string_view kObjects = "Objects: ";
    static constexpr std::string_view kSubjects = "Subjects: ";
    static constexpr std::string_view kObjectTypes = "Object types: ";
    static constexpr std::string_view kSubjectTypes = "Subject types: ";

    auto CollectLenObjectTable(){
        std::size_t len_id{};
        std::size_t len_object_name{};
        std::size_t len_object_type_name{};
        for (auto& [id, object] :objects_){
            len_id = std::max(len_id, id.size());
            len_object_name = std::max(len_object_name, object.name.size());
            auto& type_object = FindTypeObject(object.type_id);
            len_object_type_name = std::max(len_object_type_name, type_object.name.size());
        }
        return std::to_array({len_id, len_object_name, len_object_type_name});
    }

    const std::string& GetTypeName(auto& obj){
        auto matcher = overloaded
                {
                        [&](Object& obj) -> const std::string&{
                            return GetObjectTypeName(obj.type_id);
                        },
                        [&](Subject& subj)-> const std::string&{
                            return GetSubjectTypeName(subj.type_id);
                        },
                        [&](SubjectType& subj) -> const std::string&{
                            if (subj.type_id_parent){
                                return GetSubjectTypeName(subj.type_id_parent.value());
                            }
                            else {
                                return kEmpty;
                            }
                        }
                };
        auto& result = matcher(obj);
        return result;
    }

    template <typename T>
    static constexpr auto GetNameColumns(){
        return std::array<std::string_view, 0>{};
    }

    template <>
    static constexpr auto GetNameColumns<Object>(){
        return std::to_array({
                                     kId, kNameObject, kNameObjectType
                             });
    }

    template <>
    static constexpr auto GetNameColumns<ObjectType>(){
        return std::to_array({
                                     kId, kNameObjectType
                             });
    }

    template <>
    static constexpr auto GetNameColumns<Subject>(){
        return std::to_array({
                                     kId, kNameSubject, kNameSubjectType
                             });
    }

    template <>
    static constexpr auto GetNameColumns<SubjectType>(){
        return std::to_array({
                                     kId, kNameSubjectType, kNameParentSubjectType
                             });
    }

    template <typename T>
    auto GetPrintPrefix(){
        match_type(Object){
            return kObjects;
        }
        match_type(Subject){
            return kSubjects;
        }
        match_type(ObjectType){
            return kObjectTypes;
        }
        match_type(SubjectType){
            return kSubjectTypes;
        }
    }


    template <typename T>
    auto CollectLenTable(){
        std::size_t len_id{};
        std::size_t len_name{};
        std::size_t len_type{};
        for (auto& [id, item] : GetMap<T>()){
            len_id = std::max(len_id, id.size());
            len_name = std::max(len_name, item.name.size());
            auto& type_name = GetTypeName(item);
            len_type = std::max(len_type, type_name.size());
        }
        constexpr auto names = GetNameColumns<T>();
        len_id = std::max(len_id, names[0].size());
        len_name = std::max(len_name, names[1].size());
        len_type = std::max(len_type, names[2].size());
        return std::to_array({len_id, len_name, len_type});
    }
    template <>
    auto CollectLenTable<ObjectType>(){
        std::size_t len_id{};
        std::size_t len_name{};
        for (auto& [id, item] : GetMap<ObjectType>()){
            len_id = std::max(len_id, id.size());
            len_name = std::max(len_name, item.name.size());
        }
        constexpr auto names = GetNameColumns<ObjectType>();
        len_id = std::max(len_id, names[0].size());
        len_name = std::max(len_name, names[1].size());
        return std::to_array({len_id, len_name});
    }
    template <typename T>
    void PrintItems(){
        auto lens = CollectLenTable<T>();
        constexpr auto names = GetNameColumns<T>();
        std::cout << GetPrintPrefix<T>() << "\n";
        if constexpr (lens.size() == 3) {
            auto len_id = lens[0];
            auto len_name = lens[1];
            auto len_type = lens[2];
            auto borders = GenerateBorder(4 + len_id + len_name + len_type);
            std::cout << borders << '\n';
            std::cout
                    << "|" << std::left << std::setw(len_id) << names[0] << std::setw(0)
                    << "|" << std::left << std::setw(len_name) << names[1] << std::setw(0)
                    << "|" << std::left << std::setw(len_type) << names[2] << std::setw(0)
                    << "|\n";
            for (auto &[id, item]: GetMap<T>()) {
                auto& type =  GetTypeName(item);
                std::cout
                        << "|" << std::left << std::setw(len_id) << id << std::setw(0)
                        << "|" << std::left << std::setw(len_name) << item.name << std::setw(0)
                        << "|" << std::left << std::setw(len_type) << type
                        << std::setw(0)
                        << "|\n";
            }
            std::cout << borders << "\n";
        }
        else if constexpr (lens.size() == 2){
            auto len_id = lens[0];
            auto len_name = lens[1];
            auto borders = GenerateBorder(4 + len_id + len_name);
            std::cout << borders << '\n';
            std::cout
                    << "|" << std::left << std::setw(len_id) << names[0] << std::setw(0)
                    << "|" << std::left << std::setw(len_name) << names[1] << std::setw(0)
                    << "|\n";
            for (auto &[id, object]: GetMap<T>()) {
                std::cout
                        << "|" << std::left << std::setw(len_id) << id << std::setw(0)
                        << "|" << std::left << std::setw(len_name) << object.name << std::setw(0)
                        << std::setw(0)
                        << "|\n";
            }
            std::cout << borders << "\n";
        }
    }

    void PrintObjects(){
        PrintItems<Object>();
    }
    void PrintSubjects(){
        PrintItems<Subject>();
    }
    void PrintObjectsType(){
        PrintItems<ObjectType>();
    }
    void PrintSubjectsType(){
        PrintItems<SubjectType>();
    }
    void GrantRightsObjectTypeSubjectType(const std::string& object_type_id, const std::string& subject_type_id, OperationRights op_rights){
        auto& object_type = FindTypeObject(object_type_id);
        FindTypeSubject(subject_type_id);
        auto [it, inserted] = object_type.type_rights.emplace(subject_type_id, op_rights);
        if (!inserted){
            it->second |= op_rights;
        }
    }
private:
    std::unordered_map<ObjectId, Object> objects_;
    std::unordered_map<ObjectsTypeId , ObjectType> types_objects;
    std::unordered_map<SubjectId , Subject> subjects;
    std::unordered_map<TypeSubjectsId , SubjectType> type_subjects;
    GeneratorUUID generatorUuid;
};
