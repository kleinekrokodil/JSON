#include <istream>

#include "json.h"

using namespace std;

namespace json {

    namespace {

        //-------------Загрузка данных----------------

        Node LoadNode(istream& input);
        Node LoadArray(istream& input) {
            Array result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Need ']' symbol to close array");
            }

            return Node(move(result));
        }
        Node LoadString(std::istream& input) {
            char ch;
            std::string str;
            bool is_escape = false;
            while (input.get(ch)) {
                if (is_escape) {
                    switch (ch) {
                        case 'n': str += '\n'; break;
                        case 'r': str += '\r'; break;
                        case 't': str += '\t'; break;
                        case '"': [[fallthrough]];
                        case '\\': str += ch; break;
                        default: throw ParsingError("Invalid escape sequence");
                    }
                    is_escape = false;
                }
                else if (ch == '\\') {
                    is_escape = true;
                    continue;
                }
                else if (ch == '"') {
                    break;
                }
                else {
                    str += ch;
                }
            }
            if (!input) {
                throw ParsingError("No '\"' symbol in the end of the string");
            }
            return Node(move(str));
        }
        Node LoadDict(istream& input) {
            Dict result;
            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }
                string key = LoadString(input).AsString();
                input >> c;
                result.insert({move(key), LoadNode(input)});
            }
            if (!input) {
                throw ParsingError("Need '}' symbol to close dictionary");
            }
            return Node(move(result));
        }
        Node LoadBool(std::istream& input) {
            if (bool value; input >> std::boolalpha >> value) {
                return Node(value);
            }

            throw ParsingError("Fail to read bool from stream"s);
        }
        Node LoadNull(istream& input) {
            if (input.get() == 'n' && input.get() == 'u' && input.get() == 'l' && input.get() == 'l') {
                return Node(nullptr);
            }
            else throw json::ParsingError("Fail to read null from stream"s);
        }
        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            } else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return std::stoi(parsed_num);
                    } catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }
        Node LoadNode(istream& input) {
            char c;
            input >> c;

            if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == '"') {
                return LoadString(input);
            }
            else if(c == 'f' || c == 't'){
                input.putback(c);
                return LoadBool(input);
            }
            else if( c == 'n'){
                input.putback(c);
                return LoadNull(input);
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

    }  // namespace

    // -----------------Конструкторы-----------------

    Node::Node(nullptr_t value)
        : node_(value) {
    }
    Node::Node(Array array)
        : node_(move(array)) {
    }
    Node::Node(Dict map)
        : node_(move(map)) {
    }
    Node::Node(int value)
        : node_(value) {
    }
    Node::Node(double value)
        : node_(value) {
    }
    Node::Node(bool value)
        : node_(value) {
    }
    Node::Node(string value)
        : node_(move(value)) {
    }

    //-----------------Методы класса Node-----------------------

    bool Node::IsNull() const {
        return holds_alternative<nullptr_t>(node_);
    }
    bool Node::IsArray() const {
        return holds_alternative<Array>(node_);
    }
    const Array& Node::AsArray() const {
        if(IsArray()){
            return get<Array>(node_);
        }
        else{
            throw std::logic_error("Not an array");
        }
    }
    bool Node::IsMap() const {
        return holds_alternative<Dict>(node_);
    }
    const Dict& Node::AsMap() const {
        if(IsMap()) {
            return get<Dict>(node_);
        }
        else{
            throw std::logic_error("Not a map");
        }
    }
    bool Node::IsInt() const {
        return holds_alternative<int>(node_);
    }
    int Node::AsInt() const {
        if(IsInt()) {
            return get<int>(node_);
        }
        else{
            throw std::logic_error("Not an integer");
        }
    }
    bool Node::IsDouble() const {
        return holds_alternative<double>(node_) || holds_alternative<int>(node_);
    }
    bool Node::IsPureDouble() const {
        return !holds_alternative<int>(node_) && holds_alternative<double>(node_);
    }
    double Node::AsDouble() const {
        if(IsInt()){
            return static_cast<double>(get<int>(node_));
        }
        else if(IsDouble()) {
            return get<double>(node_);
        }
        else{
            throw std::logic_error("Not a real number");
        }
    }
    bool Node::IsBool() const {
        return holds_alternative<bool>(node_);
    }
    bool Node::AsBool() const {
        if(IsBool()) {
            return get<bool>(node_);
        }
        else{
            throw std::logic_error("Not a bool");
        }
    }
    bool Node::IsString() const {
        return holds_alternative<string>(node_);
    }
    const string& Node::AsString() const {
        if(IsString()) {
            return get<string>(node_);
        }
        else{
            throw std::logic_error("Not a string");
        }
    }

    //--------------------Методы класса Document----------------------

    Document::Document(Node root)
            : root_(move(root)) {
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{LoadNode(input)};
    }

    //-------------------Функции вывода-------------------------------

    void PrintNode(const Node& node, std::ostream& out){
        if(node.IsNull()){
            out << "null"s;
        }
        if(node.IsBool()){
            if(node.AsBool()){
                out << "true"s;
            }
            else{
                out << "false"s;
            }
        }
        if(node.IsInt()) {
            out << node.AsInt();
        }
        if(node.IsDouble() && !node.IsInt()) {
            out << node.AsDouble();
        }
        if(node.IsString()) {
            out << "\""s;
            std::string result;
            for (auto ch : node.AsString()) {
                if (ch == '\n') {
                    result += "\\n";
                }
                else if (ch == '\r') {
                    result += "\\r";
                }
                else if (ch == '\t') {
                    result += "\\t";
                }
                else if (ch == '\\') {
                    result += "\\\\";
                }
                else if (ch == '"') {
                    result += "\\\"";
                }
                else {
                    result += ch;
                }
            }
            out << result << "\""s;
        }
        if(node.IsMap()){
            out << "{"s;
            bool is_first = true;
            for (const auto& [key, value] : node.AsMap()) {
                out << (!is_first ? ", " : "") << "\"" << key << "\": ";
                PrintNode(value, out);
                is_first = false;
            }
            out << "}"s;
        }
        if(node.IsArray()){
            out << "["s;
            bool is_first = true;
            for(const auto& elem : node.AsArray()){
                if(!is_first){
                    out << ',';
                }
                PrintNode(elem, out);
                is_first = false;
            }
            out << "]"s;
        }
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), output);
    }

}  // namespace json