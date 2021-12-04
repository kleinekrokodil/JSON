#pragma once

#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <variant>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using Number = std::variant<int, double>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        Node() = default;
        Node(std::nullptr_t value);
        Node(int value);
        Node(double value);
        Node(bool value);
        Node(std::string value);
        Node(Dict map);
        Node(Array array);

        bool IsNull() const;
        bool IsArray() const;
        const Array& AsArray() const;
        bool IsMap() const;
        const Dict& AsMap() const;
        bool IsInt() const;
        int AsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        double AsDouble() const;
        bool IsBool() const;
        bool AsBool() const;
        bool IsString() const;
        const std::string& AsString() const;

        friend bool operator==(const Node& left, const Node& right){
            return left.node_ == right.node_;
        }

        friend bool operator!=(const Node& left, const Node& right){
            return left.node_ != right.node_;
        }

    private:
        std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> node_;
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

        friend bool operator==(const Document& left, const Document& right){
            return left.root_ == right.root_;
        }

        friend bool operator!=(const Document& left, const Document& right){
            return left.root_ != right.root_;
        }

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json